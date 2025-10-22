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

int receive_message_header(int sockfd, MSG_HEADER *header)
{
    ssize_t recv_size = recv(sockfd, header, sizeof(MSG_HEADER), 0);

    if (recv_size == 0)
    {
        printf("服务端断开连接\n");
        return -1;
    }else if(recv_size < 0){
        perror("recv_login\n");
        return -1;  
    }
    
    // printf("消息类型：%d, 消息长度: %d, 消息时间戳:%d, 消息数量：%d\n", header->msg_type, header->msg_length, header->timestamp, header->total_count);
    return 0;
}

int handle_NormalResponse_message(int clientfd, MSG_HEADER *msg_header)
{
    int remain_data = msg_header->msg_length;

    if (remain_data != sizeof(RESPONSE_MSG) - sizeof(MSG_HEADER))
    {
        printf("响应消息长度错误\n");
        return -1;
    }
    
    // 接受剩余数据
    RESPONSE_MSG response_msg;
    memcpy(&response_msg.msg_header, msg_header, sizeof(MSG_HEADER));

    char *msg_data = (char*)(&response_msg) + sizeof(MSG_HEADER); 
    int recv_size = recv(clientfd, msg_data, remain_data,  MSG_WAITALL);
    if (recv_size != remain_data)
    {
        printf("响应消息不完整\n");
        return -1;
    }

    if (response_msg.success_flag == 0)
    {
        printf("%s", response_msg.response);
        return -1;
    }else{
        printf("%s", response_msg.response);
        return 0;
    }
}

// int do_register(int sockfd)
// {
//     REGISTET_MSG msg;
//     memset(&msg, 0, sizeof(msg));
//     msg.msg_header.msg_type = REGISTER_REQUEST;
//     msg.msg_header.total_count = 1;
//     msg.msg_header.msg_length = sizeof(REGISTET_MSG) - sizeof(MSG_HEADER);
//     msg.msg_header.timestamp = time(NULL);

//     printf("---请输入以下注册信息---\n");
//     printf("账号：");
//     scanf("%s", msg.user_account);
//     printf("密码：");
//     scanf("%s", msg.user_account);
//     if(send(sockfd, (void *)&msg, sizeof(msg), 0) == -1)
//     {
//         perror("send_register");
//     }

//     MSG_HEADER msg_header;
//     if(receive_message_header(sockfd, &msg_header) != 0)
//     {
//         perror("登录时接收服务端消息失败\n");
//     }


//     if (msg_header.msg_type == NORMAL_RESPONSE)
//     {
//         if ()
//         {
//             /* code */
//         }
        
//     }

//     return 0;
// }

int do_login(int sockfd)
{
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

    // printf("接收消息头大小：%ld\n", sizeof(msg.msg_header));
    // printf("消息类型：%d, 消息长度: %d, 消息时间戳:%d, 消息数量：%d\n", msg.msg_header.msg_type, msg.msg_header.msg_length, msg.msg_header.timestamp, msg.msg_header.total_count);
    if(send(sockfd, (void *)&msg, sizeof(msg), 0) == -1)
    {
        perror("send_login");
    }

    MSG_HEADER msg_header;
    if(receive_message_header(sockfd, &msg_header) != 0)
    {
        perror("登录时接收服务端消息失败\n");
    }

    // printf("消息头类型：%d\n", msg_header.msg_type);
    if (msg_header.msg_type == NORMAL_RESPONSE)
    {
        return handle_NormalResponse_message(sockfd, &msg_header);
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
                // do_register(sockfd);
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
