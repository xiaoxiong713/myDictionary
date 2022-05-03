#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <signal.h>
#include <time.h>

#define N 32
#define R 1 //注册
#define L 2 //login
#define Q 3 // query
#define H 4 // history
#define DATABASE "my.db"

typedef struct
{
    int type;
    char name[N];
    char data[256];
} MSG;


//函数声明
int do_client(int acceptfd, sqlite3 *db); 
void do_register(int acceptfd, MSG *msg, sqlite3 *db);
int do_login(int acceptfd, MSG *msg, sqlite3 *db);
int do_query(int acceptfd, MSG *msg, sqlite3 *db);
int do_history(int acceptfd, MSG *msg, sqlite3 *db);
int do_search_word(int acceptfd, MSG * msg, char * word);
int history_callback(void* arg, int f_num, char** f_value, char** f_name);
//===========================================  main  ===================================================
// ./server 192.168.3.196  10000
int main(int argc, char const *argv[])
{

    if (argc != 3)
    {
        printf("Usage: %s server port,\n", argv[0]);
        return -1;
    }
    //======================================  参数判断 ==============================================================
    int sockfd;
    struct sockaddr_in serveraddr;
    MSG msg;
    sqlite3 *db;
    int acceptfd;
    pid_t pid;


    //打开数据库
    if (sqlite3_open(DATABASE, &db) != SQLITE_OK)
    {
        printf("%s\n", sqlite3_errmsg(db));
        return -1;
    }
    else
    {
        printf("open DATABASE success,\n");
    }


    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("fail to socket.\n");
        return -1;
    }

    bzero(&serveraddr, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]); // 把ip地址转为二进制
    serveraddr.sin_port = htons(atoi(argv[2]));

    if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr) )  < 0)
    {
        perror("fail to bind.\n");
        return -1;
    }

    //将套接字设为监听模式
    if (listen(sockfd, 5) < 0)
    {
        printf("fail to listen.\n");
        return -1;
    }

    //处理僵尸进程
    signal(SIGCHLD, SIG_IGN);

    while(1)
    {
        if ((acceptfd = accept(sockfd, NULL, NULL)) < 0)
        {
            perror("fail to accept.\n");
            return -1;
        }
        if ((pid = fork()) < 0)
        {
            perror("fail to fork.\n");
            return -1;
        }
        else if(pid == 0) // son process 处理客户端的具体的消息
        {
            close(sockfd);//关闭监听套接字
            do_client(acceptfd, db);
        }
        else //father process  接受客户端的请求
        {
            close(acceptfd);
        }

    }

    return 0;
}


int do_client(int acceptfd, sqlite3 *db)
{
    MSG msg;

    while(recv(acceptfd, &msg, sizeof(msg), 0) > 0)//阻塞式接收   //客户端退出，会返回0
    {
        printf(" 接收到了， type: %d\n", msg.type); //console.log
        //接受success
        switch(msg.type)
        {
        case R:
            do_register(acceptfd, &msg, db);
            break;
        case L:
            do_login(acceptfd, &msg, db);
            break;
        case Q:
            do_query(acceptfd, &msg, db);
            break;
        case H:
            do_history(acceptfd, &msg, db);
            break;
        default:
            printf("Invalid data msg.\n");
        }
    }
    printf("client exit.\n");
    close(acceptfd);
    exit(0);

    return 0; //只是一个写法，永远不可能执行到此
}

void do_register(int acceptfd, MSG *msg, sqlite3 *db)
{
    printf("服务器端开始执行注册函数do_register\n");
    char * errmsg;
    char sql[128];
    sprintf(sql, "insert into usr values('%s', %s);", msg->name, msg->data);
    printf("%s\n", sql); // console.log
    if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        //ERROR
        printf("%s\n", errmsg);
        strcpy(msg->data, "usr name already exist.");//向客户端返回信息
    }
    else
    {   
        //SUCCESS
        printf("client resgister ok!\n");
        strcpy(msg->data, "ok!XD");//向客户端返回信息
    }

    if(send(acceptfd, msg, sizeof(MSG), 0) < 0) // 向客户端返回信息
    {
        perror("fail to send.");
        return;
    }

    return;
}

int do_login(int acceptfd, MSG *msg, sqlite3 *db)
{
    printf("服务器端开始执行登陆函数do_login\n");
    char sql[128] = {};
    char *errmsg;
    int nrow;
    int ncloumn;
    char ** res;//查询结果, 需要一个三级指针， 则定义二级指针

    sprintf(sql, "select * from usr where name = '%s' and pass = '%s';", msg->name, msg->data);
    printf("%s\n", sql);//console.log
    //sql查询, sqlite3_exec查询需要写回调函数callbak， 查询的结果不好明确的判断，故用sqlite3_get_table
    if (sqlite3_get_table(db, sql, &res, &nrow, &ncloumn, &errmsg) != SQLITE_OK)
    {
        //查询失败
        printf("%s\n", errmsg);
        return -1;
    }
    else
    {
        printf("SQL查询语句执行成功， 但不代表匹配结果\n");
    }

    if (nrow == 1) //查询到了nrow=1, 一条结果， 未查到，nrow为0， 因为我们设置有主健， 故不可能>1
    {
        //查询到了
        strcpy(msg->data, "OK");
        send(acceptfd, msg, sizeof(MSG), 0);
        return 1;
    }

    if(nrow == 0)
    {
        //查询未找到， 返回密码或用户名错误
        strcpy(msg->data, "用户名或密码错误");
        send(acceptfd, msg, sizeof(MSG), 0);
        return 0;
    }


    return 0;
}


