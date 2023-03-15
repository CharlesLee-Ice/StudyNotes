### 缓存

* 服务器回复 Expires / Cache-Control
	1. Expires: Sun, 16 Oct 2016 05:43:02 GMT  服务器告知此日期前都有效，存在客户端和服务器时间不一致问题
	2. Cache-Control: max-age=315360000  服务器表示从现在开始315360000秒内缓存都是有效，解决时间不一致问题
	
* 服务器回复 Last-Modified（最后修改时间）/ 客户端带上 If-Modified-Since （服务器下发的Last-Modified），服务器根据 If-Modified-Since，决定是否返回304

* 服务器回复 ETag / 客户端带上 If-None-Match （服务器下发的ETag），服务器根据 If-None-Match 决定是否返回304

