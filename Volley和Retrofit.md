## Volley 和 OkHttp

* `Volley`模式使用`HttpURLConnection`来进行网络请求（`HttpURLConnection`在4.4以后也是用的`OkHttp`，但是由于集成到了Android源码，所以版本较老），虽然`Volley`也可以配置最新`OkHttp`(`RestVolley`项目)，但是`OkHttp`最新版本已经不支持`Volley`的集成兼容(之前`OkHttp`提供`OkUrlFactory`类来生成`HttpURLConnection`对象)。 

		/**
		 * @deprecated OkHttp will be dropping its ability to be used with {@link HttpURLConnection} in an
		 * upcoming release. Applications that need this should either downgrade to the system's built-in
		 * {@link HttpURLConnection} or upgrade to OkHttp's Request/Response API.
		 */
		public final class OkUrlFactory implements URLStreamHandlerFactory, Cloneable

* `OKHttp`支持`SPDY``HTTP2``WebSocket`等新特性，如果使用`Volley`默认的`HttpURLConnection`，则可能用不到这些新特性，`Volley`在低版本支持的还是`org.apache.http.legacy`的`HttpClient`，也没有考虑废弃代码迁移。

* `Volley`对于`Response`是全内存处理（包括图片请求），适合应用都是频繁的、数据量小的请求，对于大数据的的请求和响应容易OOM，而`OkHttp`对于大数据的请求和响应使用`okio`,通过`stream`处理数据，避免内存占用过多。

* `Retrofit` 和 `OkHttp`配合使用，一个专注适配，一个专注网络，缓存、重传都交给`OkHttp`做，`Retrofit`对`OkHttp`进行`Restful`，配合`RxJava`使用更优。而`Volley`配置`OkHttp`就有点职责重复，两者都做各自的缓存和重传。

* `Volley`的默认重传策略比较激进，2.5s一次重传，在网络稍差情况下，就有可能重发包，而`OkHttp`默认是10s。
* `Volley`虽然支持图片下载和解码，但是通常应用都会用其他第三方库。
* `Volley`不支持同步，但是可以自己通过定义`Future`类去实现，`Future`去`wait()`等待同时，当`RequestQueue`异步返回`Response`时，触发`Future`的`notifyAll()`函数，`wait()`返回得到`response`。
* 目前我们用的`DownloadEngine`其实封装的是`OkHttp`，没有用`volley`就是因为`OkHttp``Response`可以`response.body().byteStream()`用流的方式读取。为什么没有用系统的`DownloadManager`，因为系统`DownloadManager`会展示一个前台`notification`条目，不适合应用内自定义UI下载。而`OkHttp`通过`Response`的`Content-Length`可以做下载百分比等自定义UI。

## Volley 源码分析

一个应用可以共享一个`RequestQueue`, 该`RequestQueue`包含以下内容：

1. 一个网络请求队列`PriorityBlockingQueue<Request<?>> mNetworkQueue`，存放发送的请求:
	* 使用`PriorityQueue`（优先堆：最小堆）目的是为了让高优先级的`Request`优先处理，相同优先级则按顺序处理；
	* 使用`BlockingQueue`目的是为了调用`take`函数，当无数据时，阻塞等待数据返回；
2. 多个网络请求线程：`NetworkDispatcher extends Thread`，一般根据cpu的个数决定线程数，多个`NetworkDispather`共享一个`mNetworkQueue`，`NetworkDispatcher`的`run()`函数通过`mNetworkQueue.take()`获取优先级最高的`Request`，然后调用`NetworkResponse networkResponse = mNetwork.performRequest(request)`阻塞等待网络返回，返回后解析HTTP报文数据，通过`mDelivery.postResponse(request, response);` 提交到主线程去。
3. 一个缓存请求队列`PriorityBlockingQueue<Request<?>> mCacheQueue`，当`Request`内部的`mShouldCache == true`时，`Request`请求被直接丢到`mCacheQueue`中。
4. 一个缓存线程：`CacheDispatcher extends Thread`，`run()`函数处理`mCacheQueue`中的请求
	* 值得注意的是，当`mShouldCache == true`并且后端配置了HTTP协议的缓存协议，则`Volley`缓存机制发挥效果。如果当只客户端允许缓存，而HTTP`Response`没有缓存协议（如：`Cache-Control``Expires``Last-Modified``ETag`），则`HttpHeaderParser`类`parseCacheHeaders`中生成的`Cache.Entry entry`对象里面生命周期`ttl``softTtl`都为0，缓存没有效果。
	* `CacheDispathcer`找缓存是通过`Cache`接口找，如果找到，并且未超时，则直接`mDelivery.postResponse(request, response)`，如果未找到缓存或者缓存失效，则将`Request`塞回`mNetworkQueue`，`Cache`接口的默认实现类是`DiskBasedCache`
5. `DiskBasedCache`内部也是维护`LinkedHashMap`来维护`CacheHeader`，`CacheHeader`是从缓存的文件中读取的，`LinkedHashMap`内部除了是`HashMap`结构外，还额外维护成一套双向链表结构，记录数据的访问顺序，所以`DiskBasedCache`除了可以抛弃超出缓存期限的数据外，也可以实现LRU算法，丢弃最不常使用的数据。
	

