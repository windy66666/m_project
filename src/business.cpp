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

int Business:: search_user(sqlite3 *db, MSG_INFO *msg_info)
{
    char sql[1024];
    char *errmsg = NULL;
    int is_exsit = 0;
    sprintf(sql, "select *from user_info where account_num='%s';", msg_info->account_num);
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

int Business:: do_register(int sockfd, MSG_INFO *msg_info)
{
    char sql[1024];
    char *errmsg = NULL;
    char buf[128];
    memset(buf, 0, 128);

    int res = search_user(m_db, msg_info);
    if (res == 1)
    {
        printf("用户已经存在\n");
        strcpy(buf, "用户已经存在\n");
        if(send(sockfd, buf, sizeof(buf), 0) == -1)
        {
            perror("send_search_result");
        }
        return -1;
    }

    sprintf(sql, "insert into user_info values('%s', '%s');", msg_info->account_num, msg_info->password);
    res = sqlite3_exec(m_db, sql, NULL, NULL, &errmsg);
    if (res != SQLITE_OK)
    {
        if (errmsg != NULL)
        {
            printf("SQL error:%s\n", errmsg);
            sqlite3_free(errmsg);
            errmsg = NULL;
        }
        return -1;
    }
    
    memset(buf, 0, sizeof(buf));
    strcpy(buf, "注册成功！\n");
    if(send(sockfd, buf, sizeof(buf), 0) == -1)
    {
        perror("send_register");
    }
    printf("%s注册成功\n", msg_info->account_num);
    return 1;
}

int Business:: do_login(int sockfd, MSG_INFO *msg_info)
{
    char sql[1024];
    char *errmsg = NULL;
    char buf[128];
    memset(buf, 0, 128);

    int res = search_user(m_db, msg_info);
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
    sprintf(sql, "select *from user_info where (account_num='%s' and password='%s');", msg_info->account_num, msg_info->password);
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
    
    user_client.insert({sockfd, string(msg_info->account_num)});
    online_user_count ++;
    printf("%s登录成功\n", msg_info->account_num);
    printf("当前在线人数：%d\n", online_user_count);
    for (const auto& pair : user_client)
    {
        cout << "客户端：" << pair.first << "账号： " << pair.second << endl;
        // printf("客户端：%d   账号：%s\n", pair.first, pair.second);
    }

    memset(buf, 0, sizeof(buf));
    strcpy(buf, "登录成功!\n");
    if(send(sockfd, buf, sizeof(buf), 0) == -1)
    {
        perror("send_login");
    }
    return 1;
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

void Business:: push_task(int *clientfd_ptr)
{
    g_thread_pool_push(m_pool, clientfd_ptr, NULL);
}

void Business:: thread_handle(gpointer data, gpointer user_data)
{
    Business *self = static_cast<Business*>(user_data);
    MSG_INFO msg_info;
    int clientfd = *(int *)data;
    ssize_t recv_size = recv(clientfd, (void *) &msg_info, sizeof(msg_info), 0);

    if (recv_size == 0)
    {
        printf("客户端 %d 断开连接\n", clientfd);
        close(clientfd);
        free(data);
        return;
    }else if(recv_size < 0){
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // 这是正常情况，不是错误！
        // 在边缘触发模式下，需要等待新的数据到达
        // printf("客户端 %d 暂时无数据，等待新数据\n", clientfd);
        return;  // 退出循环，但不关闭连接
        }else{
            perror("recv");
            close(clientfd);
            free(data);
            return;
        }
    }
    
    // 处理消息
    switch (msg_info.type)
    {
    case 1:
        printf("即将注册用户,账号:%s  密码:%s\n", msg_info.account_num, msg_info.password);
        self->do_register(clientfd, &msg_info);
        break;
    case 2:
        printf("即将登录用户,账号:%s  密码:%s\n", msg_info.account_num, msg_info.password);
        self->do_login(clientfd, &msg_info);
        break;
    // case Q:
    //     printf("即将查询单词：%s\n", msg_info.query_word);
    //     do_query(clientfd, (void *) &msg_info, db);
    //     break;
    // case H:
    //     printf("即将查询用户历史记录,账号:%s \n", msg_info.account_num);
    //     do_history(clientfd, (void *) &msg_info, db);
    //     break;
    default:
        break;
    }
}


