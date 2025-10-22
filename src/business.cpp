#include "business.h"

#define db_name "my.db" 


int Business:: callback(void *arg, int col, char** value, char** key)
{
    for (int i = 0; i < col; i++)
    {
        // printf("%s:%s\n", key[i], value[i]);
        *(int *)arg = 1;
    }
    return SQLITE_OK;
}

int Business:: callback_query(void *arg, int col, char** value, char** key)
{
    for (int i = 0; i < col; i++)
    {
        // printf("%s:%s\n", key[i], value[i]);
        // *(char *)arg = *value[i];
        strcpy((char *)arg, value[i]);
    }
    return SQLITE_OK;
}

int Business:: search_user(sqlite3 *db, LOGIN_MSG *login_msg)
{
    char sql[1024];
    char *errmsg = NULL;
    int is_exsit = 0;
    sprintf(sql, "select *from user_info where account_num='%s';", login_msg->user_account);
    int res = sqlite3_exec(db, sql, callback, (void *)&is_exsit, &errmsg);
    if (res != SQLITE_OK)
    {
        if (errmsg != NULL)
        {
            printf("SQL search error:%s\n", errmsg);
            sqlite3_free(errmsg);
            errmsg = NULL;
        }
        return -1;
    }
    return is_exsit;
}

// int Business:: do_register(int sockfd, MSG_INFO *msg_info)
// {
//     char sql[1024];
//     char *errmsg = NULL;
//     char buf[128];
//     memset(buf, 0, 128);

//     // int res = search_user(m_db, msg_info);
//     // if (res == 1)
//     // {
//     //     printf("用户已经存在\n");
//     //     strcpy(buf, "用户已经存在\n");
//     //     if(send(sockfd, buf, sizeof(buf), 0) == -1)
//     //     {
//     //         perror("send_search_result");
//     //     }
//     //     return -1;
//     // }

//     // sprintf(sql, "insert into user_info values('%s', '%s');", msg_info->account_num, msg_info->password);
//     // res = sqlite3_exec(m_db, sql, NULL, NULL, &errmsg);
//     // if (res != SQLITE_OK)
//     // {
//     //     if (errmsg != NULL)
//     //     {
//     //         printf("SQL error:%s\n", errmsg);
//     //         sqlite3_free(errmsg);
//     //         errmsg = NULL;
//     //     }
//     //     return -1;
//     // }
    
//     memset(buf, 0, sizeof(buf));
//     strcpy(buf, "注册成功！\n");
//     if(send(sockfd, buf, sizeof(buf), 0) == -1)
//     {
//         perror("send_register");
//     }
//     printf("%s注册成功\n", msg_info->account_num);
//     return 1;
// }

int Business:: do_login(int sockfd, LOGIN_MSG *login_msg)
{
    char sql[1024];
    char *errmsg = NULL;
    char buf[128];
    memset(buf, 0, 128);

    int res = search_user(m_db, login_msg);
    if (res == 0)
    {
        printf("用户不存在\n");
        strcpy(buf, "用户不存在\n");
        if(send(sockfd, buf, sizeof(buf), 0) == -1)
        {
            perror("send_search_result");
        }
        return -1;
    }

    int is_right = 0;
    sprintf(sql, "select *from user_info where (account_num='%s' and password='%s');", login_msg->user_account, login_msg->user_password);
    res = sqlite3_exec(m_db, sql, callback, (void *)&is_right, &errmsg);
    if (res != SQLITE_OK)
    {
        if (errmsg != NULL)
        {
            printf("SQL search error:%s\n", errmsg);
            sqlite3_free(errmsg);
            errmsg = NULL;
        }
        return -1;
    }
    
    if (is_right == 0)
    {
        printf("密码错误\n");
        strcpy(buf, "密码错误\n");
        if(send(sockfd, buf, sizeof(buf), 0) == -1)
        {
            perror("send_login_check_result");
        }
        return -1;
    }
    
    user_client[sockfd] = string(login_msg->user_account);
    online_user_count = user_client.size();
    
    printf("%s登录成功\n", login_msg->user_account);
    printf("------------------------------\n");
    printf("当前在线人数：%d\n", online_user_count);
    for (const auto& pair : user_client)
    {
        cout << "客户端:" << pair.first << "\t" << "账号:" << pair.second << endl;
    }
    printf("------------------------------\n");

    memset(buf, 0, sizeof(buf));
    strcpy(buf, "登录成功!\n");
    if(send(sockfd, buf, sizeof(buf), 0) == -1)
    {
        perror("send_login");
    }
    return 0;
}


