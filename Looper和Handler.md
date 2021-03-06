## Looper
#### Looper相当于一个管理员

`Looper`类中有两个关键函数`prepare()``loop()`函数，在`ActivityThread`(UI主线程)或`HandlerThread`中的`run()`函数内主要就进行`Looper`的这两个函数，每一个`Looper`类掌管着自己的`MessageQueue`，所以说`Looper`是一个管理者。

	    //Looper.java
	    private static Looper sMainLooper;  // guarded by Looper.class
	
	    final MessageQueue mQueue;
	    final Thread mThread;
	    
* 每一个线程拥有一个Looper对象，Looper对象存储在静态变量sThreadLocal里面
* Thread内部有个`ThreadLocal.ThreadLocalMap threadLocals = null;` 对象，ThreadLocal实现对这个map的管理
* `final MessageQueue mQueue;`是本线程的消息队列
* `private static Looper sMainLooper;`静态变量，这样其他线程可以自由获取UI线程的Looper

#### Looper主要包含两个静态方法: prepare、loop
线程启动后，要先调用prepare方法，prepare创建当前线程所属的looper对象，并将looper对象存储到Thread对象的map里，looper对象实例化时，创建属于自己的消息队列

    //Looper.java
	static final ThreadLocal<Looper> sThreadLocal = new ThreadLocal<Looper>();
    
    private static void prepare(boolean quitAllowed) {
        if (sThreadLocal.get() != null) {
            throw new RuntimeException("Only one Looper may be created per thread");
        }
        sThreadLocal.set(new Looper(quitAllowed));
    }

`ThreadLocal.java`的`set()`函数

    public void set(T value) {
        Thread t = Thread.currentThread();
        ThreadLocalMap map = getMap(t);
        if (map != null)
            map.set(this, value);
        else
            createMap(t, value);
    }

线程其次调用loop方法，loop方法首先通过myLooper获取当前线程环境下的Looper对象，然后遍历MessageQueue处理消息

    public static void loop() {
        final Looper me = myLooper();
        if (me == null) {
            throw new RuntimeException("No Looper; Looper.prepare() wasn't called on this thread.");
        }
        final MessageQueue queue = me.mQueue;

        for (;;) {
            Message msg = queue.next(); // might block
            if (msg == null) {
                // No message indicates that the message queue is quitting.
                return;
            }

            msg.target.dispatchMessage(msg);

            msg.recycleUnchecked();
        }
    }
    
    /**
     * Return the Looper object associated with the current thread.  Returns
     * null if the calling thread is not associated with a Looper.
     */
    public static @Nullable Looper myLooper() {
        return sThreadLocal.get();
    }
    
## Handler
#### Handler相当于消息的搬运工，往消息队列`MessageQueue mQueue`发送消息的是它，处理消息的也是它`msg.target.dispatchMessage(msg);`

	//Handler.java
    private boolean enqueueMessage(MessageQueue queue, Message msg, long uptimeMillis) {
        msg.target = this; //表明处理着是Handler对象自己
        if (mAsynchronous) {
            msg.setAsynchronous(true);
        }
        return queue.enqueueMessage(msg, uptimeMillis);
    }
    
#### Handler怎么找到当前线程的消息队列的呢？
答：调用Looper类的静态方法`myLooper()`

#### 不同线程中初始化的Handler不一样，为什么？
答：`new Handler()` 内部调用`Looper.myLooper()`函数，获取的是自己所在线程的管理员，如果异步线程想要向主线程发消息，调用`new Handler(Looper.getMainLooper())`

#### 避免Handler匿名类锁住外部类

        Handler().postDelayed({
            this@MainActivity
        }, 3000)
如上例中，延迟的匿名类锁住了外部类 MainActivity 3000毫秒
