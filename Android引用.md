## WeakReference、SoftReference、PhantomReference、FinalizerReference ##

* `WeakReference`、`SoftReference`、`PhantomReference`、`FinalizerReference`基类都是`Reference`
* `WeakReference`、`SoftReference`、`PhantomReference`都是JDK定义的类，`FinalizerReference`是Android自己定义的，为了实现保存对象回收前做一些事（调用`Object.finalize()`)
* 当重载了`Object`的`finalize()`方法后，JVM通过`FinalizerReference`实现对象回收前的`finalize()`执行
* `ReferenceQueue`是一种GC回收对象的通知机制：`Reference`内的对象被回收后，`Reference`会添加到`ReferenceQueue`，Java程序一般在相应对象被回收后通过`ReferenceQueue`得到通知，做一些其它堆内存的回收工作

***`WeakReference`类内部引用保存的对象，`WeakReference`自己被长时间引用时，为什么不影响保存对象的回收***

JVM垃圾回收从根节点开始，遇到`WeakReference`，忽略其内部对保存对象的引用。当外界调用`WeakReference`的`get`方法时，其实调用的是native方法：

    @FastNative
    private final native T getReferent();

JVM回收完保存的对象后，此方法返回`null`。

如果`Reference`内部保存的对象没有被回收，就算没有其它地方引用此`Reference`，它自己是不会被JVM回收的，为什么呢？因为`Reference`对象还有使用价值：JVM回收完`Reference`内部保存的对象后，回调用下面引述的`add`方法，将`Reference`加入队列。

***为什么Android偏向使用`WeakReference`而不是`SoftReference`？***

`SoftReference`内部通过比较保存的对象的存活时间与内存的剩余值来决定是否回收保存的对象，是一种缓存机制。Server端偏向于性能，所以用`SoftReference`做缓存，Client端偏向于减少内存资源，所以推荐使用`WeakReference`。

***`PhantomReference`的有什么使用意义？***

`PhantomReference`是为了监控对象回收，`PhantomReference` 有两个特点：
1. 构造函数必须传入 `ReferenceQueue`
2. `get`函数一直返回`null`
这两个特点表明：只能通过`ReferenceQueue`是否有数据，来判断对象是否被回收。
当然，同样的事也可以通过`WeakReference`来做，不过这不是`WeakReference`的本意。

***`FinalizerReference`实现对象被回收前`Object.finalize()`方法调用的原理是什么？***

要回答这个问题，首先需要理解`Reference`构造函数都带有一个可选参数`ReferenceQueue`的作用，当JVM在native层回收完对象后，会将对应的`Reference`对象添加到调用`ReferenceQueue`内部的静态队列：
	
	public static Reference<?> unenqueued = null;

    static void add(Reference<?> list) {
        synchronized (ReferenceQueue.class) {
            if (unenqueued == null) {
                unenqueued = list;
            } else {
                // Find the last element in unenqueued.
                Reference<?> last = unenqueued;
                while (last.pendingNext != unenqueued) {
                  last = last.pendingNext;
                }
                // Add our list to the end. Update the pendingNext to point back to enqueued.
                last.pendingNext = list;
                last = list;
                while (last.pendingNext != list) {
                    last = last.pendingNext;
                }
                last.pendingNext = unenqueued;
            }
            ReferenceQueue.class.notifyAll();
        }
    }

这里添加动作是通过JNI反射调用`ReferenceQueue`内部的`add`方法，这个`unenqueued`队列被谁消费呢？
JVM启动时会开启一系列`Daemon`线程（`Daemons.java`），其中包含`ReferenceQueueDaemon`

    private static class ReferenceQueueDaemon extends Daemon {
		...
        @Override public void runInternal() {
            while (isRunning()) {
                Reference<?> list;
                try {
                    synchronized (ReferenceQueue.class) {
                        while (ReferenceQueue.unenqueued == null) {
                            ReferenceQueue.class.wait();
                        }
                        list = ReferenceQueue.unenqueued;
                        ReferenceQueue.unenqueued = null;
                    }
                } catch (InterruptedException e) {
                    continue;
                } catch (OutOfMemoryError e) {
                    continue;
                }
                ReferenceQueue.enqueuePending(list);
            }
        }
    }

这个线程工作就是消费“内部对象被回收的`Reference`对象”，具体的消费方法是最后一句，具体代码如下：

    public static void enqueuePending(Reference<?> list) {
        Reference<?> start = list;
        do {
            ReferenceQueue queue = list.queue;
            if (queue == null) {
                Reference<?> next = list.pendingNext;

				list.pendingNext = list;
  				list = next;
            } else {
                synchronized (queue.lock) {
                    do {
                        Reference<?> next = list.pendingNext;

                        list.pendingNext = list;
                        queue.enqueueLocked(list);
                        list = next;
                    } while (list != start && list.queue == queue);
                    queue.lock.notifyAll();
                }
            }
        } while (list != start);
    }

