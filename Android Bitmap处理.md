## Android Bitmap加载 ##

### 通用知识

图片可以分为三种类型：

* uncompressed (BMP)
* lossy compression (PNG,GIF(only 8bit per pixel))
* lossless compression (JPEG)
 
[difference-between-jpg-jpeg-png-bmp-gif](https://stackoverflow.com/questions/419584/what-is-the-difference-between-jpg-jpeg-png-bmp-gif-tiff-i)

### Android Bitmap 处理规则

* Android `Bitmap`配置是 `ARGB_8888`, 所以`Bitmap`大小是 `长 * 宽 * 4bytes(ARGB各占1byte)`
* `Bitmap`最好在异步线程加载
* 如果加载多张位图，最好做内存和文件缓存，避免占用过多内存

### 大图加载处理

通过设置`BitmapFactory.Options.inJustDecodeBounds`为`true`，可以只解析出图片的尺寸，然后根据展示大小和内存剩余，决定实际解析大小。

#### AsyncTask加载图片
[process-bitmap](https://stuff.mit.edu/afs/sipb/project/android/docs/training/displaying-bitmaps/process-bitmap.html)

### 缓存处理

#### 内存缓存 LruCache

为什么不用`SoftReference`缓存？

答：Android 对`SoftReference`的内存回收更激进，不适合做图片缓存。

* `LruCache`缓存大小可以设置成应用可使用的虚拟最大内存的1/8，应用可使用的虚拟最大内存计算：`Runtime.getRuntime().maxMemory()`，每次加载`Bitmap`前，先检查`LruCache`

#### 磁盘缓存 DiskLruCache

#### 

### inBitmap

* API10之前，`Bitmap`像素存放在native层，`Bitmap`对象存放在虚拟机内存中，（所以要调用`recycle`）API11到API25,`Bitmap`对象和像素都存放虚拟机中，API25(Android 8.0)之后，像素内存又存放在native层。
* API11（Android 3.0）引入了`BitmapFactory.Options.inBitmap`字段，该字段可以设置一个已经不用的`Bitmap`，新的`Bitmap`解析使用已有`Bitmap`的像素内存。但是需要注意的是API19（Android4.4）之前，只有相同大小的bitmaps可以使用。
* 多个`Bitmap`可以通过`LruCache<String, BitmapDrawable>`保存，当`LruCache`抛弃一个`BitmapDrawable`，可以将它放入`var mReusableBitmaps: MutableSet<SoftReference<Bitmap>>`，这样，下次解析新的`Bitmap`时，可以利用`mReusableBitmaps`的位图内存。

## Android 9.0 图片处理

* `ImageDecoder` 代替 `BitmapFactory`,支持图片缩小、采样、裁剪、后期圆角处理，还支持解析`git,webp`动画
* `AnimatedImageDrawable` 支持`gif,webp`动画（异步线程驱动动画），暂无Compat版本

## 主流图片加载库浅析

见Excel表

## 参考 

[Handling bitmap](https://developer.android.com/topic/performance/graphics/) 