# Android targetSdkVersion 26影响 #

## targetSdkVersion 24 影响

### 系统Doze模式和App Standby模式 ###

**Doze 和 Standby出发点是为了节省用电**

* 该特性不受`targetSdkVersion`影响，在Android 6.0(api 23)以上手机内的应用都受影响
* 该模式自Android 6.0版本就开始运作，但只限于静止不动才会生效（Doze），Android 7.0后更进一步，移动状态也适用（Lightweight Doze）
* 在`Doze`模式下，`wakelock``network`无法使用，`alarm``jobscheduler``syncadapter`推迟到活动窗口，`wifi`扫描停止
* `Doze`模式下不管Java层还是Native层，网络都不可用，所以长连接应用肯定是要断开的，`Doze`模式国内ROM也适用，长连接应用只能依赖`FCM`和国内各厂商推送

#### 具体运行状态 ####

* `Doze`模式下应用活动窗口的时间最长运行只有10分钟，窗口期间隔也会逐渐拉长，第一次窗口需要等1个小时，第二次窗口等2个小时，第三次就可能4个小时
* 窗口期的`Alarm`只允许运行运行10s
* `Standby`状态下应用一天可能只有一次活动窗口，FCM紧急消息的个数也有限制（10/day ～ 5/day）

[Doze模式过渡细节](https://source.android.com/devices/tech/power/platform_mgmt#doze)

[具体限制细节](https://developer.android.com/topic/performance/power/power-details)

#### 应用如何豁免Doze和Standy模式 ####
* `AlarmManager`可以使用`setAndAllowWhileIdle()``setExactAndAllowWhileIdle()``setAlarmClock()`在运行期间唤醒，应用需谨慎使用，应用要承担`Doze`模式退出所带来的系统电量消耗
* 应用选择发送`ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS``Intent`把用户定位设置页面，让用户设置，这种方式对用户不太友好，因为用户可能看不懂设置页的选择。
* 应用在`AndroidManifest.xml`中声明`REQUEST_IGNORE_BATTERY_OPTIMIZATIONS`权限或者应用发送`ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS``Intent`触发应用内系统弹框，让用户打开`Doze`限制，这种方式对用户较友好
* Google Play Store严格控制逃离`Doze`模式的应用，如果没有合理理由，会影响应用上架，国内微信、QQ都没有选择逃离`Doze`模式

[doze-standby](https://developer.android.com/training/monitoring-device-state/doze-standby)

### 部分广播限制 ###

`CONNECTIVITY_ACTION` 广播必须动态注册才能接收，`AndroidManifest.xml`中静态注册无效，Android 9.0(API 28)以后，该广播被弃用，推荐用户使用`ConnectivityManager.requestNetwork`或者使用`JobScheduler`

`ACTION_NEW_PICTURE``ACTION_NEW_VIDEO` 这两个广播Android 7.0（API 24）系统不发送，使用`JobInfo.Builder.addTriggerContentUri(JobInfo.TriggerContentUri)`代替，而Android 8.0(API 26)之后系统又发送了，但是只能动态注册才能接收到，这里可以看到广播这块在api 26才做彻底。

### 文件分享权限限制 ###
 
使用`FileProvider`来分享文件给其他应用
 
### NDK 私有库限制 ###
 
### 分屏和画中画 ###

## targetSdkVersion 26 影响 ##

**后台`Service`限制和`Broadcast`收窄是为了性能，之前一发广播，唤醒一堆后台应用，系统忙于清理内存，性能损耗严重，系统清理完内存，一个广播就再次破坏**

### 后台Service限制 ###

* 前台`Service`或者`Service`被其他前台进程给`bind`使用，系统不限制此类`Service`
* 后台`Service`会被系统强行`stopSelf()`，后台`startService`也会抛异常
* 应用刚从前台切到后台有几分钟的缓冲，此缓冲期内可以继续运行`Service`，应用收到FCM消息、`broadcast`或`PendingIntent`也会有几分钟的缓冲，不过还是不推荐继续用`Service`，推荐用`JobIntentService`和`JobScheduler`
* `Application`初始化时`startService`也就没意义，后台启动`Application`后，`Service`会被马上停止。
* Android8.0 前台`Service`启动需要调用`startForegroundService()`函数，并且告诉系统`startForeground()`，否则系统也会杀

### 广播限制 ###

* `explicit broadcast`就是指定接受对象的广播，例如: `Intent(Context packageContext, Class<?> cls)` `setComponent(@Nullable ComponentName component)`
* `implicit broadcast`就是只指定`action`的广播，例如：`Intent(String action)`，不管这个`action`是否是自定义的
* 在`AndroidManifest.xml`中声明的`Receiver`只能接收`explicit broadcast`，不能接收`implicit broadcast`，两种情况除外：
	1. 系统部分豁免的广播：[broadcast-exceptions](https://developer.android.com/guide/components/broadcast-exceptions)
	2. `Receiver`加了广播签名权限
* 动态注册的广播接收器能接收所有广播

### 后台定位通知频率降低 ###
不受`targetSdkVersion`影响

### Alert windows 类型变更 ###

### App快捷方式 ###