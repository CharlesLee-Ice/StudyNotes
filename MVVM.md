## MVVM

* 使用官方 `support(26.1.0之后)`，`FragmentActivity` 和 `Fragment` 添加 `Lifecycle` 的支持，有了`Lifecycle`的支持后，使用 `LiveData` 就可以限定只在`FragmentActivity``Fragment`生命周期内通知数据变化。
* 官方 `support` 包依赖 `implementation "android.arch.lifecycle:runtime:$lifecycle_version`，如果我们需要自定义使用`Lifecycle`，可以单独依赖。
* `LiveData`如果`observe`时指定`LifecycleOwner`，则`LiveData`调用`dispatchingValue`时，即`considerNotify`会判断监控者的生命周期`shouldBeActive`，决定是否通知，如果我们在`FragmentActivity``Fragment`中选择`observeForever`则`dispatchingValue`忽略生命周期。

        boolean shouldBeActive() {
            return mOwner.getLifecycle().getCurrentState().isAtLeast(STARTED);
        }

* 将数据`LiveData`放到`ViewModel`中，原因有以下两点：
 1. 避免`Activity``Fragment`管理数据，做到界面和数据分离；
 2. `ViewModel`虽然是通过`ViewModelStore mViewModelStore`作为成员变量存放在`FragmentActivity`类内部的，但是通过`retain fragment`机制能够横跨`FragmentActivity`的生命周期，解决因为横竖屏切换，导致`FragmentActivity`销毁重建，数据丢失问题。
 3. 不同`Fragment`可以通过`ViewModel`共享数据，只要将`ViewModel`放到`Activity`中即可。
* 如果`FragmentActivity`和`Fragment`被回收了，则`ViewModel`也会回收。
* `LiveData`的`setValue`属性是`protected`，也就是说我们拿到`LiveData`对象是不能改变里面数据的，但是`MutableLiveData`可以。`MediatorLiveData`继承`MutableLiveData`,是自定义的一种`Transformation.map/switchMap`
* 当`LiveData`切后台后，`Activity`和`Fragment`的`LifecycleOwner`会从`RESUMED`到`STARTED`再到`CREATED`，所以后台数据是不会更新到UI的，但如果再切前台，状态又会变成`RESUMED`，并且此时`Activity`和`Fragment`会调用`LifecycleBoundObserver`的`onStateChanged`函数，查看是否有后台未更新的数据，如果有则更新。

问：如何判断是否有数据更新，如果切后台再切前台，是否要`dispatchingValue`？

答：判断`Observer`当前数据的版本`observer.mLastVersion >= mVersion`选择是否通知。

    private void considerNotify(ObserverWrapper observer) {
        if (!observer.mActive) {
            return;
        }
        // Check latest state b4 dispatch. Maybe it changed state but we didn't get the event yet.
        //
        // we still first check observer.active to keep it as the entrance for events. So even if
        // the observer moved to an active state, if we've not received that event, we better not
        // notify for a more predictable notification order.
        if (!observer.shouldBeActive()) {
            observer.activeStateChanged(false);
            return;
        }
        if (observer.mLastVersion >= mVersion) {
            return;
        }
        observer.mLastVersion = mVersion;
        //noinspection unchecked
        observer.mObserver.onChanged((T) mData);
    }

## 使用准则
* Don’t let ViewModels (and Presenters) know about Android framework classes, Views should only know how to display data and send user events to the ViewModel (or Presenter).

* Don't reference any View or Activity in ViewModel, ViewModel have lager life scope than acitivities or fragments.
 
* Using a data repository
	1. Remote: network or the Cloud
	2. Local: database or file
	3. In-memory cache

* Avoid Leaking ViewModels, If a repository is a singleton and it holds ViewModel reference. than it may cause a leak. You have many options to avoid this:

	1. With ViewModel.onCleared() you can tell the repository to drop the callback to the ViewModel.
	2. In the repository you can use a WeakReference or you can use an Event Bus (both easy to misuse and even considered harmful).
	3. Use the LiveData to communicate between the Repository and ViewModel in a similar way to using LiveData between the View and the ViewModel.

## 参考

[ViewModels and LiveData: Patterns + AntiPatterns](https://medium.com/androiddevelopers/viewmodels-and-livedata-patterns-antipatterns-21efaef74a54)