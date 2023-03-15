## Android虚拟机中的锁优化 ##

**为什么要进行锁优化？**

答：多线程是运行独立的，需要内核这个管理者来进行临界区的管理，所以锁以阻塞的方式来实现，挂起线程和回复线程都需要转到内核态，太影响性能了。

**线程安全的方法有哪些？**

答：

* 阻塞同步（重量级锁）
* 非阻塞同步：CAS指令立flag
* 保证代码的可重入性：例如Linux信号处理函数，只访问局部变量，内部调用的函数也可重入
* 线程本地存储

Java虚拟机对`synchronized`关键字定义了两个字节码： monitorenter/monitorexit。而这两个字节码原先是通过各平台的原生互斥锁来实现，后续的Java虚拟机定义了一系列锁优化：

* 自旋锁(Spin Lock)：自旋等待一下锁释放
* 自适应锁(Adaptive Spinning)：自旋的时间可根据历史情况动态变化
* 锁消除(Lock Elimination)：只有单线程访问，没必要加锁
* 锁粗化(Lock Coarsening)：短时间内重复加锁解锁 -> 扩大锁范围
* 轻量级锁：加锁位CAS指令立flag， 解锁为CAS指令解除flag，如果另外一个线程要求锁，则自旋等待一下或者升级重量级锁
* 偏向锁：去除轻量级锁的解锁步骤，这样同一个线程后续省掉了CAS指令，如果另外一个线程要求锁，则直接升级为重量级锁

### Dalvik的锁实现 ###

Dailvik虚拟机中通过自旋锁、轻量级锁来对 `Object` 的进行锁优化

#### object lock: `thin` `fat` ####
Dailvik中所有的类型或对象都有 `Object` 定义如下，此`Object`非`Ljava/lang/Object`, 而是Java中的类型或对象或数组对象的虚拟机访问结构。

	/*
	 * There are three types of objects:
	 *  Class objects - an instance of java.lang.Class
	 *  Array objects - an object created with a "new array" instruction
	 *  Data objects - an object that is neither of the above
	 *
	 *
	 * All objects have an Object header followed by type-specific data.
	 */
	struct Object {
	    /* ptr to class object */
	    ClassObject*    clazz;
	
	    /*
	     * A word containing either a "thin" lock or a "fat" monitor.  See
	     * the comments in Sync.c for a description of its layout.
	     */
	    u4              lock;
	};

可见在虚拟机堆中分配的最小内存是 8 字节，
`u4 lock` 通过位来区分 `thin` 或者 `fat`

`thin`:

	 *    [31 ---- 19] [18 ---- 3] [2 ---- 1] [0]
	 *     lock count   thread id  hash state  0

* lock count: 当前`thin`锁定次数
* thread id: 持有当前 `thin lock` 的线程id
* hash state: ?
* 0: 0 代表object当前是`thin`状态

`fat`:

	 *    [31 ---- 3] [2 ---- 1] [0]
	 *      pointer   hash state  1

* pointer: `fat lock` 指针， 即 `Monitor` 对象：

		struct Monitor {
		    Thread*     owner;          /* which thread currently owns the lock? */
		    int         lockCount;      /* owner's recursive lock depth */
		    Object*     obj;            /* what object are we part of [debug only] */
		
		    Thread*     waitSet;	/* threads currently waiting on this monitor */
		
		    pthread_mutex_t lock;
		
		    Monitor*    next;
		
		    /*
		     * Who last acquired this monitor, when lock sampling is enabled.
		     * Even when enabled, ownerMethod may be NULL.
		     */
		    const Method* ownerMethod;
		    u4 ownerPc;
		};
* 1: 1 代表object当前是`fat`状态

#### `thin` 到 `fat` 转换（轻量级转重量级） ####

