1. 功能
   1. 服务器端执行rsh_server server会自动已daemon运行， 将运行记录存到当前目录下的rshlog文件夹下
      1. 程序检测如果没有此文件会自动创建
   2. 客户端执行rsh_client 连接server
      1. 目前不支持 多client连接单个server
2. 缺点
   1. 没有设置终端的属性 如果你改变终端大小 会出现不可预计的错误；之后会增加此功能
3. 编译
   1. make
4. 执行 
   1. ./rsh_server [port]
   2. ./rsh_client [addr] [port] 