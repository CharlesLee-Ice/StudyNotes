## sp、wp、RefBase ##
* `sp``wp``RefBase`是Android在native层实现的一套智能指针类。
* `sp`和`RefBase`组合使用可以实现类对象自动回收机制
*  `wp`配合`sp`使用才有意义：当有其它`sp`使用时，`wp`可以获取到存活的指针，当其它`sp`全都析构掉后，保存的对象指针释放掉，`wp`获取指针返回`null`，类似Java的`WeakReference`的使用场景，不同的是，`WeakReference`保存的对象是在下次`GC`时被回收，而`wp`保存的指针是在最后一个`sp`析构时`delete`
*  `wp`在需要调用对象方法前，需要通过`promote()`转成`sp`，如果返回`null`，则代表对象已内存回收

### sp、RefBase实现原理 ###

* 想要实现对象自动`delete`，类必须继承`RefBase`，同时后面一律通过`sp`对象来访问类成员，如

		class Animal: RefBase {
		    Animal();
		    ~Animal();
		    void eat();
		};
		
		void test() {
		    sp<Animal> ptr = new Animal();
		    ptr->eat();
		}

上例中，在test函数执行完毕后,`Animal`对象会自动`delete`，

* `RefBase`类中两个关键函数：
	
		void incStrong(const void* id) const;
		void decStrong(const void* id) const;
	
	这两个函数分别添加和减少本对象被`sp`占有的总数
* 	`sp`类的构造函数和析构函数分别调用`incStrong``decStrong`
	
		template<typename T>
		sp<T>::sp(T* other)
		    : m_ptr(other)
		{
		    if (other) other->incStrong(this);
		}
			
		template<typename T>
		sp<T>::~sp()
		{
		    if (m_ptr) m_ptr->decStrong(this);
		}
		
* `decStrong`函数内部会检查被`sp`占有的总数，如果等于0，则代表没有`sp`占用此对象，这是可以进行`delete`，实现上述`Animal`类对象的自动回收

### wp、RefBase实现原理 ###

`RefBase`构造函数时生成`weakref_impl`对象，`weakref_impl`是`RefBase`的内部类

	RefBase::RefBase()
	    : mRefs(new weakref_impl(this))
	{}
	
其内部通过维护下面四个成员实现强弱引用的计数：
	
	volatile int32_t    mStrong;
	volatile int32_t    mWeak;
	RefBase* const      mBase;
	volatile int32_t    mFlags;
	
	//mFlags取值
    enum {
        OBJECT_LIFETIME_WEAK    = 0x0001,
        OBJECT_LIFETIME_FOREVER = 0x0003
    };
    
`mFlags`默认0，满足强弱指针的大部分使用场景。
`mStrong`为强引用计数，就是上述讨论的`sp`占有计数，由`incStrong``decStrong`控制。
`mWeak`为弱引用计数， 当下列两种场景会进行加减：

* `incStrong``decStrong`函数内会进行`incWeak``decWeak`

		void RefBase::incStrong(const void* id) const
		{
		    weakref_impl* const refs = mRefs;
		    refs->addWeakRef(id);
		    refs->incWeak(id);
		    refs->addStrongRef(id); 
		 
		    const int32_t c = android_atomic_inc(&refs->mStrong); 
		    ...
		}
		
		void RefBase::decStrong(const void* id) const
		{
		    weakref_impl* const refs = mRefs;
		    refs->removeStrongRef(id);
		    const int32_t c = android_atomic_dec(&refs->mStrong);

		    if (c == 1) {
		        const_cast<RefBase*>(this)->onLastStrongRef(id);
		        if ((refs->mFlags&OBJECT_LIFETIME_WEAK) != OBJECT_LIFETIME_WEAK) {
		            delete this;
		        }
		    }
		    refs->removeWeakRef(id);
		    refs->decWeak(id);
		}

*为什么`sp`在操作`incStrong``decStrong`会关心`mWeak`的计数？*

答：当`mFlags`为0时，对象回收完全由`mStrong`是否为0决定，但是如果`mFlags`为`OBJECT_LIFETIME_WEAK`，则代表对象回收还由`mWeak`决定，所以在`decStrong`内，如果判断`mFlags == OBJECT_LIFETIME_WEAK`为`true`，则不能执行`delete`。此时`decStrong`最后一行`decWeak`内部进行`mWeak`计数判断，如果为0，则执行`delete`。

* `wp`构造和析构函数内部会进行`incWeak``decWeak`
	
		template<typename T>
		wp<T>::wp(T* other)
		    : m_ptr(other)
		{
		    if (other) m_refs = other->createWeak(this);
		}
		
		
		RefBase::weakref_type* RefBase::createWeak(const void* id) const
		{
		    mRefs->incWeak(id);
		    return mRefs;
		}
		
		
		template<typename T>
		wp<T>::~wp()
		{
		    if (m_ptr) m_refs->decWeak(this);
		}
		