// char* gettime()
// {
//     time_t time_now;
//     struct tm *t;
//     time_now=time(NULL);
//     t=localtime(&time_now);
//     return asctime(t);
// }


int get_date(char *date)
{
    time_t t;
    struct tm *tp;
    time(&t);//获得1970秒数

    //时间转换
    tp = localtime(&t);
    sprintf(date, "%d-%d-%d %d:%d:%d", tp->tm_year + 1900, tp->tm_mon+1, tp->tm_mday,
                tp->tm_hour, tp->tm_min, tp->tm_sec);

    printf("get_date: %s\n", date);//console.log
    return 1;
}



int do_query(int acceptfd, MSG *msg, sqlite3 *db)
{
    printf("服务器端开始执行查询单词函数do_query\n");
    
    char word[64];
    strcpy(word, msg->data);//拿出从客户端传过来的单词数据

    int found = 0;
    char date[128];
    char sql[128];
    char *errmsg;
    found = do_search_word(acceptfd,msg,word);
    if (found == 1)
    {
        //找到了单词, 将用户名， 时间， 单词， 插入到历史记录表格中

        //获取系统时间, 添加历史记录到数据库表格
        get_date(date);
        sprintf(sql, "insert into record values('%s', '%s', '%s')", msg->name, date, word);
        printf("%s\n",sql); //console.log
        if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
        {
            printf("%s\n", errmsg);
            return -1;
        }
    }
    else
    {
        //未找到单词
        strcpy(msg->data, "抱歉，未查询该单词:(");
    }

    //将查询到的结果(单词注释部分)，发送回客户端
    send(acceptfd, msg, sizeof(MSG), 0);

    return 1;
}

//得到历史记录查询结果,并且需要将历史记录发送给客户端
                                //列数
int history_callback(void* arg, int f_num, char** f_value, char** f_name)
{
    //历史记录表格 表头 ：    name  time   word
    int acceptfd;
    MSG msg;
    acceptfd = *((int *)arg);
    sprintf(msg.data, "%s , %s", f_value[1], f_value[2]);
    send(acceptfd, &msg, sizeof(MSG), 0);

    return 0;
}
int do_history(int acceptfd, MSG *msg, sqlite3 *db)
{
    printf("服务器开始执行查询历史记录函数do_history\n");

    char sql[128] = {};
    char * errmsg;

    sprintf(sql, "select * from record where name = '%s'", msg->name);
                            //这个回调会执行多次，查询到一条结果，就执行一次回调， 我猜想应该会阻塞，直到查询结果完了
    if (sqlite3_exec(db, sql, history_callback, (void *)&acceptfd, &errmsg) != SQLITE_OK)
    {
        printf("%s\n", errmsg);
    }
    else
    {
        printf("历史记录查询成功\n");
    }

    //所有的记录查询发送完毕，给客户端发送一个结束信息
    msg->data[0] = '\0';
    send(acceptfd, msg, sizeof(MSG), 0);
    return 0;
}


int do_search_word(int acceptfd, MSG * msg, char * word)
{
    //对文件的操作： 打开文件，读取文件，比对文件

    FILE * fp;
    int len;
    char temp[512] = {};
    int result;
    char *p;
    if ( (fp = fopen("dict.txt", "r")) == NULL )
    {
        perror("fopen: 文件流打开失败\n");
        strcpy(msg->data, "服务器消息：文件流打开失败");
        send(acceptfd, msg, sizeof(MSG), 0);
        return -1;
    }
    //打印， 客户端要查询的单词
    len = strlen(word);
    printf("文件流打开成功, 单词: %s长度: %d \n", word, len);

    //读文件， 查单词
    while(fgets(temp, 512, fp) != NULL)
    {
        result = strncmp(temp, word, len);
        if (result < 0)
        {
            continue;
        }
        if (result > 0 || temp[len] != ' ')
        {
            break;
        }
        //找到了要查询的单词， dog     n.xxxxxxxxxxxxx
        p = temp + len;

        //去除头部空格
        while(*p == ' ')
        {
            p++;
        }

        //得到了注释部分， 跳跃所有空格
        strcpy(msg->data, p);

        //注释拷贝完毕，关闭文件
        fclose(fp);
        return 1;

    }// end of while
    fclose(fp);
    return 0;

}