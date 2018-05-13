## Toolbar

	<android.support.design.widget.AppBarLayout
	    xmlns:android="http://schemas.android.com/apk/res/android"
	    xmlns:app="http://schemas.android.com/apk/res-auto"
	    android:layout_height="?attr/actionBarSize"
	    android:layout_width="match_parent"
	    android:theme="@style/ThemeOverlay.AppCompat.Dark.ActionBar">
	    <android.support.v7.widget.Toolbar
	        android:id="@+id/toolbar"
	        android:layout_height="match_parent"
	        android:layout_width="match_parent"
	        android:background="?attr/colorPrimary"
	        app:popupTheme="@style/ThemeOverlay.AppCompat.Light"
	        app:layout_scrollFlags="scroll|enterAlways"/>
	</android.support.design.widget.AppBarLayout>

##### 1. 为什么Toolbar或者外面包裹的AppBarLayout需要设置主题：android:theme="@style/ThemeOverlay.AppCompat.Dark.ActionBar"？ 
因为一般应用的主题为Theme.AppCompat.Light.NoActionBar，Light主题为白底黑字，所以Toolbar的标题为黑色，给Toolbar加上Dark主题后，则标题变为白色。

##### 2. 为什么Toolbar设置app:popupTheme="@style/ThemeOverlay.AppCompat.Light"
因为Toolbar主题为Light，而希望右边菜单弹出的样式为白底黑字，所以设置popupTheme为Light

##### 3. Toolbar在DrawerLayout和CoordinatorLayout布局中需要添加AppBarLayout的包裹，否则显示异常

##### 4. setSupportActionBar(mToolbar) 设置后立刻调用mToolbar.setTitle("Hello");不生效，需要在setSupportActionBar之前调用或者post执行setTitle,或者如下

        mToolbar = (Toolbar)findViewById(R.id.toolbar);
        setSupportActionBar(mToolbar);

        ActionBar actionBar = getSupportActionBar();
        actionBar.setTitle("Hello");
        
##### 5. setDisplayHomeAsUpEnabled 是 setHomeButtonEnabled 的升级版，不仅左上角图标可点，而且默认添加向左的箭头，如果想要换图片，调用setHomeAsUpIndicator。setDisplayShowHomeEnabled表示左上角应用图标可见，默认true，图标可通过setLogo、setIcon替换

