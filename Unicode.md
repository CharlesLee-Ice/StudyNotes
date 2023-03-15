#Unicode & Unicode Encoding


## Unicode
### Unicode Code Space

* Code points 范围： 0 ～ 0x10FFFF
* 最高位 0~0x10：1个基本平面（BMP: Basic Multilingual Plane） +  16 个扩展平面
* 每一个平面对应 0～0xFFFF 65536个 code points
* 总共可表达的 code points： 17 * 65536 = 1,114,112
* Plan1:  SMP  Supplementary Multilingual Plane)  包含 Emoji, 数学字母符号, 音乐符号等
* 所有 code points中约 10% 定义了并在使用

### Unicode Scalars
* Unicode Scalars 是一部分 code points 的集合： U+0000~U+D7FF 和 U+E000~U+10FFFF
* 中间的 U+D800~U+DFFF是 Surrogate Pair


#### Unicode Code Points拼接 / Canonical Equivalence(NF) / Compatibility Equivalence(NFK)
* 国家码（CN, HK, TW, US, GB）也是两个Unicode code points的拼接，Code Points Range: U+1F1E6(地区码A)～U+1F1FF（地区码Z）
* 字符 é 有两种 Unicode Standard 定义形式： 一种是precomposed形式（单个code point U+00E9），一种是decomposed形式(两个 code points: U+0065 字母e 和 U+0301 ◌́ 的拼接)，这两种可能 Unicode code point 不一样，但语义和屏幕视觉展示是一样的，对应不同编码时字符串比较应该要相同，这种对比方式为 Canonical Equivalence(NF)
* 部分系统展示字母 fi 是，fi 是连在一块的，看不到 i 上面的点，为了避免这个情况，Unicode Standard专门定义了 ff, fi, fl, ffi, ffl . 其中每一个组合对应一个 code point.   而当 U+0066(f)和U+0066(f) 组合在一起时，和 U+FB00(ff) 语义上一样，但屏幕展示可能不一样，这种对比方式成为 Compatibility Equivalence(NFK)



####全角半角
* 中日韩等标点符号屏幕上显示更宽，占用两个半角字符的宽度，需要对应的Unicode编码（字母”A-Z/a-z”也有全角）；
* 早期扩展的2-bytes GB编码中，这些中文符号也是占用 2-bytes，比ASCII中的1byte 大一倍；


##UTF (Unicode Transformation Formats)
* UTF为Unicode 的存储传输形式；


####UTF-16

* Unicode Standard 默认编码方式为 UTF-16, Why?
    * UTF-16定义的早，80年代就已定义，而 UTF-8 93年才面世。UTF-16早期为了扩展 ASCII 和多语言兼容而定义出来，后续发现范围还是不够；Windows API和内核均已采用(UTF-16LE)；应用层的String也默认为 UTF-16，Java  9（Char字符）也采用UTF-16BE，注意 Android String 的Serialized后字符串编码格式随local 的Linux系统走，也就是 UTF-8；
    * UTF-16能直接表达 BMP 所有的code points，Standard定义即编码，直观；其他平面可以通过两个 UTF-16 来表达，借用Standard 保留的 Surrogate Pair；

* UTF-16如何表达 > 0xFFFF 的 code points？
    * UTF-16 用两个 UTF-16 字符来表达，也就是4字节来表达；
    * Unicode Standard中 U+D800 ~ U+DFFF 为保留字段（Surrogate Pair），将其分为两个范围:
        * U+D800~U+DBFF: 该表达范围为 0~0x03FF，共10bits，
        * U+DC00~U+DFFF: 该表达范围为 0～0x03FF，共10bits，
        * 上述实际共可表达20bits 的 Unicode Code Points， 用前 4 bits 来表达Plans，即0～0xF可表达16个平面，剩余16bits 表达每一个Plane下的 code points（0～0xFFFF）
        * Surrogate Pair 可表达除 BMP 外的16个平面，加上双字节本身表达的 BMP 平面，共组成了 Unicode Standard的17个平面定义，这也是为什么定义17个平面的原因；

* 为什么定义了17个平面（0x1F）而不是16（0xF）？ 0xF明显少一位
    * 见上述 “UTF-16如何表达 > 0xFFFF 的 code points？”

