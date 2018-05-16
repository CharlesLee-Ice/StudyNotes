# Android 后台任务


## AsyncTask

#### Threading rules
* The AsyncTask class must be loaded on the UI thread. This is done automatically as of JELLY_BEAN.
* The task instance must be created on the UI thread.
* execute(Params...) must be invoked on the UI thread.

#### 为什么AsyncTask实例化和execute必需要在UI线程？
答：因为onPreExecute、onProgressUpdate、onPostExecute、onCancelled会涉及到UI更新，只有AsyncTask在UI线程初始化和execute才能保证这些函数在UI线程环境执行。

#### 为什么AsyncTask不适合执行耗时较长的任务？
答：

1、因为AsyncTask经常通过匿名内部类来使用，较长的后台时间，锁住了外部类对象，影响外部类对象的回收

2、AsyncTask 内部使用 SerialExecutor 来执行任务，多个异步任务顺序执行，一个较长的任务就会阻塞其它。当然`setDefaultExecutor`可以替换SerialExecutor

### 原理

#### 初始化

    public AsyncTask(@Nullable Looper callbackLooper) {
        mHandler = callbackLooper == null || callbackLooper == Looper.getMainLooper()
            ? getMainHandler()
            : new Handler(callbackLooper);

        mWorker = new WorkerRunnable<Params, Result>() {
            public Result call() throws Exception {
                mTaskInvoked.set(true);
                Result result = null;
                try {
                    Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND);
                    //noinspection unchecked
                    result = doInBackground(mParams);
                    Binder.flushPendingCommands();
                } catch (Throwable tr) {
                    mCancelled.set(true);
                    throw tr;
                } finally {
                    postResult(result);
                }
                return result;
            }
        };

        mFuture = new FutureTask<Result>(mWorker) {
            @Override
            protected void done() {
                try {
                    postResultIfNotInvoked(get());
                } catch (InterruptedException e) {
                    android.util.Log.w(LOG_TAG, e);
                } catch (ExecutionException e) {
                    throw new RuntimeException("An error occurred while executing doInBackground()",
                            e.getCause());
                } catch (CancellationException e) {
                    postResultIfNotInvoked(null);
                }
            }
        };
    }

* mHandler 在 publishProgress 和 postResult 时向主线程发送消息
* mWorker 后台工作 Runnable
* mFuture 处理工作结束时要做的事：postResultIfNotInvoked，Future 其实也是个 Runnable ，它的 `run()` 函数内部执行 WorkerRunnable 的 `run()` 函数，AsyncTask 执行`execute()` 时调用的是 Future 的 `run()` 函数

#### 执行

##### 1、SerialExecutor 顺序执行

	private static volatile Executor sDefaultExecutor = SERIAL_EXECUTOR;

    private static class SerialExecutor implements Executor {
        final ArrayDeque<Runnable> mTasks = new ArrayDeque<Runnable>();
        Runnable mActive;

        public synchronized void execute(final Runnable r) {
            mTasks.offer(new Runnable() {
                public void run() {
                    try {
                        r.run();
                    } finally {
                        scheduleNext();
                    }
                }
            });
            if (mActive == null) {
                scheduleNext();
            }
        }

        protected synchronized void scheduleNext() {
            if ((mActive = mTasks.poll()) != null) {
                THREAD_POOL_EXECUTOR.execute(mActive);
            }
        }
    }

    @MainThread
    public final AsyncTask<Params, Progress, Result> execute(Params... params) {
        return executeOnExecutor(sDefaultExecutor, params);
    }

    @MainThread
    public static void execute(Runnable runnable) {
        sDefaultExecutor.execute(runnable);
    }
    
SerialExecutor 为 AsyncTask 的默认执行器，由此可知，如果多个 AsyncTask 同时执行，则这些后台任务会顺序执行。

可以通过 `setDefaultExecutor ` 替换 `sDefaultExecutor `

##### 2、ThreadPoolExecutor 线程池执行

    /**
     * An {@link Executor} that can be used to execute tasks in parallel.
     */
    public static final Executor THREAD_POOL_EXECUTOR;

    static {
        ThreadPoolExecutor threadPoolExecutor = new ThreadPoolExecutor(
                CORE_POOL_SIZE, MAXIMUM_POOL_SIZE, KEEP_ALIVE_SECONDS, TimeUnit.SECONDS,
                sPoolWorkQueue, sThreadFactory);
        threadPoolExecutor.allowCoreThreadTimeOut(true);
        THREAD_POOL_EXECUTOR = threadPoolExecutor;
    }
    
多线程并行执行异步任务，这里并行是否有意义？毕竟已经有 SerialExecutor 的存在。

#### ThreadPoolExecutor

