服务器，gateserver、statusserver、chatserver、verifyserver
封装defer类，在函数返回时执行一系列操作类似go语言的defer操作
使用protobuf序列化与grpc服务进行服务器之间通信
封装grpc连接池，grpc客户端根据主机名获取连接池，从连接池中取出连接与grpc服务器通信
使用hiredis、Mysql Connector C++第三方库 封装redis、mysql（单例模式）数据库连接池，数据库DAO层
使用boost asio封装网络层、使用多reactor模式，主reactor负责接收连接交给子reactor监听连接
将客户端连接封装成session类含有socket，sessionid，userid等成员变量，可根据sessionid实现踢人，封装数据的接收和发送接口
定义tlv（消息id+消息长度+消息内容）消息结构实现服务器和客户端通信，消息内容采用json格式，构造消息接收节点、处理切包，构造消息发送节点，保证发送数据的有序性
使用单线程logic system处理客户端发送的请求，网络消息封装成logic node投递到logic system的任务队列中，logic system根据消息id查找不同的回调函数处理
封装用户管理，建立uid和session的映射关系，对数据进行转发
    

客户端
tcp mgr处理发送，通过发送信号投递到等待队列可以保证发送的有序性，线程安全



