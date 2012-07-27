服务框架文件说明：
目录： src/online

Command类：封装了服务的启动、停止等操作
Command.cpp、Command.h

Server类：定义了整个服务框架、及所需的资源
Server.cpp、Server.h

Service类：基于anet的通信服务端，定义具体的服务（服务端口、anet的回调句柄。。。）
Service.cpp、Service.h

Session类：用于保存会话信息
Session.cpp、Session.h

TaskQueue类：任务队列，用于存储等待调度的session
TaskQueue.cpp、TaskQueue.h

Dispatcher类：调度类。是一个单独的线程，负责从TaskQueue中得到session，根据session产生相应的worker，从线程池指定线程来运行该worker。
Dispatcher.cpp、Dispatcher.h

WorkerFactory、Worker类：Worker类是用于线程执行操作的封装类、按照不同的角色分为SearcherWork,MergerWorker,BlenderWorker
Worker.h、Worker.cpp

ParallelCounter类：用于服务的流量控制及异常恢复
ParallelCounter.cpp、ParallelCounter.h

CacheHandle类：对memcache client的封装
Cache.cpp、Cache.h

ConnectionPool类：用于管理anet connection
ConnectionPool.h, ConnectionPool.cpp

Request类：基于anet的通信客户端，用于blender向merger、merger向searcher发送请求
Request.cpp、Request.h

WebProbe类：用于提供简单的文件访问服务
WebProbe.cpp、WebProbe.h

Query类：负责解析query串
Query.h、Query.cpp

CMClient类：clustermap客户端
