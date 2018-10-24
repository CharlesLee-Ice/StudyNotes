## Supporting Multiple Screens ##

### px = dp * (dpi / 160) ###
**px / dpi = dp / 160**

* dp是 Density-Independent Pixels(dip)
* dpi是 Dots Per Inch
* ppi(Pixels Per Inch) == dpi
* 160dp == 1 inch == 2.54cm
* px是像素单位，而dp是长度单位
* Density计算： px/dp

## Screen Density Solution ##

### Screen Density划分 ###

	ldpi (low) ~120dpi
	mdpi (medium) ~160dpi
	hdpi (high) ~240dpi
	xhdpi (extra-high) ~320dpi
	xxhdpi (extra-extra-high) ~480dpi
	xxxhdpi (extra-extra-extra-high) ~640dpi

主流手机：

* 华为P20（429ppi）~ xxhdpi
* vivo X21（402ppi）~ xxhdpi
* Google Pixel XL(533ppi) ~ xxxhdpi

[Device Matrixs](https://material.io/tools/devices/)

### Solution ###

#### 1. Use density-independent pixels ####
宽高使用 dp 单位，和像素无关， 文本大小使用 sp 单位，随屏幕和设置页字体大小而变化

#### 2. Provide alternative bitmaps ####
添加相应密度的资源文件夹，例如图片资源：

	res/drawable-ldpi/
	res/drawable-mdpi/
	res/drawable-hdpi/
	res/drawable-xhdpi/
	res/drawable-xxhdpi/
	
1. 系统根据屏幕dpi选择相应文件夹下的资源
2. 如果未找到合适的资源，则选择默认资源进行放大或缩小

选择默认资源缩放时，系统会合理选择，例如缺少ldpi的图片，则系统会选择hdpi的图片以0.5比例缩小，而不会选择mdpi图片缩小0.75。因为0.5比例缩小容易实现。

**Question:**

1. 为什么launch icon要另外放入mipmap文件夹？[mipmap vs drawable](http://stackoverflow.com/questions/28065267/mipmap-vs-drawable-folders)

According to this Google blogpost:

	It’s best practice to place your app icons in mipmap- folders (not the drawable- folders) 
	because they are used at resolutions different from the device’s current density.

mipmap文件夹一般分的dpi更多，例如xxxhdpi，这是为了让应用图标在高屏幕手机桌面上显示的更加清晰，而如果应用全面支持drawable-xxxhdpi的话，该目录下图片资源比较大，导致应用体积大，所以一般drawable资源只支持到xxhdpi。

#### 3. Use vector graphics instead ####
支持矢量图代替bitmap

## Screen Size Solution ##

#### 1. Use ConstraintLayout ####
LinearLayout通过`layout_weight`属性来伸缩视图，但是layout过程中有一定性能开销，使用ConstraintLayout更加灵活高效。

#### 2. Avoid hard-coded layout sizes ####
避免使用固定dp的宽高，尽量使用`match_parent`，`wrap_content`属性。

#### 3. Different Layout ####
针对不同的屏幕大小提供不同的UI布局文件，例如：

	res/layout-land/main_activity.xml           
	// 横屏布局
	res/layout-sw600dp/main_activity.xml  
	// For 7” tablets (600dp wide and bigger) 无论横屏竖屏，取屏幕两边最小值，至少600dp以上，采用此目录内布局
	res/layout-w600dp/main_activity.xml  
	// 无论横屏竖屏, 当前宽度不少于600dp时，采用此目录内布局
	res/layout-sw600dp-land/main_activity.xml
	// 横屏时，当前宽度不少于600dp，采用此目录内布局

#### 4.Modularize UI components with fragments ####
模块化代码成Fragment，方便不同layout使用fragment拼接。

#### 5. Create stretchable nine-patch bitmaps ####
使用点9图支持不同大小场景。

#### 
## 参考 ##
[Android Developer](http://developer.android.com/guide/practices/screens_support.html)