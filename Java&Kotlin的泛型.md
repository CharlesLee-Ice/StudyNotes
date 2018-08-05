## Java 和 Kotlin 的泛型(Generics) ##

### 泛型（Generics） ###

*什么是泛型？*

举例来说：

Java:

	public interface List<E> extends Collection<E> {
		...
	}
	

`List<E>` 称为泛型，`E`称为类型参数（Type Parameter）

Kotlin:

	public interface List<out E> : Collection<E> {
		...
	}

*为什么用泛型？*

泛型提供编译时类型检查，避免运行时`ClassCastException `异常。

举例来说：
	
	List list = new ArrayList();
	list.add("abc");
	list.add(new Integer(5)); //OK
	
	for(Object obj : list){
		//type casting leading to ClassCastException at runtime
	    String str=(String) obj; 
	}
	
上面代码在编译时正确，运行时却会抛出`ClassCastException `异常。

### 受限类型参数（Bounded Type Parameters）###

#### 上限类型参数 ####

Java:

	public static <T extends Comparable<T>> int compare(T t1, T t2) {
		return t1.compareTo(t2);
	}

Kotlin:
	
	fun <T> copyWhenGreater(list: List<T>, threshold: T): List<String>
	        where T : CharSequence,
	              T : Comparable<T> {
	    return list.filter { it > threshold }.map { it.toString() }
	}

上限类型参数有3点特征：

* 实例化限制（Restricted Instantiation），实例化类型参数时，编译器会检查类型是否符合限制范围；

* 获取非静态方法（Access To Non-Static Members），虽然不知道具体的类型，但是可以调用上限类型的非静态方法；
	
		class Box<T extends Number> {  
		   private T value;  
		   public Box(T t) { value = t; }  
		   public int increment() { return value.intValue()+1; } // <= would be an error without the Number bound 
		   ... 
		}

* 类型清除（Type Erasure），泛型通过编译时的类型检查，避免了运行时的类强转错误`ClassCastException `，编译通过后，程序运行的字节码已经没有泛型存在的必要了，所以编译器通过类型清除（Type Erasure）去除泛型，用边界类型或`Object`代替，相应的地方添加上类型强转。如果泛型定义时，没有边界，则类型变成`Object`，所以`List<String> one`和`List<Integer> two`其实在字节码是同一种`List<Object>`。在定义有上下限的泛型编译后，如上例，所有的类型`T`都被替换成了上限`Number`,外界传入的类型变量`Integer a`强制转换成`Number`类型，如下面代码所示：
	
		class Box {  
		   private Number value;  
		   public Box( Number t) { value = t; }  
		   ... 
		}
		
		Integer a = 10;
		Box box = new Box((Number)a);
		
***Why don't Java Generics support primitive types?***
	
	List<int> bar = new ArrayList<int>();// 编译报错

还是和类型擦除有关，编译后的代码，泛型都被替换成了`Object`或其他边界类，原始类型没办法强转成`Object`。

**受限类型参数只有上限(upper bound)，没有下限(lower bound)**
	
	class Box< T super Number >// compile error
	
下限类型参数没有意义，只有上面“实例化限制（Restricted Instantiation)”这个效果，达不到其它两点效果。

### 类和子类型 (Classes & Subtyping) ###

Java通过 `extends` 和 `implements` 实现 子类型转换（subtyping），可以认为 `List<String>` 是 `Collection<String>` 的子类型，但是 `List<String>` 确不是 `List<Object>` 的子类型。

*为什么Java 限制 `List<String>` 不能是 `List<Object>` 的子类型？*

答：为了安全，如下例所示：

	// Java
	List<String> strs = new ArrayList<String>();
	List<Object> objs = strs; // !!! The cause of the upcoming problem sits here. Java prohibits this!
	objs.add(1); // Here we put an Integer into a list of Strings
	String s = strs.get(0); // !!! ClassCastException: Cannot cast Integer to String
	
这些子类型的限制导致了如下使用不便：
	
	// Java
	interface Collection<E> ... {
	  void addAll(Collection<E> items);
	}
	
	void copyAll(Collection<Object> to, Collection<String> from) {
	  to.addAll(from); // !!! Would not compile with the naive declaration of addAll:
	                   //  Collection<String> is not a subtype of Collection<Object>
	}
	
这里类型转换被限制了，称为 **不可变（Invariance）**，Java为了解决这个问题，引入通配符概念（Wildcards）

### 通配符(Wildcards) ###

通配符 `?` 代表了一种未知类型，通配符包含：上边界通配符、无边界通配符、下边界通配符。

#### 上边界通配符（Upper Bounded Wildcard） ####

通过上边界通配符解决上面例子中的使用不便：
	
	interface Collection<E> ... {
		boolean addAll(Collection<? extends E> c);
	}
	
	void copyAll(Collection<Object> to, Collection<String> from) {
	  to.addAll(from);
	}

