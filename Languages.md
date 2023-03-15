##Languages


##Asynchronous Programming

* Threading
* Callbacks -> nested callbacks, JS has moved from callbacks to Promise
* Future/Promise -> JS 返回 Promise 时即可以 Promise.then / Promise.catch 这样链式 Callback 处理，也可以 try / await 这种Procedure 方式处理（想想会议加入流程是个 Procedure 的处理方式，用 await 很符合）， 但Java的 Future 只提供了 Cancel/ isDone / get ，除了Cancel / isDone 有稍些作用外，主线程不可能调用 get 阻塞函数，可能后端有使用 Future 的场景？
* RxJava 等方式，链式编程，API 复杂，需要封装 Observable/ Observer 类，然后调用 observeOn / subscribeOn 等多个API. API太多太具体，不够抽象，不喜欢，虽然也是响应式编程，和MVVM不同的是，是串型单链响应，Observable 源头有多少错误，Observer 就要再次if / else 分开处理，耦合严重。
* MVVM 是响应式，但不是链式，相比链式就是揭耦合，例如 ViewModel 有很多 State,  上层逻辑者之遥定义各种错误的 State，UI编程者根据这不同的 State做页面响应，没有显性的链条，类似从链条处理变成了树。
* Kotlin Coroutines
    * CoroutineScope: 例如 ViewModel 扩展的 viewModelScope / lifecycleScope (都是 CoroutineScope 对象)，CoroutineScope 没有任何接口，只保存CoroutineContext, 同一个context，取消父scope，该context所有coroutines 都取消， CoroutineScope作用就是管理在他作用域内的所有 coroutines. 为什么要有scope？ 通过scope清除所有的子 coroutine, 避免内存泄漏，例如Android 当lifecycle对象销毁时，对应的 lifecycleScope.cancel() 避免所有子coroutines 引用；
    * coroutineScope(小写c开头，是一个fun)，该函数为 CoroutineScope builder，会继承当前block的CoroutineScope. 父scope cancel 了，所有子scope 或子 coroutine 都销毁，这构成 structured concurrency. 类似的还有在父 scope 内调用 withContext(this.coroutineContext) 生成子scope。
    * withContext(Dispatchers.IO) {} 可以在父 scope 中切换到另外一个线程运行，但对调用方来说相当于 synchronous 调用，
    * launch / async 为 coroutine builder. block 内可以调用 suspend 函数，在 launch / async block 内编程，相当于用 procedure的方式写blocking 代码，编译器会帮忙翻译成 non-blocking 的 Callback 形式
    * 函数前面的 suspend 为Kotlin编译器的关键字，代表该函数可能返回 Suspendable 对象 或 真实结果，编译器针对每一个 coroutine 拆分成一个状态机函数，例如下列拆分:

    
		    launch  { //  a new coroutine
		        delay(1000L) // non-blocking delay
		        println("World!") // print after delay
		    }
		    launch-continuation {
		        when (state)
			        case 0:
		        		state = 1
		 		    	delay(1000L)
		        	case 1:
		        		println("World!")
		    }
    
    
    * coroutine编译器翻译过程中，有多少个suspend 点，则有多少个 state case。
    * 那如何走上述 launch-continuation 函数的case 1 呢？举例：delay(100) 这个 suspend 函数，内部调用 Android postDelay 函数，并传入 Callback， Callback 内调用 continuation的 resume 方法，进而重新调用 launch-continuation 函数。


##C++ Primer

###new/delete
delete []: new T[] 时，释放函数必须要用 delete []， 除了释放内存外，还能走数组中每一个对象的析构函数 
如果类中用到动态申请内存，所有构造函数必须要初始化指针 nullptr 或 new, 析构函数直接 delete (无需判nullptr, delete nullptr 是安全的)，delete后，指针置 nullptr


###inline
什么时候用inline，函数内部逻辑执行时间 < 函数调用时间， 空间（代码冗余量） 换 时间（执行效率），很多inline都是一行，空间基本不会增加什么；
和 ANSI C #define 区别？ define 容易出现错误代码: #define x   x*x，使用时 x 可能是  value1 + value2，结果变成 value1 + value2 * value1 + value2；#de fine x (x)*(x) 仍然不能避免：（value++）* (value++), value执行了 ++ 两次；


