#include <stdio.h>
#include <sys/types.h>       
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "include/protocol.h"
#include <time.h>

#define N_buf_print 1024
// #define R 1    // user - register / query
// #define L 2    // user - login / history
// #define Q 3    // user - quit
// #define H 4    // user - history
// #define db_name "my.db" 

// typedef struct {
//     char account_num[128];
//     char password[128];
//     int type;
//     char query_word[128];
// }MSG_INFO;

void menu_show(char *buf_print, int state)
{
    if (state == 0)
    {
        memset(buf_print, 0, N_buf_print);
        sprintf(buf_print, 
            "********************************\n"
            "   1. 注册   2.登录   3.退出      \n"
            "********************************\n"
            "Please choose:\n");

        write(STDOUT_FILENO, buf_print, N_buf_print);
    }

    if (state == 1)
    {
        memset(buf_print, 0, N_buf_print);
        sprintf(buf_print, 
            "********************************\n"
            "   1. 好友列表请求   2.群组列表请求   3.好友添加请求  4.创建群聊请求   5.添加群聊请求  6.发送聊天消息请求  \n"
            "********************************\n"
            "Please choose:\n");

        write(STDOUT_FILENO, buf_print, N_buf_print);
    }
}

int do_register(int sockfd)
{
    REGISTET_MSG msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_header.msg_type = REGISTER_REQUEST;
    msg.msg_header.total_count = 1;
    msg.msg_header.msg_length = sizeof(REGISTET_MSG) - sizeof(MSG_HEADER);
    msg.msg_header.timestamp = time(NULL);

    printf("---请输入以下注册信息---\n");
    printf("账号：");
    scanf("%s", msg.user_account);
    printf("密码：");
    scanf("%s", msg.user_account);
    if(send(sockfd, (void *)&msg, sizeof(msg), 0) == -1)
    {
        perror("send_register");
    }

    char buf[128];
    ssize_t res = recv(sockfd, buf, sizeof(buf), 0);
    if (res == 0)
    {
        printf("服务端断开连接\n");
        return -1;
    }else if(res < 0)
    {
        perror("recv_register\n");
        return -1;
    }else{
        write(STDOUT_FILENO, buf, sizeof(buf));
    }
    return 0;
}

int do_login(int sockfd)
{
    char buf[128];
    char buf_pipei[128];

    LOGIN_MSG msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_header.msg_type = LOGIN_REQUEST;
    msg.msg_header.total_count = 1;
    msg.msg_header.msg_length = sizeof(LOGIN_MSG) - sizeof(MSG_HEADER);
    msg.msg_header.timestamp = time(NULL);

    printf("账号：");
    scanf("%s", msg.user_account);
    printf("密码：");
    scanf("%s", msg.user_password);

    printf("接收消息头大小：%ld\n", sizeof(msg.msg_header));
    printf("消息类型：%d, 消息长度: %d, 消息时间戳:%d, 消息数量：%d\n", msg.msg_header.msg_type, msg.msg_header.msg_length, msg.msg_header.timestamp, msg.msg_header.total_count);
    if(send(sockfd, (void *)&msg, sizeof(msg), 0) == -1)
    {
        perror("send_login");
    }

    ssize_t res = recv(sockfd, buf, sizeof(buf), 0);
    if (res == 0)
    {
        printf("服务端断开连接\n");
        close(sockfd);
        return -1;
    }else if(res < 0)
    {
        perror("recv_login\n");
        return -1;
    }else{
        write(STDOUT_FILENO, buf, sizeof(buf));
    }

    memset(buf_pipei, 0, 128);
    strcpy(buf_pipei, "登录成功!\n");
    if (strcmp(buf, buf_pipei) == 0)
    {
        return 0;
    }
    
    return -1;
}

// int do_query(int sockfd, MSG_INFO *msg_info)
// {
//     msg_info->type = Q;
//     char buf[1024];
//     memset(buf, 0, sizeof(buf));
//     printf("请输入要查询的单词：");
//     scanf("%s", msg_info->query_word);

//     if(send(sockfd, (void *)msg_info, sizeof(*msg_info), 0) == -1)
//     {
//         perror("send_query");
//     }

//     ssize_t res = recv(sockfd, buf, sizeof(buf), 0);
//     if (res == 0)
//     {
//         printf("服务端断开连接\n");
//         close(sockfd);
//         return -1;
//     }else if(res < 0)
//     {
//         perror("recv_query_info\n");
//         return -1;
//     }else{
//         write(STDOUT_FILENO, buf, sizeof(buf));
//     }

//     return 0;
// }

// int do_history(int sockfd, MSG_INFO *msg_info)
// {
//     msg_info->type = H;
//     char buf[2048];
//     memset(buf, 0, sizeof(buf));

//     if(send(sockfd, (void *)msg_info, sizeof(*msg_info), 0) == -1)
//     {
//         perror("send_history_query");
//     }

//     ssize_t res = recv(sockfd, buf, sizeof(buf), 0);
//     if (res == 0)
//     {
//         printf("服务端断开连接\n");
//         close(sockfd);
//         return -1;
//     }else if(res < 0)
//     {
//         perror("recv_history_info\n");
//         return -1;
//     }else{
//         write(STDOUT_FILENO, buf, sizeof(buf));
//     }
// }

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage:%s serverip port\n", argv[0]);
        return -1;
    }

    int domain = AF_INET;
    int type = SOCK_STREAM;

    int sockfd = socket(domain, type, 0);
    if (sockfd == -1)
    {
        perror("socket");
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));
    
    if(connect(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1){
        perror("connect");
        close(sockfd);
        return -1;
    }else{
        printf("connect successfully!\n");
    }

    // MSG_INFO msg_info;
    // printf("%ld", sizeof(msg_info));
    char buf_print[N_buf_print];
    int state = 0;             //记录用户现在处于哪个状态，来决定显示什么内容（注册、登录、退出 / 词典使用界面） 

    while (1)
    {
        menu_show(buf_print, state);

        int choice;
        if (state == 0)
        {
            scanf("%d", &choice);
            switch (choice)
            {
            case 1:
                printf("register\n");
                do_register(sockfd);
                break;
            case 2:
                printf("login\n");
                if(do_login(sockfd) == 0)
                {
                    state = 1;
                }
                break;
            case 3:
                printf("Exit successfully!\n");
                close(sockfd);
                exit(0);
                break;
            default:
                break;
            }
        }else{
            scanf("%d", &choice);
            switch (choice)
            {
            case 1:
                printf("query:\n");
                // do_query(sockfd, &msg_info);
                break;
            case 2:
                printf("history:\n");
                // do_history(sockfd, &msg_info);
                break;
            case 7:
                printf("Exit successfully!\n");
                close(sockfd);
                exit(0);
                break;
            case 0:
                state = 0;
                break;
            default:
                break;
            }
        }
    }
    
    return 0;
}
