# Android 后台任务


[Google Developers: Background Tasks](https://developer.android.com/training/best-background)

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
    
    @MainThread
    public final AsyncTask<Params, Progress, Result> execute(Params... params){
        return executeOnExecutor(sDefaultExecutor, params);
    }

    @MainThread
    public final AsyncTask<Params, Progress, Result> executeOnExecutor(Executor exec,
            Params... params) {
		...
        mStatus = Status.RUNNING;

        onPreExecute();

        mWorker.mParams = params;
        exec.execute(mFuture);

        return this;
    }
    
* `mHandler` 在 `publishProgress` 和 `postResult` 时向主线程发送消息
* `mWorker` 后台工作 `Runnable`
* `mFuture` 处理工作结束时要做的事：`postResultIfNotInvoked`，`Future` 其实也是个 `Runnable` ，它的 `run()` 函数内部执行 `WorkerRunnable mWorker` 的 `run()` 函数，`AsyncTask` 执行`execute()` 时调用的是 `Future` 的 `run()` 函数
* `Future`为什么称之为Future，因为内部定义接口：`isDone``get`可选择阻塞等待任务完成

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

    //ThreadPoolExecutor.java
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

## 推荐 ##

### JobIntentService ###
* `IntentService`的升级版，`support`包加入，支持高低版本API一致，低版本用`IntentService`实现，Android 8.0以后（targetSdkVersion 26）以后，因为后台任务限制，换用`JobService`实现。

#### 优点 ####
* 一套api适配不同Android版本
* 执行任务期间`wakelock`保护
* 任务执行中，被系统杀死时，可以选择重新安排任务

#### 缺点 ####
* 任务会立刻执行，不能向`JobScheduler`一样选择条件，内部使用`JobScheduler`时，调用`jobInfo.setOverrideDeadline(0).build()`

[jobintentservice](https://medium.com/til-kotlin/jobintentservice-for-background-processing-on-android-o-39535460e060)

[Schedule jobs intelligently](https://developer.android.com/topic/performance/scheduling)

### JobScheduler
[scheduling-jobs-like-a-pro-with-jobscheduler](https://medium.com/google-developers/scheduling-jobs-like-a-pro-with-jobscheduler-286ef8510129)

##### 触发时机 #####
* 网络可达性
* 定时
* 充电或灭屏Idle
* `ContentProvider`变化

##### 注意点 #####
* 5.0以上才支持
* 在`Doze`模式下，短则半小时，长则几个小时才能触发一次窗口活动，窗口活动最长10分钟，10分钟内如果没做完，系统也直接进入Idle状态。未做完的事需等待下一个窗口。
* 应用如果进入`Standby`模式，则一天内可能只有一次窗口期执行任务。
* `JobService`是`Service`的之类，`Service`在Android 8.0不让用了，用`JobService`代替

### AlarmManager ###

* 5.0以下搭配`Service`使用

### Firebase JobDispatcher ###

* 需要 Google Play Service
* 支持5.0以下的类`JobScheduler`API接口


### DownloadManager ###

* 但需要通过HTTP下载东西时，最好通过`DownloadManager`下载，因为它是一个前台服务

### WorkManager（Jetpack推荐） ###

* 整合`JobScheudler``JobDispatcher`(可选)`AlarmManager`的后台任务
## 不推荐 ##
### IntentService ###

* Android O(api 26)版本以下使用
* `IntentService`相当好用，相当于升级版的`AsyncTask`，`IntentService`内部通过`Thread``Looper``Handler`实现了类似`HandlerThread`机制：

        HandlerThread thread = new HandlerThread("IntentService[" + mName + "]");
        thread.start();

        mServiceLooper = thread.getLooper();
        mServiceHandler = new ServiceHandler(mServiceLooper);

* `IntentService`替换版本是`JobIntentService`，Android O之后通过`JobService`实现
* **注意`onHandleIntent`函数内处理事件是没有`wakelock`保护的，如果工作必须保证要完成，可以添加`wakelock`，如果是从`broadcast`触发的任务，则换用`WakefulBroadcastReceiver`**

#### 延伸 ####
[aquire-partial-wakelock-in-a-intentservice](https://stackoverflow.com/questions/41953458/aquire-partial-wakelock-in-a-intentservice) 这篇问答里面说到，除了闹钟和消息提醒等必须要用到`wakelock`保证外，其他后台任务都可以不用`wakelock`。而且普通的`AlarmManager`触发的`onReceive`函数都是`wakelock`保证的，`onReceive`执行完毕就不保证了

*问：为什么不通过`HandlerThread`来执行多个任务？*

答：因为`HandlerThread`不能后台执行，试想当应用从前台切到后台，而`HandlerThread`还在处理业务，如果此时内存紧张，根据应用优先级[process-lifecycle](https://developer.android.com/guide/components/activities/process-lifecycle)，此时应用优先级比后台`Service`低，极有可能被杀，而且杀掉之后还没方法恢复，其他场景如：后台定时Alarm，后台推送消息处理。

所以`IntentService`优势如下：

* 通过`Service`提高应用优先级，避免后台被杀；
* 被杀之后恢复，`onStartCommond`返回`START_REDELIVER_INTENT`可以让系统恢复`Service`，并重新开始上一次未执行完毕的任务。
* 但所有`Intent`被处理完之后，`Service`自动关闭

*问：`IntentService`异步线程是单线程执行，如何变成多线程执行？*

* 可以自定义类继承`Service`, 实现多个`Looper``Thread`，具体实现例子见下文

[官网：ExtendingIntentService](https://developer.android.com/guide/components/services#ExtendingIntentService)



### SyncAdapter

* 用于同步账号和设备相关信息给服务器
* 代码实现较复杂，需要 `authenticator` 和 `content provider`等相关实现
* 官方建议用`JobScheduler`代替，除非想用`SyncAdapter`独有特性 

### Service ##

**`targetSdkVersion 26`及以上，不允许后台运行`Service`，应用后台时不允许调用`startService`，前台`Service`属于前台**

* 应用从前台切到后台，还有几分钟的窗口期，窗口期过后，`Service`被系统强制关闭
* 在`Application`里面没有必要调用`startService`，因为当应用在后台被系统重启时，`startService`调用会抛异常

## 第三方方案 ##

### EventBus

### RxJava