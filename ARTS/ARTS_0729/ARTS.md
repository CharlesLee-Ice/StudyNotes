## Algorithm ##

### Leetcode 上第三道题 ###

**题目：求一个字符串中最长的不重复字符串长度**

	Examples:
	Given "abcabcbb", the answer is "abc", which the length is 3.
	Given "bbbbb", the answer is "b", with the length of 1.
	
**我的答题：**
	
	class Solution {
	public:
	    int lengthOfLongestSubstring(string s) {
	        int maxLen = 0;
	        int start = 0;
	        int end = 0;
	        const char* ptr = s.data();
	
	        while (end < s.length()) {
	            bool isFind = false;
	            for (int i = start; i < end; i++) {
	                if (ptr[i] == ptr[end]) {
	                    start = i + 1;
	                    isFind = true;
	                    break;
	                }
	            }
	
	            if (!isFind) {
	                end++;
	                maxLen = max(maxLen, end - start);
	            }
	        }
	
	        return maxLen;
	    }
	};
	
**参考最优答案感想：**
时间复杂度最低的解答：用Java的`HashSet`来实现是否有重复字符串，避免了我答案中里面的一次for循环。

## Review ##

[Kotlin on The JVM - Bytecode generation](https://kotlinexpertise.com/kotlin-byte-code-generation/)

Java虚拟机：
一个简单快速的定义：Java虚拟机的存在是为了运行Java字节码。Java虚拟机是在不同平台操作系统中运行的一个虚拟电脑，正是有了Java虚拟机，才让Java语言变得平台独立。但是Java虚拟机其实是不知道Java语言的语法的。它只定义能运行在上面的二进制格式（class文件）。
这代表下面几点：

1. Java虚拟机没有指定必须要Java语言编写的程序；
2. 只要编译出正确格式的class文件，任何语言都可以使用；
3. 不同语言编译出的class文件，在虚拟机上可以互相调用。

Java编译器（JDK里面包含的javac）能够将`.java`文件转为`.class`文件，
Kotlin同样通过Kotlin编译器将Kotlin语言编译成`.class`文件。通过`Intelli J IDEA`菜单`Tools → Kotlin → Show Kotlin Bytecode → Decompile` ，可以看Kotlin代码对应的Java实现。

*举例1*

以高级函数为例，在File.kt文件中定义函数（Java不允许在.java文件中直接定义函数，必须放到类中，Kotlin却可以）：

	fun foobar(){}

该函数编译字节码后对应的Java代码如下：
	
	public final class FileKt { public static final void foobar() { } }

可以看到Kotlin的高级函数其实是以类的静态函数来实现的。

*举例2*

定义一个函数，从5依次递减2直到1，打印遍历值

	fun loopWithRange(){ for(i in 5 downTo 1 step 2){ print(i) } }

编译字节码后对应Java代码如下：
	
	   public static final void loopWithRange() {
	      IntProgression var10000 = RangesKt.step(RangesKt.downTo(5, 1), 2);
	      int i = var10000.getFirst();
	      int var1 = var10000.getLast();
	      int var2 = var10000.getStep();
	      if (var2 > 0) {
	         if (i > var1) {
	            return;
	         }
	      } else if (i < var1) {
	         return;
	      }
	
	      while(true) {
	         System.out.print(i);
	         if (i == var1) {
	            return;
	         }
	
	         i += var2;
	      }
	   }
	   
可以看到对应Java实现代码稍有复杂，实际Java编写无需使用类，只要一个for循环即可，但Kotlin采用一种通用方式实现相类似代码的简洁写法，以稍高点的效率换取代码的简洁。
通过上面代码的对应实现，可以更深入的了解Kotlin这门语言。

## Tips ##

[Android数据结构归纳](../../Android数据结构/Android数据结构.md)

## Share ##

这周发现一个学英语的好地方：[BBC Learning English](http://www.bbc.co.uk/learningenglish/)

每天早上上班路上听里面节目：**6 Minute English**