通过上边界通配符，实现子类型转换(Subtyping), 上例中`Collection<String>`是`Collection<？extends Object>`的子类型，这里的子类型转换称之为 **协变（Covariance）**

上边界通配符定义的变量，因为不知道具体的类型，所以不能向对象中添加数据，上例中的`addAll`函数不能向参数`Collection<? extends E> c` 中添加数据（添加数据时，编译器会报错），但是可以通过 `Collection<? extends E> c`获取数据，即调用`get`方法。 一句话总结：上边界通配符只能读数据。

#### 无边界通配符（Unbounded Wildcard） ####
	
	public static void printData(List<?> list){
		for(Object obj : list){
			System.out.print(obj + "::");
		}
	}
	
无边界通配符可以被类型参数（Type Parameter）代替，无边界通配符同样不知道具体数据类型，不可以添加数据。

#### 下边界通配符（Lower bounded Wildcard） ####

	public static void addIntegers(List<? super Integer> list){
		list.add(new Integer(50));
	}
	
	List<Number> numbers = new ArrayList<>();
	addIntegers(numbers)
	
上例中, 可以传入类型 `List<Number>` 实现子类型转换(Subtyping), `List<Number>` 是 `List<? super Integer>` 的子类型，这里的子类型转换称之为 **逆变（Contravariance）**。

下边界通配符定义的对象，由于知道类型的下限，可以向对象中写入数据，上例中可以向 `List<? super Integer> list` 对象添加新的`Integer`。但是不知道下边界通配符的具体类型及类型内方法，所以不能读数据，即不能调用 `List<? super Integer> list`对象的`get`方法。一句话总结：下边界通配符只能写数据。


### Kotlin: in, out ###

上面子类型的例子中，Java为了安全，限制了 `List<String>` 到 `List<Object>` 的子类型转换，但假如 `List<E>` 类中没有任何写入数据的接口时，子类型转换的限制就没有必要了。例如：

	// Java
	interface Source<T> {
	  T nextT();
	}
	
	void demo(Source<String> strs) {
	  Source<Object> objects = strs; // !!! Not allowed in Java
	  // ...
	}
	
上例中，`Source<T>`类只有一个读数据的方法，子类型转换：`Source<Object> objects = strs;`是没有任何风险的，后面代码获取到`objects`对象是没有方法向对象中填入数据的，但Java编译器仍然禁止这种写法，为了能够实现转换，必须要写成 `Source<? extends Object> objects = strs;`。 这样写后，`Source<? extends Object> objects ` 和 `Source<Object> objects` 对后面代码来说是完全一致的，所以Java 通配符显得没有意义，因此Kotlin采用**声明处类型变异(declaration-site variance)**` 来简洁的解决这种问题。
	
	interface Source<out T> {
	    fun nextT(): T
	}
	
	fun demo(strs: Source<String>) {
	    val objects: Source<Any> = strs // This is OK, since T is an out-parameter
	    // ...
	}

通过定义类型参数`T`的注释`out`，实现`Source<String>` 是 `Source<Any>` 的子类型，这里称之为**协变**。`out`定义后，编译器检查`Source`类中函数只能返回类型参数`T`，即可以“读”，不能传入类型参数`T`，即不能“写”，此时`Source`可以称之为生产类。

除了`out`，kotlin还提供注释`in`，如下例中：
	
	interface Comparable<in T> {
	    operator fun compareTo(other: T): Int
	}
	
	fun demo(x: Comparable<Number>) {
	    x.compareTo(1.0) // 1.0 has type Double, which is a subtype of Number
	    // Thus, we can assign x to a variable of type Comparable<Double>
	    val y: Comparable<Double> = x // OK!
	}
 
 注释`in`实现`Comparable<Number>` 是 `Comparable<Double>` 的子类型，称之为**逆变**。`in`定义后，编译器检查`Comparable`类中的函数只能传入类型参数`T`，即可以“写”，不能返回类型参数`T`，即不能“读”，此时`Comparable`可以称之为消费类。
 
 Kotlin的`in``out`一句话总结：**Consumer in, Producer out!**
  
## 参考链接 ##
[Java Generics Example Tutorial – Generic Method, Class, Interface](https://www.journaldev.com/1663/java-generics-example-method-class-interface)

[Lesson: Generics](https://docs.oracle.com/javase/tutorial/extra/generics/index.html)

[Why is there no lower bound for type parameters?](http://www.angelikalanger.com/GenericsFAQ/FAQSections/TypeParameters.html#FAQ107)

[Kotlin Generics: in out](https://kotlinlang.org/docs/reference/generics.html)

[Understanding Generics and Variance in Kotlin](https://proandroiddev.com/understanding-generics-and-variance-in-kotlin-714c14564c47)