这里有两种情况：`Reference`构造函数传入的`queue`为`null`还是不为`null`，
1. 如果为`null`，则将`Reference`对象从列表中剥出，`pendingNext`赋值自己（自己引用自己），避免队列中的引用，从而被JVM回收；
2. 如果不为`null`，则将`Reference`对象添加到当初构造函数传入的`queue`上去。从而外界程序可以观察到`Reference`保存的对象已经被回收了。

再来看`FinalizerReference`如何实现`Object.finalize`调用。
`Daemon`线程中还包含`FinalizerDaemon`线程，代码如下：

    private static class FinalizerDaemon extends Daemon {
		...
        private final ReferenceQueue<Object> queue = FinalizerReference.queue;
        private Object finalizingObject = null;

        @Override public void runInternal() {
            while (isRunning()) {
                try {
                    FinalizerReference<?> finalizingReference = (FinalizerReference<?>)queue.poll();
                    if (finalizingReference != null) {
                        finalizingObject = finalizingReference.get();
                        progressCounter.lazySet(++localProgressCounter);
                    } else {
                        finalizingObject = null;
                        progressCounter.lazySet(++localProgressCounter);
                        // Slow path; block.
                        FinalizerWatchdogDaemon.INSTANCE.goToSleep();
                        finalizingReference = (FinalizerReference<?>)queue.remove();
                        finalizingObject = finalizingReference.get();
                        progressCounter.set(++localProgressCounter);
                        FinalizerWatchdogDaemon.INSTANCE.wakeUp();
                    }
                    doFinalize(finalizingReference);
                } catch (InterruptedException ignored) {
                } catch (OutOfMemoryError ignored) {
                }
            }
        }

        @FindBugsSuppressWarnings("FI_EXPLICIT_INVOCATION")
        private void doFinalize(FinalizerReference<?> reference) {
            FinalizerReference.remove(reference);
            Object object = reference.get();
            reference.clear();
            try {
                object.finalize();
            } catch (Throwable ex) {
            } finally {
                finalizingObject = null;
            }
        }
    }

首先需要找到`FinalizerReference.queue`是谁？

	public final class FinalizerReference<T> extends Reference<T> {
	    public static final ReferenceQueue<Object> queue = new ReferenceQueue<Object>();
	
	    public static void add(Object referent) {
	        FinalizerReference<?> reference = new FinalizerReference<Object>(referent, queue);
	        ...
	    }
	}

当系统要回收一个重载了`finalize()`方法的对象前，JNI反射调用上述`add`方法，这里会`new FinalizerReference<Object>(referent, queue)`。参考上面`ReferenceQueue`的介绍，这里的`queue`就是`Reference`类构造函数的参数，通过`ReferenceQueueDeamon`来添加数据，监听对象的回收（只不过这里是对象回收前）。
当`queue`中添加了数据后，就轮到`FinalizerDaemon`线程的消费了。具体消费过程就是执行`object.finalize()`

## WeakHashMap ##

* `WeakHashMap`是一种`key -> value`的缓存机制，当内存足够时，`WeakHashMap`保留对象，当内存不够时，`GC`自动回收其存放数据
* 当没有主动增删数据时，不同时机调用`WeakHashMap`对象的`size()``isEmpty()``containsKey()``get()`会不一样

### 实现原理 ###
`WeakHashMap`内部通过 `Entry`继承 `WeakReference`来实现
    
    private final ReferenceQueue<Object> queue = new ReferenceQueue<>();
    
    private static class Entry<K,V> extends WeakReference<Object> implements Map.Entry<K,V> {
        V value;
        final int hash;
        Entry<K,V> next;

        Entry(Object key, V value,
              ReferenceQueue<Object> queue,
              int hash, Entry<K,V> next) {
            super(key, queue);
            this.value = value;
            this.hash  = hash;
            this.next  = next;
        }
		...
    }

`super(key, queue)`将`key`通过`WeakReference`来管理，同时传入`ReferenceQueue`，这样当内存不足时，`key`对象被回收，`ReferenceQueue`中添加`WeakReference`（在这里就是`Entry`）。而在操作`WeakHashMap``size()``resize()``getTable()`等方法内，会`poll`检查`ReferenceQueue<Object> queue`中存放的数据，找到`Entry`对象，从`HashMap`拉链结构中删除。

## 参考 ##

[Reference深入浅出](https://blog.csdn.net/KM_Cat/article/details/51607231)