## Kotlin Standard.kt ##

[原文链接](http://beust.com/weblog/2015/10/30/exploring-the-kotlin-standard-library/)

`Standard.kt`文件定义了`Kotlin`一些基本方法，这些方法都是[`Scoping Functions`](https://kotlin.guide/scoping-functions)

### `let()` ###

	public inline fun <T, R> T.let(block: (T) -> R): R {
	    return block(this)
	}

调用方通过参数`it`传入，返回函数的结果。

***为什么在`let()`函数内的`this`是`T`对象，而在`block()`内部的`this`确是外部类对象？***

**答：因为传参的`block()`函数相当于外部类的成员函数**

	DbConnection.getConnection().let { connection -> 100
	}
	
上下文不需要得到`Connection`对象，只需要凭借`Connection`对象计算得到另一个类型的结果`100`。

### `apply()` ###

	public inline fun <T> T.apply(block: T.() -> Unit): T {
	    block()
	    return this
	}

和`let()`函数不一样，`apply()`可以通过`this`访问`Receiver`，并且返回调用方`Receiver`

**这里的`block()`函数定义为类型`T`的扩展函数，所以在`block()`内部访问`this`获取的是类型`T`对象**

	return QuizFragment().apply {
	                arguments = Bundle().apply { putString(Category.TAG, categoryId) }
	            }
	            
`apply()`一般用于`new`新对象时进行初始化。

### also() ###
	public inline fun <T> T.also(block: (T) -> Unit): T {
	    block(this)
	    return this
	}

`also()`函数针对`Receiver`是通过参数传入`it`，类似`let()`，但是返回值同`apply()`，返回`Receiver`。

### run() ###
	public inline fun <T, R> T.run(block: T.() -> R): R {
	    return block()
	}

`run()` 调用的`block()`函数时`Receiver`的扩展函数，同`apply()`。但是返回是新的类型`R`，同`let()`。

### with() ###

	public inline fun <T, R> with(receiver: T, block: T.() -> R): R {
	    return receiver.block()
	}
	
`with()`的使用和`run()`类似。