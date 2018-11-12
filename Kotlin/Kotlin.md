# Kotlin读书笔记 #

## Classes and Objects ##

### Constructor ###
类构造函数分为一个 primary constructor 和多个 secondary constructors.

**If the class has a primary constructor, each secondary constructor needs to delegate to the primary constructor, either directly or indirectly through another secondary constructor(s). Delegation to another constructor of the same class is done using the this keyword:**
	
	class Person(val name: String) {
	    constructor(name: String, parent: Person) : this(name) {
	        parent.children.add(this)
	    }
	}

`init {}` 函数是 primary constructor 的一部分，所以 secondary constructor 执行前，先执行 init 函数

### Visiblity Modifiers ###

Java 中一个外部类只能有两种修饰符，一个 `public`，一个默认修饰符（同一个package下可见），没有 `protected` 和 `private`，因为外部类定义就是拿来给外界用的。
Kotlin 中 函数、类、接口（interface）、对象、变量都可以在文件中定义，所以 `private` 修饰符有效，表明属于文件私有，`protected` 修饰符无效， 

Kotlin 新增修饰符 internal： **it is visible everywhere in the same module**

module为：

* an IntelliJ IDEA module;
* a Maven project;

**NOTE for Java users: outer class does not see private members of its inner classes in Kotlin.**

### Extension ###

扩展只是一种写法，不是真的把函数和对象插到类中去，所以没有 `backing field`，不能执行 `initializer`

	val Foo.bar = 1 // error: initializers are not allowed for extension properties

### Generics ###

**The Existential Transformation: Consumer in, Producer out!**

[Variance](https://proandroiddev.com/understanding-generics-and-variance-in-kotlin-714c14564c47)

### Nested and Inner Classes ###

* 内部类只有添加 `inner` 修饰符才能持有外部类的对象引用，才能访问外部类的方法
* 和Java一样，也是先有外部类对象，才能有内部类对象
* 匿名内部类通过object来定义

		window.addMouseListener(object: MouseAdapter() {
		    override fun mouseClicked(e: MouseEvent) {
		        // ...
		    }
		                                                                                                            
		    override fun mouseEntered(e: MouseEvent) {
		        // ...
		    }
		})


### Object ###

#### Object Expressions ####

**Object expression is a structure that creates a single instance of an object**
	
	val coords = object {
	    var x = 10
	    var y = 10
	}


和Java匿名内部类同样场景使用
	
	open class A(x: Int) {
	    public open val y: Int = x
	}
	
	interface B {...}
	
	val ab: A = object : A(1), B {
	    override val y = 15
	}

和 Java 匿名内部类不同的是，kotlin object 对象可以不通过定义 `final` 修饰符直接访问外界变量

#### Object Decalrations ####

**Object declaration is an implementation of the singleton pattern.**

* object声明的单例类是线程安全的
* 在一个类内部的object声明可以加上companion(该单例类的名字叫Companion, 已省略掉)，该object单例（Companion静态类的静态对象）内部的函数，就相当于外部类的静态函数
	
		class MyClass {
		    companion object Factory {
		        fun create(): MyClass = MyClass()
		    }
		}
		val instance = MyClass.create()
		val instance2 = MyClass.Companion.create()
		//Factory是单例类，但是MyClass不是单例类
	
		class Animal {
		    fun eat() { println("eat") }
		
		    companion object {
		        fun newAnimal() : Animal = Animal()
		    }
		}
		//Animal 类对应的.class文件
		public final class Animal {
		   public static final Animal.Companion Companion = new Animal.Companion((DefaultConstructorMarker)null);
		
		   public final void eat() {
		      String var1 = "eat";
		      System.out.println(var1);
		   }
		
		   public static final class Companion {
		      @NotNull
		      public final Animal newAnimal() {
		         return new Animal();
		      }
		
		      private Companion() {
		      }
		
		      public Companion(DefaultConstructorMarker $constructor_marker) {
		         this();
		      }
		   }
		}
* 由于是静态类，companion objec 定义的类内部不能访问外部类的方法
* object declaration 是 lazy 模式

#### Enum Classes ####
	
	enum class ProtocolState {
	    WAITING {
	        override fun signal() = TALKING
	    },
	
	    TALKING {
	        override fun signal() = WAITING
	    };
	
	    abstract fun signal(): ProtocolState
	}

每一个枚举常量值都是枚举类的一个对象，上面代码中的枚举值实现自己的匿名类，匿名类继承自 `ProtocolState`

每一个枚举常量都有两个成员属性：
	
	val name: String
	val ordinal: Int

### Delegate Properties ###

#### Lazy ####
lazy模式其实使用一个 SynchronizedLazyImpl : Lazy 类来代理属性的访问

	public fun <T> lazy(initializer: () -> T): Lazy<T> = SynchronizedLazyImpl(initializer)

* lazy模式是线程安全的
* 判断一个属性是否走过`initializer`函数
	
	    val hello = lazy { 1 }
	    if (hello.isInitialized()) {}


## Functions and Lambdas ##

### Lambdas ###


## Others ##

### Equality ###

* Kotlin 用 `==` 代替 Java 的 `equals` 函数，那如何判断两个引用对象是不是同一个对象呢？通过 `===` 判断。Kotlin的 `a == b` 对应的 Java 翻译 `a?.equals(b) ?: (b === null)`
* NaN is considered equal to itself
* NaN is considered greater than any other element including POSITIVE_INFINITY
* -0.0 is considered less than 0.0