## Fragment生命周期

<img src="fragment_lifecycle.png">

#### Fragment在一个Activity内切换
	public void onClick(View v) {  
	        FragmentTransaction fragmentTransaction = fragmentManager.beginTransaction();  
	        switch (v.getId()) {  
	            case R.id.button1:  
	                if (fragOne == null) {  
	                    fragOne = new FragOne();  
	                    fragmentTransaction.replace(R.id.frag_container, fragOne, fragNames[0]);  
	//                    fragmentTransaction.addToBackStack(fragNames[0]);  
	                } else {  
	                    Fragment fragment = fragmentManager.findFragmentByTag(fragNames[0]);  
	                    fragmentTransaction.replace(R.id.frag_container, fragment, fragNames[0]);  
	                }  
	                break;  
	            case R.id.button2:  
	                if (fragTwo == null) {  
	                    fragTwo = new FragTwo();  
	                    fragmentTransaction.replace(R.id.frag_container, fragTwo, fragNames[1]);  
	//                    fragmentTransaction.addToBackStack(fragNames[1]);  
	                } else {  
	                    Fragment fragment = fragmentManager.findFragmentByTag(fragNames[1]);  
	                    fragmentTransaction.replace(R.id.frag_container, fragment, fragNames[1]);  
	                }  
	                break;  
	            default:  
	                break;  
	        }  
	        fragmentTransaction.commit();  
	    }
	}
上面将addToBackStack代码屏蔽：
	fragmentOne:onPause
	fragmentOne:onStop
	fragmentOne:onDestoryView
	fragmentOne:onDestory
	fragmentOne:onDetach
	
	fragmentTwo:onAttach
	fragmentTwo:onCreate
	fragmentTwo:onCreateView
	fragmentTwo:onActivityCreated
	fragmentTwo:onStart
	fragmentTwo:onResume

当addToBackStack代码打开
	fragmentOne:onPause
	fragmentOne:onStop
	fragmentOne:onDestoryView
	
	fragmentTwo:onAttach
	fragmentTwo:onCreate
	fragmentTwo:onCreateView
	fragmentTwo:onActivityCreated
	fragmentTwo:onStart
	fragmentTwo:onResume

点击按钮button1，回到FragmentOne
	fragmentTwo:onPause
	fragmentTwo:onStop
	fragmentTwo:onDestoryView
	
	fragmentOne:onCreateView
	fragmentOne:onActivityCreated
	fragmentOne:onStart
	fragmentOne:onResume

#### add() hide() show()对Fragment生命周期不影响

## View的绘制 ##

### onMeasure ###

* `View`的`measure` 方法是`final`的，`measure`调用的`onMeasure`方法可以被重载， 比如`LinearLayout`的具体测量方法就是在`onMeasure`里面的：
	
	    @Override
	    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
	        if (mOrientation == VERTICAL) {
	            measureVertical(widthMeasureSpec, heightMeasureSpec);
	        } else {
	            measureHorizontal(widthMeasureSpec, heightMeasureSpec);
	        }
	    }

* `View`的`match_parent` `wrap_content` `100dp`等宽高设置会转换成`measure`方法时的宽高参数`widthMeasureSpec` `heightMeasureSpec`, ``

### requestLayout & invalidate & postInvalidate ###

#### requestLayout ####

子View调用`requestLayout`方法，会标记`PFLAG_FORCE_LAYOUT`给当前View及父容器，父容器再次调用`requestLayout`,标记同时逐层向上提交，直到ViewRootImpl处理该事件，ViewRootImpl会调用三大流程，从`measure`开始，对于每一个含有标记位的view及其子View都会进行测量、布局、绘制，只有调用`requestLayout`的View和它的直系上级们会重新走测量、布局、绘制流程，旁系兄弟姐妹们是不用走这些流程。

### invalidate ###

子View调用了`invalidate`方法后，会为该View添加一个标记位`PFLAG_DRAWN`，同时不断向父容器请求刷新，父容器通过计算得出自身需要重绘的区域，直到传递到ViewRootImpl中，最终触发`performTraversals`方法，进行开始View树重绘流程(只绘制需要重绘的视图)。由于没有添加`measure`和`layout`的标记位，因此`measure`、`layout`流程不会执行，而是直接从draw流程开始。


### postInvalidate ###

`postInvalidate`是在非UI线程中调用，`invalidate`则是在UI线程中调用。 通过`Handler`向`ViewRootImpl`中发送`INVALIDATE`消息，触发`view`的`invalidate`函数

## Serializable VS Parcelable ##

* `Parcelable` 运行的过程比 `Serializable` 速度要快，因为不涉及到反射。
* `Serializable` 过程耗内存
* `Serializable` 代码写起来更方便
* 网络、IO传输用`Serializable`，方便版本控制，本地IO通过`serialVersionUID`判断是否能序列化，网路通过`Json`字符串来反射序列化和反序列化数据，可以实现不对等对象的传递
* 内部中传递数据用`Parcelable`，速度更快，省内存，传递双方结构必须一致。