Business:: Business()
{
    m_pool = g_thread_pool_new(thread_handle, this, 5, FALSE, NULL);

    if(sqlite3_open(db_name, &m_db) != SQLITE_OK)
    {
        perror("sqlite3_open");
    }
}

Business:: ~Business()
{

}

void Business:: push_task(int *clientfd)
{
    g_thread_pool_push(m_pool, clientfd, NULL);
}


void Business:: thread_handle(gpointer data, gpointer user_data)
{
    Business *self = static_cast<Business*>(user_data);
    // MSG_INFO msg_info;
    int clientfd = *(int *)data;
    // printf("客户端clientfd:%d\n", clientfd);
    MSG_HEADER msg_header;
    // 先清空内存，避免旧数据干扰
    memset(&msg_header, 0, sizeof(MSG_HEADER));
    if(self->receive_message_header(clientfd, &msg_header, self) == -1)
    {
        printf("接收消息头失败\n");
        return;
    }

    // 根据消息类型处理消息
    switch (msg_header.msg_type)
    {
    case REGISTER_REQUEST:
        // printf("即将注册用户,账号:%s  密码:%s\n", msg_info.account_num, msg_info.password);
        // self->do_register(clientfd, &msg_info);
        break;
    case LOGIN_REQUEST:
        // printf("-------------------执行登录-----------\n");
        self->handle_login_message(clientfd, &msg_header);
        break;
    // case 3:
    //     printf("即将查询单词：%s\n", msg_info.query_word);
    //     self.do_quit(clientfd);
    //     break;
    // case H:
    //     printf("即将查询用户历史记录,账号:%s \n", msg_info.account_num);
    //     do_history(clientfd, (void *) &msg_info, db);
    //     break;
    default:
        break;
    }
}


int Business:: receive_message_header(int sockfd, MSG_HEADER *header, Business *business)
{
    Business *self = business;
    ssize_t recv_size = recv(sockfd, header, sizeof(MSG_HEADER), 0);
    // printf("接收消息头大小：%ld\n", recv_size);
    // printf("理论消息头大小：%ld\n", sizeof(MSG_HEADER));
    if (recv_size == 0)
    {
        // printf("客户端 %d 断开连接\n", sockfd);
        close(sockfd);

        cout << "客户端：" << self->user_client[sockfd] << " 断开连接" << endl;
        // printf("%s客户端断开连接\n", self->user_client[clientfd]);
        self->user_client.erase(sockfd);
        self->online_user_count = self->user_client.size();
        printf("------------------------------\n");
        printf("当前在线人数：%d\n", self->online_user_count);
        for (const auto& pair : self->user_client)
        {
            cout << "客户端:" << pair.first << "\t" << "账号:" << pair.second << endl;
        }
        printf("------------------------------\n");
        return 1;
    }else if(recv_size < 0){
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // 这是正常情况，不是错误！
        // 在边缘触发模式下，需要等待新的数据到达
        // printf("客户端 %d 暂时无数据，等待新数据\n", clientfd);
        return 0;  // 退出循环，但不关闭连接
        }else{
            perror("recv");
            close(sockfd);
            return -1;
        }
    }
    // printf("消息类型：%d, 消息长度: %d, 消息时间戳:%d, 消息数量：%d\n", header->msg_type, header->msg_length, header->timestamp, header->total_count);
    return 0;
}


int Business:: handle_login_message(int clientfd, MSG_HEADER *msg_header)
{
    int remain_data = msg_header->msg_length;

    if (remain_data != sizeof(LOGIN_MSG) - sizeof(MSG_HEADER))
    {
        printf("登录消息长度错误\n");
        return -1;
    }
    
    // 接受剩余数据
    LOGIN_MSG login_msg;
    memcpy(&login_msg.msg_header, msg_header, sizeof(MSG_HEADER));

    char *msg_data = (char*)(&login_msg) + sizeof(MSG_HEADER); 
    int recv_size = recv(clientfd, msg_data, remain_data,  MSG_WAITALL);
    if (recv_size != remain_data)
    {
        printf("登录消息不完整\n");
        return -1;
    }

    printf("即将登录用户,账号:%s  密码:%s\n", login_msg.user_account, login_msg.user_password);
    if(do_login(clientfd, &login_msg) != 0)
    {
        printf("登录错误\n");
        return -1;
    }

    return 0;
}