当然`decWeak`内部`delete`是否执行是要看`mFlags`取值的。

当想通过`wp`操作对象时，不能像`sp`那样直接执行`->`（操作符重载），必须调用`promote()`转换成`sp`。当然这时候可能`promote()`返回`null`。具体代码如下：
	
	template<typename T>
	sp<T> wp<T>::promote() const
	{
	    return sp<T>(m_ptr, m_refs);
	}
	
	template<typename T>
	sp<T>::sp(T* p, weakref_type* refs)
	    : m_ptr((p && refs->attemptIncStrong(this)) ? p : 0)
	{}
	
	bool RefBase::weakref_type::attemptIncStrong(const void* id)
	{
		incWeak(id);
	 
		weakref_impl* const impl = static_cast<weakref_impl*>(this);
	 
		int32_t curCount = impl->mStrong;
		LOG_ASSERT(curCount >= 0, "attemptIncStrong called on %p after underflow",
			this);
		while (curCount > 0 && curCount != INITIAL_STRONG_VALUE) {
			if (android_atomic_cmpxchg(curCount, curCount+1, &impl->mStrong) == 0) {
				break;
			}
			curCount = impl->mStrong;
		}
	 
		if (curCount <= 0 || curCount == INITIAL_STRONG_VALUE) {
			bool allow;
			if (curCount == INITIAL_STRONG_VALUE) {
				// Attempting to acquire first strong reference...  this is allowed
				// if the object does NOT have a longer lifetime (meaning the
				// implementation doesn't need to see this), or if the implementation
				// allows it to happen.
				allow = (impl->mFlags&OBJECT_LIFETIME_WEAK) != OBJECT_LIFETIME_WEAK
					|| impl->mBase->onIncStrongAttempted(FIRST_INC_STRONG, id);
			} else {
				// Attempting to revive the object...  this is allowed
				// if the object DOES have a longer lifetime (so we can safely
				// call the object with only a weak ref) and the implementation
				// allows it to happen.
				allow = (impl->mFlags&OBJECT_LIFETIME_WEAK) == OBJECT_LIFETIME_WEAK
					&& impl->mBase->onIncStrongAttempted(FIRST_INC_STRONG, id);
			}
			if (!allow) {
				decWeak(id);
				return false;
			}
			curCount = android_atomic_inc(&impl->mStrong);
	 
			// If the strong reference count has already been incremented by
			// someone else, the implementor of onIncStrongAttempted() is holding
			// an unneeded reference.  So call onLastStrongRef() here to remove it.
			// (No, this is not pretty.)  Note that we MUST NOT do this if we
			// are in fact acquiring the first reference.
			if (curCount > 0 && curCount < INITIAL_STRONG_VALUE) {
				impl->mBase->onLastStrongRef(id);
			}
		}
	 
		impl->addWeakRef(id);
		impl->addStrongRef(id);
	 
		if (curCount == INITIAL_STRONG_VALUE) {
			android_atomic_add(-INITIAL_STRONG_VALUE, &impl->mStrong);
			impl->mBase->onFirstRef();
		}
	 
		return true;
	}
	
	bool RefBase::onIncStrongAttempted(uint32_t flags, const void* id)
	{
	    return (flags&FIRST_INC_STRONG) ? true : false;
	}

这里主要分这几种情况：

* `m_ptr`为`null`，则直接返回`null`
* `mStrong > 0`，则有`sp`占用，可以安全返回
* `mStrong == INITIAL_STRONG_VALUE`（`mStrong`默认值为`INITIAL_STRONG_VALUE`），则`allow`一直返回`true`，为什么呢？
	
		allow = (impl->mFlags&OBJECT_LIFETIME_WEAK) != OBJECT_LIFETIME_WEAK
			|| impl->mBase->onIncStrongAttempted(FIRST_INC_STRONG, id);

	答：一个对象`new`出来，`sp`没用过，用了`wp`，则默认使用者不会主动`delete`的，因为如果使用者自己主动`delete`掉，那他使用`wp`有什么意义（都自己`delete`了，还用智能指针干嘛）？所以这块程序默认返回`true`，可见`wp`还是要配合`sp`才符合用法。
* `mStrong <= 0`,则此时必须要求`mFlags == OBJECT_LIFETIME_WEAK`才能返回`true`，毕竟此时调用在`wp`类内部，`mWeak`肯定大于0

### 提问 ###

当`sp`不再使用，`delete RefBase`后，`wp`是如何追踪到该对象失效的呢？

答：
但`RefBase`被`sp``decStrong`函数`delete`后，`RefBase`内部类对象`weakref_impl`还没有被`delete`掉，而`wp`通过`weakref_type* m_refs`保存该对象，只有当`weakref_impl`内部`mWeak`计数也变成0才执行`weakref_impl`的`delete`

## 参考 ##
[Android系统的智能指针](https://blog.csdn.net/luoshengyang/article/details/6786239)