###引用
C++ 引入引用 来避免 指针不方便管理的问题
函数参数一般加 const & 来传参数，比值传参更高效，如果是 原始类型推荐值传递，struct/class 推荐引用
函数返回不能返回一个局部变量的引用，编译器不会报错，局部变量在函数走完时会走析构函数；
！！！给一个引用对象 赋值（=）一个新的对象，只会更改引用的原对象内部数据；


###函数返回值修饰：const & / & / value / const value
返回 value, 低效，走拷贝构造函数（编译器大多优化掉了 RVO return value optimization）
返回 const value, 避免歧义，歧义代码：value1 + value2 = value3 (value 类有 operator+)
返回 &, 高效，不能返回函数局部变量，
返回 const &，高效，不能返回函数局部变量
Where is the return object stored?


###操作符重载
为什么需要操作符重载？编程更加 OO 思想；
操作符重载必须要求：其中一个操作数是用户定义类型；
操作符重载分为 成员函数操作符重载 (Value1.operator+(Value2)) 和 非成员函数操作符重载(operator+(Value1, Value2))
非成员函数操作符重载原因：T.operator+(int) 可以，但 int.operator+(T) 就不行了？为了避免第二种写法不行，所以存在 非成员函数操作符重载；
非成员函数操作符重载，一般添加 friend 写到类声明地方，定义和封装性更好；
操作符重载函数一般有返回值，为了方便类似 Value1 + Value2 + Value3 等OO场景，std::cout << Value1 << Value2


###Implicit Conversion
(Chapter 11 Automatic Conversions)
隐式转换有两种类型：
A. 只包含一个参数的类构造函数;  T value = 10(implicit) / T value(10) (explicit)
B. 类成员类型操作符重载： operator int() { return int(10); }
两种隐式转换都容易出不可预料的问题，所以建议都加上 explicit

###类构造函数
构造函数只包含一个参数的，一定要加 explicit， 避免 implicit conversion(Chapter 11 Automatic Conversions). 隐式转换有时候会导致编程者意想不到的错误：https://stackoverflow.com/questions/121162/what-does-the-explicit-keyword-mean
构造函数初始化列表中初始化顺序是最后一个先执行，如果类成员初始化有互相条件依赖，一定要注意。Initialization Order(Chapter 14)

###默认拷贝构造函数 和 默认赋值操作符
编译器会自动生成拷贝构造函数 T(const T& t) 和 默认赋值操作符operator=(const T& t)
这两个默认函数都是 成员浅拷贝；如果涉及深拷贝，需要自己重写，如果类中用到 new ，则必须要自己写 拷贝构造函数 和 赋值操作符

###父子类对象互相赋值(Chapter 13 Class Design Review)
Brass base(); BrassPlus plus = base;  不确定，走BrassPlus.operator=(const BrassPlus &)， 如果定义了构造函数 BrassPlus(const Brass &)的话，可以隐式从 Brass 对象专为 BrassPlus;
BrassPlus plus(); Brass base = plus; 可以赋值，走 Brass.operator=(const Brass &base), BrassPlus & 可以向上转换(upcasting) 成 Brass & ；


###继承
先考虑好  is-a 和 has-a 的关系，is-a 是继承，has-a 是组成，Private Inheritance 同样能达到 has-a 效果，参考（Chapter 14）

###静态函数绑定/动态函数绑定
* 首选静态函数绑定，效率高，不多占用内存，编译时根据调用函数的签名就能确定跳转地址；
* 动态函数绑定是编译时有好几个类似的签名（不同的是父子类名修饰符），编译器无法决定实际调用哪一个，需要运行时的类型判断来调用， 例如virtual 关键字函数的处理，编译时给class生成的静态数组，编译器给有virtual函数的类和子类添加一个指针变量，这个变量指向 vtbl (virtual function table). 父类和子类指向不同的 vtbl， vtbl 中存放所有virtual 函数的地址，编译器在生成子类二进制文件时，vtbl pointer 先拷贝一份父类的 vtbl，然后遍历查看子类是否override virtual 函数，有的话，更改子类 vtbl 中函数的地址；额外开销：运行空间多占用 vtbl，运行时先遍历查看 vtbl； 
* 如果子类调用virtual函数时加作用域： Base::function(), 应该时静态函数绑定了？毕竟vtbl指针中virtual函数地址已经更改掉了。
* 构造函数不是继承和动态绑定的实现，子类在执行构造函数时：编译器会默认先调用父类构造函数，或者代码写明调用哪个父类构造函数（DerivedClass(int a) : BaseClass(a)）
* 基类析构函数一定要是 virtual 的
* override 父类虚函数，必须保证函数签名相同（函数名+参数），返回值如果是引用或指针的话，允许协变，父类 virtual 函数返回 DerivedClass &，子类 virtual 函数可以更改返回值为 BaseClass &;


