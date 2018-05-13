##透明状态栏
* 4.4以下系统无法实现，为黑色
* 4.4通过设置样式中windowTranslucentStatus为true实现，但记得要在根layout布局中设置android:fitsSystemWindows="true" 来避免布局和statusbar重叠
* 5.0以上系统有下面几个方法

#####方法1
同4.4设置，但是5.0以上会添加一层黑色透明。

#####方法2

	<item name="android:windowTranslucentStatus">false</item>
	<item name="android:windowTranslucentNavigation">true</item>
	<item name="android:statusBarColor">@android:color/transparent</item>
 
这里windowTranslucentStatus设置false去除黑色透明条，windowTranslucentNavigation设置true，系统会设置`SYSTEM_UI_FLAG_LAYOUT_STABLE`和`SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN`属性，同时设置statusBarColor让statusbar透到自己的layout上去。

windowTranslucentNavigation设置true后有个缺点：bottom sheet弹窗时，navigation会透明。
#####方法3

	<item name="android:statusBarColor">@android:color/transparent</item>
	//getWindow().setStatusBarColor(Color.TRANSPARENT);
	
	getWindow().getDecorView().setSystemUiVisibility(
	    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
	    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);  
		    
此时同样需要注意设置fitsSystemWindows为true, 或者代码中添加一个view，如下：
	
    StatusBarView statusBarView = new StatusBarView(activity);
    LinearLayout.LayoutParams params =
            new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, getStatusBarHeight(activity));
    statusBarView.setLayoutParams(params);
    statusBarView.setBackgroundColor(Color.argb(alpha, 0, 0, 0));
    statusBarView.setId(FAKE_TRANSLUCENT_VIEW_ID);
    
    ViewGroup contentView = (ViewGroup) activity.findViewById(android.R.id.content);
    contentView.addView(statusBarView.setId);

#####方法4
如果只是想设置一个图片在statusbar后面，而不是设置一个view，则可以设置activity级别的背景
        
    <item name="android:statusBarColor">@android:color/transparent</item>
    <item name="android:background">@color/colorAccent</item>

##### PS
* 有些布局如DrawerLayout或CoordinatorLayout只要设置android:statusBarColor透明即可达到透明布局效果
* 导航栏实现透明效果设置windowTranslucentNavigation
`<item name="android:windowTranslucentNavigation">true</item>`
* windowDrawsSystemBarBackgrounds这个属性默认为true，颜色为colorPrimaryDark,如果改成false，则状态栏变黑

####参考
* [lollipop-draw-behind-statusbar-with-its-color-set-to-transparent](http://stackoverflow.com/questions/27856603/lollipop-draw-behind-statusbar-with-its-color-set-to-transparent)

##全屏布局
##### 方法1
    requestWindowFeature(Window.FEATURE_NO_TITLE);
    getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
##### 方法2
	android:theme="@android:style/Theme.NoTitleBar.Fullscreen"
	
##沉浸式布局
####Leanback模式
* enableFullScreen为true，则进入全屏模式，全屏模式下，点击屏幕出现系统UI，点击事件被吸收，应用接收不到此点击，再次点击，应用正常接收。
* enableFullScreen为false，则进入全局布局，即view的布局是以整个屏幕为基准，status bar和navigation bar会盖在应用布局之上。
* `SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN`表示layout以fullscreen为基准（不考虑statusbar的padding）, `SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION`表示以navigation bar为隐藏的状态下进行布局（不改变navigation bar的隐藏或未隐藏状态）。
* `SYSTEM_UI_FLAG_LAYOUT_STABLE`标志设置可以让布局平稳的过渡（`SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN` 到 `SYSTEM_UI_FLAG_FULLSCREEN`或者`SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION`到`SYSTEM_UI_FLAG_HIDE_NAVIGATION`), 通常对系统view有效（ActionBar）。
* 应用可通过setOnSystemUiVisibilityChangeListener监听UI变化
* 此模式适用播放器等应用（全屏时点击，显示系统UI，但是不进行实际操作）

```
private void enableFullScreen(boolean enabled) {
    int newVisibility =  View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN;

    if(enabled) {
        newVisibility |= View.SYSTEM_UI_FLAG_FULLSCREEN
                |  View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
    }

    getDecorView().setSystemUiVisibility(newVisibility);
}
```
####Immersive模式
* Android 4.4以上有效
* 屏幕边缘滑动会显示系统UI，并且应用同时能接收点击事件
* 应用第一次进入Immersive模式，会有系统引导，提醒用户屏幕上边缘下滑可以出现status bar（我没遇到过）
* 应用可通过setOnSystemUiVisibilityChangeListener监听UI变化
* 适用阅读器等应用（全屏时点击，应用可接收到此事件）

```
protected void enableFullScreen(boolean enabled) {
    int newVisibility =  View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN;

    if(enabled) {
        newVisibility |= View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_IMMERSIVE;
    }

    getDecorView().setSystemUiVisibility(newVisibility);
}
```   

####Immersive Sticky模式
* 进入Immersive Sticky模式后，屏幕边缘滑动可展示系统UI，过几秒后自动消失
* 应用无法接收到系统UI变化通知，系统UI布局其实未发生变化，所以不通知
* 适用游戏和绘画等应用

####沉浸式布局注意点
* 实现onWindowFocusChanged()函数，获取焦点时，可以进入沉浸模式；焦点丢失时（弹出dialog或menu）,则退出沉浸模式，并取消沉浸模式下的业务
* 通过GestureDetector的onSingleTapUp(MotionEvent)方法，可以允许用户点击屏幕即可展现或隐藏系统UI，简单的点击事件并不能完美处理，因为点击事件并不能响应跨越屏幕边缘的滑动。

####参考链接
* [Bitesize Android KitKat: Week 1: Full Screen Apps](https://www.shinobicontrols.com/blog/bitesize-android-kitkat-week-1-full-screen-apps)
* [Using Immersive Full-Screen Mode](https://developer.android.com/training/system-ui/immersive.html)
