## 为什么`JNIEnv *env`在`C`和`C++`中使用不一样？

`C`使用：

	//参数 JNIEnv *env
	(*env)->NewStringUTF(env, "Hello World”);
	
`C++`使用：
	
	//参数 JNIEnv *env
	env->NewStringUTF(“Hello World”);	

答：因为用户的JNI代码可能用`C`也可能用`C++`

分场景分析，假定虚拟机用`C++`编译器编译

#### 用户代码`C++`编译

用户引用的头文件`jni.h`定义如下：

	typedef _JNIEnv JNIEnv;
	
	struct _JNIEnv {
	    /* do not rename this; it does not seem to be entirely opaque */
	    const struct JNINativeInterface* functions;
	
	#if defined(__cplusplus)
	
	    jint GetVersion()
	    { return functions->GetVersion(this); }
	
	    jclass DefineClass(const char *name, jobject loader, const jbyte* buf,
	        jsize bufLen)
	    { return functions->DefineClass(this, name, loader, buf, bufLen); }
	
	#endif
	
	}

虚拟机调用用户JNI函数的实际`JNIEnv`结构为：

	struct JNIEnvExt {
	    const struct JNINativeInterface* funcTable;     /* must be first */
	
	    const struct JNINativeInterface* baseFuncTable;
	
	    u4      envThreadId;
	    Thread* self;
	
	    /* if nonzero, we are in a "critical" JNI call */
	    int     critical;
	
	    struct JNIEnvExt* prev;
	    struct JNIEnvExt* next;
	};
	
虚拟机传入用户JNI函数为 `JNIEnvExt *env` ，用户`_JNIEnv`结构第一个变量也是 `JNINativeInterface* functions`, 两者地址匹配，用户使用JNI接口没问题。

#### 用户代码`C`编译
用户引用的头文件`jni.h`定义如下：

	typedef const struct JNINativeInterface* JNIEnv;
	
用户JNI函数可改为：
	
	JNIEXPORT jstring JNICALL
	Java_com_example_hellojni_HelloJni_stringFromJNI( JNINativeInterface** env,
	                                                  jobject thiz );
	//JNIEnv* env -> JNINativeInterface** env,  

虚拟机传入的仍然为`JNIEnvExt *env`，此时用户JNI代码访问变为：`env->funcTable->NewStringUTF`, 此时考虑到`JNINativeInterface* funcTable`就在`JNIEnvExt`结构体的第一位，用户JNI函数直接调用 `(*env)` 得到 `funcTable`。

***为什么不让用户JNI调用`env->functions->NewStringUTF`？ 这样可以屏蔽C/C++差异***

答： 减少用户api调用代码，避免两层指针访问。