## XML parse ##

* XML: Extensible Markup Language
* XML is a popular format for sharing data on the internet
* Android 推荐使用 `XmlPullParser`
* XML解析方式有三种：XML DOM (Document Object Model)、SAX(Simple API for XML)、PULL
* DOM方式是将XML加载进一个Object，然后通过方法和属性来访问Object，占用内存多，但是xml内容能交错访问
* SAX是流式解析，通过`tag (<something> </something>)`触发相应的事件，从而读取内容
* PULL也是流式解析，但不同于SAX需要解析整个XML，PULL可以只解析某一个节点，PULL可以提供更细致的控制

## Activity和Fragment 生命周期问题 ##

当configuration变化时，`Activity``Fragment`会重新走`onCreate`流程，那`Activity`和`Fragment`还是原来的对象吗？

答：不是，configuration变化后，重新`new`Activity和Fragment对象。需要注意`Activity`的`onCreate(@Nullable Bundle savedInstanceState)`中参数`savedInstanceState`在第二个创建时，不为`null`。我们只需要处理`Activity``Fragment`中的`onSaveInstanceState`即可。所以为了实现configuration变化时，数据保留，采取`Retain Fragment`方法，使用最新的`ViewModel`也可以，`ViewModel`内部其实也是通过`Retain Fragment`实现。

当内存不足时，回收`Activity``Fragment`时,那`Retain Fragment`还能保留吗？

答：不一定，系统可能只回收了UI相关的`Activity`,没有回收`Retain Fragment`。

举例：

	public class MainActivity extends AppCompatActivity {
	    private static final String TAG_TASK_FRAGMENT = "task_fragment";
	
	    int a = 0;
	
	    private TaskFragment mTaskFragment;
	    boolean newProductFragment = false;
	    boolean newTaskFragment = false;
	
	    @Override
	    protected void onCreate(@Nullable Bundle savedInstanceState) {
	        super.onCreate(savedInstanceState);
	        setContentView(R.layout.main_activity);
	
	        // Add product list fragment if this is first creation
	        if (savedInstanceState == null) {
	            newProductFragment = true;
	
	            ProductListFragment fragment = new ProductListFragment();
	
	            getSupportFragmentManager().beginTransaction()
	                    .add(R.id.fragment_container, fragment, ProductListFragment.TAG).commit();
	        }
	
	        FragmentManager fm = getSupportFragmentManager();
	        mTaskFragment = (TaskFragment) fm.findFragmentByTag(TAG_TASK_FRAGMENT);
	
	        // If the Fragment is non-null, then it is currently being
	        // retained across a configuration change.
	        if (mTaskFragment == null) {
	            newTaskFragment = true;
	            mTaskFragment = new TaskFragment();
	            fm.beginTransaction().add(mTaskFragment, TAG_TASK_FRAGMENT).commit();
	        }
	
	        (new Handler()).post(() -> {
	            a++;
	            ProductListFragment fragment = (ProductListFragment)getSupportFragmentManager().findFragmentByTag(ProductListFragment.TAG);
	            fragment.b++;
	            mTaskFragment.c++;
	            Log.i("Test", " "+a+" "+fragment.b+" "+mTaskFragment.c+" "+newTaskFragment+" "+newProductFragment);
	        });
	    }
	
	}
	
	public class TaskFragment extends Fragment {
	    int c = 0;
	
	    @Override
	    public void onCreate(Bundle savedInstanceState) {
	        super.onCreate(savedInstanceState);
	
	        // Retain this fragment across configuration changes.
	        setRetainInstance(true);
	    }
	}
	
启动`MainActivity`后，切换到横屏，打印日志如下：

	Test:  1 1 1 true true // 第一次启动
	Test:  1 1 2 false false // 手机横屏，configuration变化
	
此时切换后台，打开设置页开发者模式中的“不保留互动”，切回到应用，打印日志如下：

	Test:  1 1 1 false false // Retain Fragment也被回收了
	
	
## 权限 ##

`READ_EXTERNAL_STORAGE `
`WRITE_EXTERNAL_STORAGE`

Starting in API level 19, this permission is not required to read/write files in your application-specific directories returned by Context.getExternalFilesDir(String) and Context.getExternalCacheDir()

但是外部存储公用部分还是需要权限申请的，例如存储图片到系统共用图片库：
	
	fun getPublicAlbumStorageDir(albumName: String): File? {
	    // Get the directory for the user's public pictures directory.
	    val file = File(Environment.getExternalStoragePublicDirectory(
	            Environment.DIRECTORY_PICTURES), albumName)
	    if (!file?.mkdirs()) {
	        Log.e(LOG_TAG, "Directory not created")
	    }
	    return file
	}