* UTF-16的文本如何区分Byte Order？
    * 单个UTF-16字符在内存中如何区分语义高位和低位？如果区分了第一个UTF-16字符，则后面字符就自动确定了；
    * UTF-16字符串需在文本前插入 BOM  字符(Byte Order Mark)， Unicode Standard 定义 U+FEFF， 如果字符串第一个byte是0xFE，则为Big-Endian存储，如果第一个byte是0xFF，则为Small-Endian存储；

* 为什么UTF-16使用不广？
    * 占用空间大，无效0x00高位占用，约50%浪费;
    * 需要插入 BOM ;

* 为什么编码会涉及到 Big-Endian (BE) or Little-Endian (LE) Byte Order?
    * 当一个byte编码不够时，早期扩展的 16-bit encoding 来表示一个字符，那这个 16-bit有内存高地位，内存的高地位和表达的高地位可能不一致。
    * 大小端不涉及多个字符的顺序，只涉及一个字符的系统内存高地位顺序。


####UTF-8

可能用1～4个bytes来表达一个code points，
    * ASCII码 用一个byte， 0xxx xxxx 直接表达，range: \u0001 ~ \u007F
    * 2个bytes表达形式:      110x xxxx, 10xx xxxx, range: \u0080 ~ \u07FF
    * 3个bytes表达形式：   1110 xxxx, 10xx xxxx, 10xx xxxx, range: \u0800 ~ \uFFFF
    * 4个bytes表达形式：   1111 0xxx, 10xx xxxx, 10xx xxxx, 10xx xxxx ，4字节UTF-8用于除BMP的其他平面（Supplementary Characters）
    * 第一个字符前面的 1110 用 ‘0’ 来将 ‘111’ 和后面的实际字符隔开，如果没有 0，则无法知道长度，后面的 10， 代表是跟随字符，如果没有 0则无法知道是第一个字符还是跟随字符；

* UTF-8 目前使用最广原因：
    * 空间占用少，利于存储和传输；
    * ASCII码表示的字符不变，兼容 ASCII，历史app可在 UTF-8的系统环境下正常运行；
    * 没有 Byte Order的干扰；

* UTF-8 缺点
    * 无法index访问字符串中的指定字符，UTF-16也不行；
    * 不利于字符串解析，UTF-16要快些，因为大部分2bytes跳过；


###Modified UTF-8

####表达特点
* 字符串中的 U+0000 (NULL) 通过两个字节 110 00000 10 000000 (0xC0 80) 来表达，而不是 0 0000000，这样含有 NULL 的文本照样可以用传统 C 库str相关函数（strlen等），避免了文本中空字符导致的文本处理错误。
* 只有 1-byte, 2-byte, 3-byte 的UTF-8在使用，原来4-byte 表达的 Supplementary characters 通过 surrogate pairs 来表达；

####Others
* JNI 相关String API / DataInput and DataOutput API / Constant String in class files 时采用的 Modified UTF-8
* 数据InputStreamReader/OutputStreamReader/String序列化等还是根据平台（例如Linux）的UTF-8标准来；
* String 内部存储形式根据 Java 版本变化，Java 9之前用 UTF-16 （有一个版本不一样），Java 9默认开始压缩存储：判断是否都是单子节字符，则使用 ISO-8859-1 （ASCII 变种，高位bit也使用），如果判断有双子节字符，则使用UTF-16
* 想想之前JNI 部分 Emoji 字符无法传递给上层？ 
    * 可能是 JNI 中 String 是 modified-UTF8，emoji用两个3bytes 形式表达，而从网络侧收到的却是 4bytes的 UTF8 emoji，该4bytes传入JNI String，JNI String向Java String转换出现字符无法转换问题。但为什么Android5以下才出现，难道Android5以上适配了4bytes的UTF8？

## Reference
[Modified_UTF-8](https://en.wikipedia.org/wiki/UTF-8#Modified_UTF-8)

[Java modified-utf-8](https://docs.oracle.com/javase/6/docs/api/java/io/DataInput.html#modified-utf-8)

[Stackoverflow modified-utf-8](https://stackoverflow.com/questions/9699071/what-is-the-javas-internal-represention-for-string-modified-utf-8-utf-16/9699138#9699138)

[Unicode](https://andybargh.com/unicode/)













