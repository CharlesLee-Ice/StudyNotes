##为什么使用代理?##

*Effective Java*书中第四章讲到*继承*的弊端，并且推荐**Favor composition over inheritance**,而`Kotlin`通过`by`关键字实现代理。

###那继承有哪些弊端呢？###

最主要的有两点：父类行为的不确定性，继承影响父类的行为

例如要实现`HashSet`元素的累积添加次数：
	
	public class CountingSet extends HashSet<Long> {
	    private long addedCount = 0;
	
	    @Override
	    public boolean add(Long aLong) {
	        addedCount++;
	        return super.add(aLong);
	    }
	
	    @Override
	    public boolean addAll(Collection<? extends Long> c) {
	        addedCount = addedCount + c.size();
	        return super.addAll(c);
	    }
	
	    public long getAddedCount() {
	        return addedCount;
	    }
	}

但其实`HashSet`的`addAll`方法其实是调用自己的`add`方法逐一添加，所以这里`addedCount`计数有误，但如果通过代理的方式来实现，则无风险。

	public class CountingSet implements Set<Long> {
	
	    private final Set<Long> delegate = Sets.newHashSet();
	    private long addedCount = 0L;
	
	    @Override
	    protected Set<Long> delegate() {
	        return delegate;
	    }
	
	    @Override
	    public boolean add(Long element) {
	        addedCount++;
	        return delegate().add(element);
	    }
	
	    @Override
	    public boolean addAll(Collection<? extends Long> collection) {
	        addedCount += collection.size();
	        return delegate().addAll(collection);
	    }
	}

同理，当继承父类时，重载父类的函数，那相当于改变了父类的行为，父类自己调用会出现不可预知的问题。

###Android系统中用到代理的地方###

* `ContextWrapper`代理`Context`对象

		/**
		 * Proxying implementation of Context that simply delegates all of its calls to
		 * another Context.  Can be subclassed to modify behavior without changing
		 * the original Context.
		 */
		public class ContextWrapper extends Context {
		    Context mBase;
		
		    public ContextWrapper(Context base) {
		        mBase = base;
		    }
		    
		    @Override
		    public AssetManager getAssets() {
		        return mBase.getAssets();
		    }
		
		    @Override
		    public Resources getResources() {
		        return mBase.getResources();
		    }
		    ...
		}

	上面`ContextWrapper`的注释已经解析的很清楚了，就是避免`ContextWrapper`的子类的行为影响`Context`。
	
* 	`AppCompatActivity`代理`Activity`对象，这里具体是：`AppCompatActivity`内的`AppCompatDelegate mDelegate`代理了`Activity`的具体工作。

		public abstract class AppCompatDelegate {
		    public abstract void setContentView(@LayoutRes int resId);
		    public abstract void onCreate(Bundle savedInstanceState);
		    ...
		}

	原先该`Activity`干的活全给`AppCompatDelegate`做了。
	
	这里额外补充一点，`AppCompatDelegateImpl`通过继承`LayoutInflater.Factory2`重载`LayoutInflater`的`onCreateView`方法。从而实现不同的`View`的替换，从而做到控件向下兼容：
		
		//AppCompatViewInflater.java
		final View createView(...) {
	        final Context originalContext = context;
	        ...
	        switch (name) {
	            case "TextView":
	                view = createTextView(context, attrs);
	                verifyNotNull(view, name);
	                break;
	            case "ImageView":
	                view = createImageView(context, attrs);
	                verifyNotNull(view, name);
	                break;
	            case "Button":
	                view = createButton(context, attrs);
	                verifyNotNull(view, name);
	                break;
	            ...
	            default:
	                // The fallback that allows extending class to take over view inflation
	                // for other tags. Note that we don't check that the result is not-null.
	                // That allows the custom inflater path to fall back on the default one
	                // later in this method.
	                view = createView(context, name, attrs);
	        }
	
	        return view;
	    }
	    
##Kotlin中的代理##

`kotlin`通过`by`关键字很好的支持代理，例如：
	
	interface Base {
	    fun print()
	}
	 
	class BaseImpl(val x: Int) : Base {
	    override fun print() { print(x) }
	}
	 
	class Derived(b: Base) : Base by b
	 
	fun main(args: Array<String>) {
	    val b = BaseImpl(10)
	    Derived(b).print() // prints 10
	}

不像`ContextWrapper`的代理，需要实现`Context`函数，一个一个转移到`mBase`上去，通过`by`关键字省略了很多`boilerplate`代码。

`Kotlin`代理还有以下其它使用场景：

* 不能继承，应为`Kotlin`类是`public final`
* 只希望对外提供有限接口，避免外界`cast`

###Kotlin自带的代理用法###

* `Lazy`
* `Delegates.observable()` `Delegates.vetoable()`
* Storing Properties in a Map
		
		class User(val map: Map<String, Any?>) {
		    val name: String by map
		    val age: Int     by map
		}
		
		val user = User(mapOf(
		    "name" to "John Doe",
		    "age"  to 25
		))

###Kotlin自定义属性代理###

`val`变量的代理需要继承`ReadOnlyProperty`类，而`var`变量的代理则需要继承`ReadWriteProperty`
	
	class IntPreference(
	        private val preferences: SharedPreferences,
	        private val name: String,
	        private val defaultValue: Int = 0
	) : ReadWriteProperty<Any, Int> {
	
	    override fun getValue(thisRef: Any, property: KProperty<*>): Int {
	        return preferences.getInt(name, defaultValue)
	    }
	
	    override fun setValue(thisRef: Any, property: KProperty<*>, value: Int) {
	        preferences.edit().putInt(name, value).apply()
	    }
	}
	
	fun SharedPreferences.int(name: String) = IntPreference(this, name, 0)
	
	class PreferencesManager @Inject constructor(context: Context) {
	    var launchCount by context.getSharedPreferences("myApp", Context.MODE_PRIVATE).int("count")
	}
	
## 参考 ##

[Kotlin: Delegated Properties](https://kotlinlang.org/docs/reference/delegated-properties.html)

[Kotlin “By” Class Delegation: Favor Composition Over Inheritance](https://medium.com/rocket-fuel/kotlin-by-class-delegation-favor-composition-over-inheritance-a1b97fecd839)

[Kotlin ‘By’ Property Delegation: Create Reusable Code](https://medium.com/rocket-fuel/kotlin-by-property-delegation-create-reusable-code-f2bc2253e227)