`dvmLockObject`(dalvik/vm/Sync.cpp)代码：

	void dvmLockObject(Thread* self, Object *obj)
	{
	    volatile u4 *thinp;
	    ThreadStatus oldStatus;
	    struct timespec tm;
	    long sleepDelayNs;
	    long minSleepDelayNs = 1000000;  /* 1 millisecond */
	    long maxSleepDelayNs = 1000000000;  /* 1 second */
	    u4 thin, newThin, threadId;
	
	    assert(self != NULL);
	    assert(obj != NULL);
	    threadId = self->threadId;
	    thinp = &obj->lock;
	retry:
	    thin = *thinp;
	    if (LW_SHAPE(thin) == LW_SHAPE_THIN) {
	        /*
	         * The lock is a thin lock.  The owner field is used to
	         * determine the acquire method, ordered by cost.
	         */
	        if (LW_LOCK_OWNER(thin) == threadId) {
	            /*
	             * The calling thread owns the lock.  Increment the
	             * value of the recursion count field.
	             */
	            obj->lock += 1 << LW_LOCK_COUNT_SHIFT;
	            if (LW_LOCK_COUNT(obj->lock) == LW_LOCK_COUNT_MASK) {
	                /*
	                 * The reacquisition limit has been reached.  Inflate
	                 * the lock so the next acquire will not overflow the
	                 * recursion count field.
	                 */
	                inflateMonitor(self, obj);
	            }
	        } else if (LW_LOCK_OWNER(thin) == 0) {
	            /*
	             * The lock is unowned.  Install the thread id of the
	             * calling thread into the owner field.  This is the
	             * common case.  In performance critical code the JIT
	             * will have tried this before calling out to the VM.
	             */
	            newThin = thin | (threadId << LW_LOCK_OWNER_SHIFT);
	            if (android_atomic_acquire_cas(thin, newThin,
	                    (int32_t*)thinp) != 0) {
	                /*
	                 * The acquire failed.  Try again.
	                 */
	                goto retry;
	            }
	        } else {
	            ALOGV("(%d) spin on lock %p: %#x (%#x) %#x",
	                 threadId, &obj->lock, 0, *thinp, thin);
	            /*
	             * The lock is owned by another thread.  Notify the VM
	             * that we are about to wait.
	             */
	            oldStatus = dvmChangeStatus(self, THREAD_MONITOR);
	            /*
	             * Spin until the thin lock is released or inflated.
	             */
	            sleepDelayNs = 0;
	            for (;;) {
	                thin = *thinp;
	                /*
	                 * Check the shape of the lock word.  Another thread
	                 * may have inflated the lock while we were waiting.
	                 */
	                if (LW_SHAPE(thin) == LW_SHAPE_THIN) {
	                    if (LW_LOCK_OWNER(thin) == 0) {
	                        /*
	                         * The lock has been released.  Install the
	                         * thread id of the calling thread into the
	                         * owner field.
	                         */
	                        newThin = thin | (threadId << LW_LOCK_OWNER_SHIFT);
	                        if (android_atomic_acquire_cas(thin, newThin,
	                                (int32_t *)thinp) == 0) {
	                            /*
	                             * The acquire succeed.  Break out of the
	                             * loop and proceed to inflate the lock.
	                             */
	                            break;
	                        }
	                    } else {
	                        /*
	                         * The lock has not been released.  Yield so
	                         * the owning thread can run.
	                         */
	                        if (sleepDelayNs == 0) {
	                            sched_yield();
	                            sleepDelayNs = minSleepDelayNs;
	                        } else {
	                            tm.tv_sec = 0;
	                            tm.tv_nsec = sleepDelayNs;
	                            nanosleep(&tm, NULL);
	                            /*
	                             * Prepare the next delay value.  Wrap to
	                             * avoid once a second polls for eternity.
	                             */
	                            if (sleepDelayNs < maxSleepDelayNs / 2) {
	                                sleepDelayNs *= 2;
	                            } else {
	                                sleepDelayNs = minSleepDelayNs;
	                            }
	                        }
	                    }
	                } else {
	                    /*
	                     * The thin lock was inflated by another thread.
	                     * Let the VM know we are no longer waiting and
	                     * try again.
	                     */
	                    ALOGV("(%d) lock %p surprise-fattened",
	                             threadId, &obj->lock);
	                    dvmChangeStatus(self, oldStatus);
	                    goto retry;
	                }
	            }
	            ALOGV("(%d) spin on lock done %p: %#x (%#x) %#x",
	                 threadId, &obj->lock, 0, *thinp, thin);
	            /*
	             * We have acquired the thin lock.  Let the VM know that
	             * we are no longer waiting.
	             */
	            dvmChangeStatus(self, oldStatus);
	            /*
	             * Fatten the lock.
	             */
	            inflateMonitor(self, obj);
	            ALOGV("(%d) lock %p fattened", threadId, &obj->lock);
	        }
	    } else {
	        /*
	         * The lock is a fat lock.
	         */
	        assert(LW_MONITOR(obj->lock) != NULL);
	        lockMonitor(self, LW_MONITOR(obj->lock));
	    }
	}

上述`thin`到`fat`的转换步骤：

* `Object` 默认是在`thin`状态
* 锁进入：当前是`thin`状态，判断持有人的`thread id`是否是当前线程:
	* 如果`thread id`为0，则无人持有，这时CAS指令赋值 `thread id`, 把锁的`lock`状态占上
	* 如果是当前线程，则锁已经占用了，可以放心使用，lock count++
	* 如果不是当前线程，则 for 死循环自旋等待，自旋等待，自旋过程通过，`sched_yield`和`nanosleep`来让出线程，`nanosleep`唤醒后判断一下锁状态：
		* 锁是`thin`状态，并且`thread id`为0，没人占用了。此时CAS指令赋值`thread id`，同时执行`inflateMonitor`将锁升级为重量级锁（`fat`），函数见下，为什么此时升级？说明该锁有多线程同步的要求，升级一下锁，以便后续锁同步情况发生（锁如果一直单线程用，则不会出现升级场景）

				static void inflateMonitor(Thread *self, Object *obj)
				{
				    Monitor *mon;
				    u4 thin;
				
				    assert(self != NULL);
				    assert(obj != NULL);
				    assert(LW_SHAPE(obj->lock) == LW_SHAPE_THIN);
				    assert(LW_LOCK_OWNER(obj->lock) == self->threadId);
				    /* Allocate and acquire a new monitor. */
				    mon = dvmCreateMonitor(obj);
				    lockMonitor(self, mon);
				    /* Propagate the lock state. */
				    thin = obj->lock;
				    mon->lockCount = LW_LOCK_COUNT(thin);
				    thin &= LW_HASH_STATE_MASK << LW_HASH_STATE_SHIFT;
				    thin |= (u4)mon | LW_SHAPE_FAT;
				    /* Publish the updated lock word. */
				    android_atomic_release_store(thin, (int32_t *)&obj->lock);
				}
		* 锁是`thin`状态，还有人占用，再递增睡眠时间，继续`nanosleep`(自旋)
		* 锁是`fat`状态，已经有其他线程忍不住了，出手升级了，我们只要进入`fat`阻塞等待状态即可。
* 锁进入：当前是`fat`状态，直接阻塞等待即可。

### ART 锁实现 ###