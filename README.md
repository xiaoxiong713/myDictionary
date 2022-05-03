# myDictionary
一个C代码的练习案例，在命令行实现客户端和服务端的链接（SOCKET），网络电子词典


# 先前执行：
	sqlite3  my.db
	create table usr(name text primary key, pass text);
	create table record(name text, data text, word text);
	.schema
	.quit
<!-- 
===========================================================
帮助命令（忽略）：
insert into usr values('zhangshan', 123);
//查看僵尸进程 ps axj
===========================================================
 -->
# 拿到代码，先执行:
gcc server.c -o server -lsqlite3
gcc client.c -o client -lsqlite3

# 再run起来：
./server 172.20.10.3 8888
./client 172.20.10.3 8888

# 作者信息
author: xiaoxiong
website: https://www.xiaoxiong713.com
project: 纯C代码，电子词典（文件I/O，SOCKET网络, 进程线程...）
version: 1.0

