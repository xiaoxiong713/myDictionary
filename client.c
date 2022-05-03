#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define N 32
#define R 1 //注册
#define L 2 //login
#define Q 3 // query
#define H 4 // history

typedef struct
{
    int type;
    char name[N];
    char data[256];
} MSG;


int do_register(int sockfd, MSG *msg)
{
    printf("客户端开始执行注册函数do_register...\n");
    msg->type = R;
    printf("请输入姓名：\n");
    scanf("%s", msg->name);
    getchar();
    printf("请输入密码: \n");
    scanf("%s", msg->data);
    getchar();
    printf("name:%s, passwd:%s\n", msg->name, msg->data);//调试代码
    //一定是大写msg的大小， 小写的永远是指针的大小，4个字节

    if ( send(sockfd, msg, sizeof(MSG), 0) < 0 ) //发送
    {
        printf("fail to send.\n");
        return -1;
    }
    if (recv(sockfd, msg, sizeof(MSG), 0) < 0) //等待接收
    {
        printf("fail to recvice.\n");
        return -1;
    }

    //ok! or usr already exist.
    printf("%s\n", msg->data); // 打印回复
    return 0;
}

int do_login(int sockfd, MSG *msg)
{
    printf("客户端开始执行登陆函数do_login\n");
    msg->type = L;
    printf("请输入姓名：\n");
    scanf("%s", msg->name);
    getchar();
    printf("请输入密码: \n");
    scanf("%s", msg->data);
    getchar();

    //发送和回收
    if ( send(sockfd, msg, sizeof(MSG), 0) < 0 ) //发送
    {
        printf("fail to send.\n");
        return -1;
    }
    if (recv(sockfd, msg, sizeof(MSG), 0) < 0) //等待接收
    {
        printf("fail to recvice.\n");
        return -1;
    }
    if (strncmp(msg->data, "OK", 3) == 0)//字符串比较函数
    {
        printf("欢迎登陆！\n");
        return 1;//根据这个1,客服端判断跳转二级菜单
    }
    else
    {
        //登陆失败，服务器返回错误信息
        printf("%s\n", msg->data);
    }
    return 0;
}

int do_query(int sockfd, MSG *msg)
{
    printf("客户端开始执行查询函数do_query\n");
    msg->type = Q;
    while(1)
    {
        printf("输入你要查询的单词(# exit)-> ");
        scanf("%s", msg->data);
        if (strncmp(msg->data, "#", 1) == 0)
        {
            //用户输入#，表示退出
            break;//返回上一级菜单
        }
        //单词发送给服务器,等待结果
        if ( send(sockfd, msg, sizeof(MSG), 0) < 0 ) //发送
        {
            printf("fail to send.\n");
            return -1;
        }
        if (recv(sockfd, msg, sizeof(MSG), 0) < 0) //等待接收
        {
            printf("fail to recvice.\n");
            return -1;
        }
        printf("->\t%s\n", msg->data);//打印返回的数据

    }//end of while


    return 0;
}
int do_history(int sockfd, MSG *msg)
{
    printf("客户端开始执行查询历史记录函数do_history\n");
    msg->type = H;
    send(sockfd, msg, sizeof(MSG), 0);

    //接收服务器的回复
    while(1)
    {   

        //收到服务器的回调函数发来的msg，回调函数会执行多次，则这个循环执行多次

        recv(sockfd, msg, sizeof(MSG), 0); //阻塞接收
        if (msg->data[0] == '\0')
        {
            printf("查询结束/没有数据\n");
            break;
        }
   
        //打印历史记录
        printf("%s\n", msg->data);
    }//end of while


    return 0;
}

//======================================================  main  ===================================================
// ./server 192.168.3.196  10000
int main(int argc, char const *argv[])
{

    if (argc != 3)
    {
        printf("Usage: %s server port,\n", argv[0]);
        return -1;
    }
    //======================================  参数判断 ==============================================================
    int sockfd = -1 ;
    struct sockaddr_in serveraddr;
    MSG msg;
    memset(&msg, 0, sizeof(msg));

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("fail to socket.\n");
        return -1;
    }

    bzero(&serveraddr, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]); // 把ip地址转为二进制
    serveraddr.sin_port = htons(atoi(argv[2]));


    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        perror("fail to connect.\n");
        return -1;
    }

    //========================================end of 创建流式套接字,连接服务器=========================================

    int n;

    while(1)
    {
        printf("************************************\n");
        printf("* 1.register    2.login       3.quit*\n");
        printf("************************************\n");
        printf("please choose:");
        scanf("%d", &n);

        getchar();// clear

        switch(n)
        {
        case 1:
            do_register(sockfd, &msg);
            break;
        case 2:
            if(do_login(sockfd, &msg) == 1)
            {
                //登陆成功
                goto next;
            }
            break;
        case 3:
            close(sockfd);
            exit(0);
            break;
        default:
            printf("Invalid data cmd.\n");
        }
    }

next:
    while(1)
    {
        printf("************************************\n");
        printf("* 1.查询单词     2.查询记录      3.退出*\n");
        printf("************************************\n");
        printf("please choose:\n");
        scanf("%d", &n);
        getchar();

        switch(n)
        {
        case 1:
            do_query(sockfd, &msg);
            break;
        case 2:
            do_history(sockfd, &msg);
            break;
        case 3:
            close(sockfd);
            exit(0);
            break;
        default:
            printf("Invalid data cmd.\n");
        }
    }
    return 0;
}