###overload / override / redefinition
overload 函数名相同，参数不同；
override 函数名+参数 子类和父类必须相同，返回值允许协变；
redefinition, overload 的一种，只会在子类重载父类中出现，会导致父类同名函数消失，父类如果多个同名函数也会一同消失；


###lvalue/rvalue
lvalue 是一个有内存地址的表达式且可以使用取地址符 & ，lvalue 可以在赋值运算的左边或右边
rvalue, 其他不是lvalue的表达式，rvalue 只能在 赋值运算 = 的右边，
http://thbecker.net/articles/rvalue_references/section_01.html
* 函数参数一般是通过寄存器传递的，但还是可以取参数的地址(&parameter)，因为实际实践中，参数会以本地变量的形式存在stack中: https://stackoverflow.com/questions/68931847/why-is-taking-address-of-function-argument-allowed-in-c



###std::move
解决function return by pass value 的低性能问题，值传递对象会涉及到对象的拷贝构造函数（或赋值函数），拷贝构造函数内部可能涉及深拷贝，每一次值传递对象都会涉及一次内存深拷贝，std::move通过增加对象两个move 函数来解决该问题：
* move constructor -> construct new object by steal data’s ptr from temporaries(rvalue)
* move assignment operator -> replace left existing object by steal right data’s ptr
* std::move的作用就是将一个lvalue转变为一个rvalue，进而能走对象的 rvalue ref copy constructor and rvalue assignment fun
* std::move可以传入rvalue 也可以传入 lvalue
* lvalue rvalue 另外一个区分就是 有名字的是 lvalue，没有名字的是 rvalue
* Object && B = func(), Object A = B，此时会不会走 A 的move constructor？ 不会，因为 B 类型虽然是  rvalue ref, 但有名字，此时还是走的 copy constructor， 必须要调用 std::move 转换为 rvalue ref.
http://thbecker.net/articles/rvalue_references/section_01.html


###friends 函数存在场景
* 操作符重载有时需要 非成员函数操作符重载，例如 Time.operator*(double m) 如果OOP思想写成  m * Time 就不行，所以需要需要非成员函数操作符重载： void operator*(double m, Time time), 该场景下需要操控 Time 私有成员。
friends 是否不符合 OOP 封装思想？
* friends函数可以看成是class的另外一种 interface，提供更好的灵活性


###static_cast / dynamic_cast / reinterpret_cast / const_cast
* static_cast: 类似C中的 float 转换为 int
* dynamic_cast: 父类指针对象转换为子类对象，如果动态类型判断不符，则返回nullptr
* reinterpret_cast: bits 内容不改，以另外一个对象形式转变，例如 float 翻译为 int，内存数据无改动，翻译规则变了；
* const_cast: 特殊情况下，需要去除const 和 volatile 修饰符


###RAII (Resource-Acquisition-Is-Initialization)
* RAII is a programming idiom, its concept is just a C stack variable, for example a lock object is declared automatically acquire the lock, and released after function returns.
https://stackoverflow.com/a/51284267/5772324


###shared_ptr / weak_ptr
* 通过make_shared 创造对象时，内部会创建两个对象，一个是我们使用的obj，一个是维护引用计数的 __shared_weak_count ref对象；
* ref 对象内部有强引用计数，也有弱引用计数，强引用计数归零，delete obj, 弱引用计数归零，释放 ref

###std::bind
* 绑定一个对象成员函数存放起来以供后续使用；
* 绑定时可传入对象的shared_ptr来控制对象生命周期；
* 绑定时可以指定参数的placeholder;
* 类似Chromium Base 里的 BindOnce / BindRepeat  -> OnceCallback/RepeatCallback
* 返回是个 rvalue reference?

###STL Standard Template Library
