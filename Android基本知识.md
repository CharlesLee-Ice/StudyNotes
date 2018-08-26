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

子View调用requestLayout方法，会标记当前View及父容器，同时逐层向上提交，直到ViewRootImpl处理该事件，ViewRootImpl会调用三大流程，从measure开始，对于每一个含有标记位的view及其子View都会进行测量、布局、绘制。

### invalidate ###

子View调用了invalidate方法后，会为该View添加一个标记位，同时不断向父容器请求刷新，父容器通过计算得出自身需要重绘的区域，直到传递到ViewRootImpl中，最终触发performTraversals方法，进行开始View树重绘流程(只绘制需要重绘的视图)。


### postInvalidate ###

postInvalidate是在非UI线程中调用，invalidate则是在UI线程中调用。 通过`Handler`向`ViewRootImpl`中发送`INVALIDATE`消息，触发`view`的`invalidate`函数

## Serializable VS Parcelable ##

* `Parcelable` 运行的过程比 `Serializable` 速度要快，因为不涉及到反射。
* `Serializable` 过程耗内存
* `Serializable` 代码写起来更方便
* 网络、IO传输用`Serializable`，方便版本控制，IO通过`serialVersionUID`判断是否能序列化，网路通过`Json`字符串来反射序列化和反序列化数据，可以实现不对等对象的传递
* 程序内部传递数据用`Parcelable`，速度更快，省内存，传递双方结构必须一致。

## XML parse ##

* XML: Extensible Markup Language
* XML is a popular format for sharing data on the internet
* Android 推荐使用 `XmlPullParser`
* XML解析方式有三种：XML DOM (Document Object Model)、SAX(Simple API for XML)、PULL
* DOM方式是将XML加载进一个Object，然后通过方法和属性来访问Object，占用内存多，但是xml内容能交错访问
* SAX是流式解析，通过`tag (<something> </something>)`触发相应的事件，从而读取内容
* PULL也是流式解析，但不同于SAX需要解析整个XML，PULL可以只解析某一个节点，PULL可以提供更细致的控制