##### 两个关键变量

    private final class Worker
        extends AbstractQueuedSynchronizer
        implements Runnable
    {
        /** Thread this worker is running in.  Null if factory fails. */
        final Thread thread;
        /** Initial task to run.  Possibly null. */
        Runnable firstTask;
        /** Per-thread task counter */
        volatile long completedTasks;

        /**
         * Creates with given first task and thread from ThreadFactory.
         * @param firstTask the first task (null if none)
         */
        Worker(Runnable firstTask) {
            setState(-1); // inhibit interrupts until runWorker
            this.firstTask = firstTask;
            this.thread = getThreadFactory().newThread(this);
        }

        /** Delegates main run loop to outer runWorker. */
        public void run() {
            runWorker(this);
        }
    }
	
	private final HashSet<Worker> workers = new HashSet<>();
	private final BlockingQueue<Runnable> workQueue;

Worker 继承 Runnable， 一个 Worker 保存一个线程对象， 线程执行 Worker 的 `run()` 函数，最大线程个数通过 `maximumPoolSize` 变量控制

    private boolean addWorker(Runnable firstTask, boolean core) {
		...
	
        boolean workerStarted = false;
        boolean workerAdded = false;
        Worker w = null;
        try {
            w = new Worker(firstTask);
            final Thread t = w.thread;
            if (t != null) {
                final ReentrantLock mainLock = this.mainLock;
                mainLock.lock();
                try {
                    // Recheck while holding lock.
                    // Back out on ThreadFactory failure or if
                    // shut down before lock acquired.
                    int rs = runStateOf(ctl.get());
	
                    if (rs < SHUTDOWN ||
                        (rs == SHUTDOWN && firstTask == null)) {
                        if (t.isAlive()) // precheck that t is startable
                            throw new IllegalThreadStateException();
                        workers.add(w);
                        int s = workers.size();
                        if (s > largestPoolSize)
                            largestPoolSize = s;
                        workerAdded = true;
                    }
                } finally {
                    mainLock.unlock();
                }
                if (workerAdded) {
                    t.start();
                    workerStarted = true;
                }
            }
        } finally {
            if (! workerStarted)
                addWorkerFailed(w);
        }
        return workerStarted;
    }

当外界调用 `void execute(Runnable command);` 函数时，如果当前 workers 个数小于 `maximumPoolSize`，则通过 `addWorker` 添加线程执行任务，如果当前 workers 个数已满，则执行 `workQueue.offer(command)` 添加到等待队列中去。

每个 Worker 线程真正执行的函数时 `runWorker`, 内部首先执行 `firstTask`， `firstTask` 就是之前 `addWorker` 时需要执行的任务，这里有个 `while` 循环，`firstTask` 执行完毕再通过 `getTask` 判断 `workQueue` 中有没有等待的任务，如果有，则执行之。

		final void runWorker(Worker w) {
	        Thread wt = Thread.currentThread();
	        Runnable task = w.firstTask;
	        w.firstTask = null;
	        w.unlock(); // allow interrupts
	        boolean completedAbruptly = true;
	        try {
	            while (task != null || (task = getTask()) != null) {
	                w.lock();
	                // If pool is stopping, ensure thread is interrupted;
	                // if not, ensure thread is not interrupted.  This
	                // requires a recheck in second case to deal with
	                // shutdownNow race while clearing interrupt
	                if ((runStateAtLeast(ctl.get(), STOP) ||
	                     (Thread.interrupted() &&
	                      runStateAtLeast(ctl.get(), STOP))) &&
	                    !wt.isInterrupted())
	                    wt.interrupt();
	                try {
	                    beforeExecute(wt, task);
	                    Throwable thrown = null;
	                    try {
	                        task.run();
	                    } catch (RuntimeException x) {
	                        thrown = x; throw x;
	                    } catch (Error x) {
	                        thrown = x; throw x;
	                    } catch (Throwable x) {
	                        thrown = x; throw new Error(x);
	                    } finally {
	                        afterExecute(task, thrown);
	                    }
	                } finally {
	                    task = null;
	                    w.completedTasks++;
	                    w.unlock();
	                }
	            }
	            completedAbruptly = false;
	        } finally {
	            processWorkerExit(w, completedAbruptly);
	        }
    	}
  
`getTask` 通过 `poll` `take` 来阻塞等待 `BlockingQueue<Runnable> workQueue`，如果初始化 `ThreadPoolExecutor` 时设置了 `keepAliveTime`（超过 `corePoolSize` 的线程 `poll` 等待 `keepAliveTime`，如果还没有任务，则关闭线程）
  
      private Runnable getTask() {
      	...
        for (;;) {
            int c = ctl.get();
            ...
            int wc = workerCountOf(c);

            // Are workers subject to culling?
            boolean timed = allowCoreThreadTimeOut || wc > corePoolSize;

            try {
                Runnable r = timed ?
                    workQueue.poll(keepAliveTime, TimeUnit.NANOSECONDS) :
                    workQueue.take();
                if (r != null)
                    return r;
                    ...
            } catch (InterruptedException retry) {
            	...
            }
        }
    }
    
**PS：`ThreadPoolExecutor` 应用可以直接使用，如果有多个后台任务需要同时执行，则只要初始化 `ThreadPoolExecutor`,并多次调用 `public void execute(Runnable command)` 即可**

## JobService

## SyncAdapter

## EventBus

## RxJava