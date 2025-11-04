#include "business.h"

Business:: Business(data_handler * DataHandler)
{
    m_pool = g_thread_pool_new(thread_handle, this, 5, FALSE, NULL);
    m_db_handler = DataHandler;
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

    printf("msg_type:%d\n", msg_header.msg_type);
    // 根据消息类型处理消息
    switch (msg_header.msg_type)
    {
    case REGISTER_REQUEST:
        // printf("即将注册用户,账号:%s  密码:%s\n", msg_info.account_num, msg_info.password);
        self->handle_register_message(clientfd, &msg_header);
        break;
    case LOGIN_REQUEST:
        // printf("-------------------执行登录-----------\n");
        self->handle_login_message(clientfd, &msg_header);
        break;
    case ACCOUNT_QUERY_REQUEST:
        printf("查询用户\n");
        self->handle_AccountQuery_message(clientfd, &msg_header);
        break;
    case ADD_FRIEND_REQUEST:
        self->handle_addfriend_message(clientfd, &msg_header);
        break;
    case ACCEPT_FRIEND_ASK:
        self->handle_acceptfriend_message(clientfd, &msg_header, ACCEPT_FRIEND_ASK);
        break;
    case REJECT_FRIEND_ASK:
        self->handle_acceptfriend_message(clientfd, &msg_header, REJECT_FRIEND_ASK);
        break;
    default:
        break;
    }
}


int Business:: receive_message_header(int sockfd, MSG_HEADER *header, Business *business)
{
    Business *self = business;
    ssize_t recv_size = recv(sockfd, header, sizeof(MSG_HEADER), 0);
    printf("接收消息头大小：%ld\n", recv_size);
    printf("理论消息头大小：%ld\n", sizeof(MSG_HEADER));
    if (recv_size == 0)
    {
        // printf("客户端 %d 断开连接\n", sockfd);
        close(sockfd);
        bool flag = false;
        USER_INFO user_info;

        for (const auto& pair : self->user_client)
        {
            if (pair.second == sockfd)
            {
                cout << "客户端：" << pair.first.user_account << " 断开连接" << endl;
                self->user_client.erase(pair.first);
                flag = true;

                user_info.status = 0;
                strcpy(user_info.user_account, pair.first.user_account);
                strcpy(user_info.user_name, pair.first.user_name);
                break;
            }
        }
        
        if (!flag)
        {
            cout << "客户端：" << sockfd << " 断开连接" << endl;
        }
        
        // printf("%s客户端断开连接\n", self->user_client[clientfd]);
        // self->user_client.erase(sockfd);

        //  拉取用户好友列表
        FRIEND_LIST_MSG *friend_list_msg = nullptr;
        if(m_db_handler->get_friendlist(user_info.user_account, &friend_list_msg) != 0) 
        {
            free(friend_list_msg);
            return -1;
        }
        //  构造好友离线通知消息
        FRIEND_LIST_MSG* friend_outline_notice = nullptr;
        if(m_db_handler->consturct_friend_notice_msg(&user_info, &friend_outline_notice) != 0) 
        {
            free(friend_outline_notice);
            return -1;
        }

        for (int i = 0; i < friend_list_msg->msg_header.total_count; i++)
        {
            auto it = user_client.find(friend_list_msg->friends[i]);

            if (it != user_client.end())
            {
                friend_list_msg->friends[i].status = 1;
                auto& friend_client = it->second;
                if(send(friend_client, friend_outline_notice, sizeof(MSG_HEADER)+friend_outline_notice->msg_header.msg_length, 0) == -1)
                {
                    printf("向好友：%s 推送用户:%s 离线提醒消息失败\n", friend_list_msg->friends[i].user_account, user_info.user_account);
                }

            }else{
                friend_list_msg->friends[i].status = 0;
            }
        }

        free(friend_list_msg);
        free(friend_outline_notice);

        self->online_user_count = self->user_client.size();
        printf("------------------------------\n");
        printf("当前在线人数：%d\n", self->online_user_count);
        for (const auto& pair : self->user_client)
        {
            cout << "客户端:" << pair.second << "\t" << "账号:" << pair.first.user_account << "\t" << "昵称:" << pair.first.user_name << endl;
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

template<typename T>
int Business:: receive_remain_message(int clientfd, MSG_HEADER *msg_header, T* total_msg)
{
    int remain_data = msg_header->msg_length;

    if (remain_data != sizeof(T) - sizeof(MSG_HEADER))
    {
        printf("消息长度错误\n");
        return -1;
    }
    
    memcpy(&total_msg->msg_header, msg_header, sizeof(MSG_HEADER));

    char *msg_data = (char*)(total_msg) + sizeof(MSG_HEADER); 
    int recv_size = recv(clientfd, msg_data, remain_data, MSG_WAITALL);
    if (recv_size != remain_data)
    {
        printf("消息不完整\n");
        return -1;
    }

    return 0;
}

int Business:: handle_login_message(int clientfd, MSG_HEADER *msg_header)
{
    USER_INFO my_user_info;

    // 接受剩余数据
    LOGIN_MSG login_msg;

    if(receive_remain_message(clientfd, msg_header, &login_msg) != 0)
    {
        return -1;
    }

    USER_QUERY_RESPONSE_MSG response_msg;
    memset(&response_msg, 0, sizeof(response_msg));
    response_msg.msg_header.msg_type = LOGIN_RESPONSE;
    response_msg.msg_header.msg_length = sizeof(response_msg) - sizeof(MSG_HEADER);
    response_msg.msg_header.timestamp = time(NULL);

    printf("即将登录用户,账号:%s  密码:%s\n", login_msg.user_account, login_msg.user_password);

    if(do_login(clientfd, &login_msg, &response_msg, &my_user_info)!= 0)
    {
        if(send(clientfd, &response_msg, sizeof(response_msg), 0) == -1)
        {
            printf("向用户:%s 发送登录响应失败", login_msg.user_account);
        }
        return -1;
    }

    //  拉取用户好友申请列表
    FRIEND_ASK_NOTICE_MSG *friend_ask_notice_msgs = nullptr;  
    if(m_db_handler->get_addfriend_ask(login_msg.user_account, &friend_ask_notice_msgs) != 0)  // 注意！！！ 这里得是二级指针，因为需要修改其地址值
    {
        free(friend_ask_notice_msgs);
        return -1;
    }

    if(send(clientfd, friend_ask_notice_msgs, sizeof(MSG_HEADER)+friend_ask_notice_msgs->msg_header.msg_length, 0) == -1)
    {
        printf("向用户:%s 推送好友请求通知消息失败\n", login_msg.user_account);
    }

    free(friend_ask_notice_msgs);

    //  构造好友上线通知消息
    FRIEND_LIST_MSG* friend_online_notice = nullptr;
    if(m_db_handler->consturct_friend_notice_msg(&my_user_info, &friend_online_notice) != 0) 
    {
        free(friend_online_notice);
        return -1;
    }
    //  拉取用户好友列表
    FRIEND_LIST_MSG *friend_list_msg = nullptr;
    if(m_db_handler->get_friendlist(login_msg.user_account, &friend_list_msg) != 0) 
    {
        free(friend_list_msg);
        return -1;
    }

    for (int i = 0; i < friend_list_msg->msg_header.total_count; i++)
    {
        auto it = user_client.find(friend_list_msg->friends[i]);

        if (it != user_client.end())
        {
            friend_list_msg->friends[i].status = 1;
            auto& friend_client = it->second;
            if(send(friend_client, friend_online_notice, sizeof(MSG_HEADER)+friend_online_notice->msg_header.msg_length, 0) == -1)
            {
                printf("向好友：%s 推送用户：%s 上线通知消息失败\n", friend_list_msg->friends[i].user_account, login_msg.user_account);
            }

        }else{
            friend_list_msg->friends[i].status = 0;
        }
    }

    if(send(clientfd, friend_list_msg, sizeof(MSG_HEADER)+friend_list_msg->msg_header.msg_length, 0) == -1)
    {
        printf("向用户:%s 推送好友请求通知消息失败\n", login_msg.user_account);
    }

    free(friend_list_msg);
    free(friend_online_notice);

    //  发送用户登录响应
    if(send(clientfd, &response_msg, sizeof(response_msg), 0) == -1)
    {
        printf("向用户:%s 发送登录响应失败", login_msg.user_account);
    }

    printf("%s登录成功\n", login_msg.user_account);
    printf("------------------------------\n");
    printf("当前在线人数：%d\n", online_user_count);
    for (const auto& pair : user_client)
    {
        cout << "客户端:" << pair.second << "\t" << "账号:" << pair.first.user_account << "\t" << "昵称:" << pair.first.user_name << endl;
    }
    printf("------------------------------\n");

    return 0;
}

int Business:: handle_register_message(int clientfd, MSG_HEADER *msg_header)
{
    // 接受剩余数据
    REGISTET_MSG register_msg;

    if(receive_remain_message(clientfd, msg_header, &register_msg) != 0)
    {
        return -1;
    }

    RESPONSE_MSG response_msg;
    memset(&response_msg, 0, sizeof(response_msg));
    response_msg.msg_header.msg_type = REGISTER_RESPONSE;
    response_msg.msg_header.msg_length = sizeof(RESPONSE_MSG) - sizeof(MSG_HEADER);
    response_msg.msg_header.timestamp = time(NULL);

    printf("即将注册用户,账号:%s  密码:%s\n", register_msg.user_account, register_msg.user_password);
    do_register(clientfd, &register_msg, &response_msg);

    if(send(clientfd, &response_msg, sizeof(response_msg), 0) == -1)
    {
        printf("向用户:%s 发送注册响应失败", register_msg.user_account);
    }

    return 0;
}

int Business:: handle_AccountQuery_message(int clientfd, MSG_HEADER *msg_header)
{
    // 接受剩余数据
    ACCOUNT_QUERY_MSG query_msg;

    if(receive_remain_message(clientfd, msg_header, &query_msg) != 0)
    {
        return -1;
    }

    USER_QUERY_RESPONSE_MSG response_msg;
    memset(&response_msg, 0, sizeof(response_msg));
    response_msg.msg_header.msg_type = USER_QUERY_RESPONSE;
    response_msg.msg_header.msg_length = sizeof(response_msg) - sizeof(MSG_HEADER);
    response_msg.msg_header.timestamp = time(NULL);

    printf("%s即将查询用户,查询账号:%s \n", query_msg.user_account, query_msg.query_account);
    do_query(clientfd, &query_msg, &response_msg);

    if(send(clientfd, &response_msg, sizeof(response_msg), 0) == -1)
    {
        printf("向用户:%s 发送查询响应失败", query_msg.user_account);
    }

    return 0;
}

int Business:: handle_addfriend_message(int clientfd, MSG_HEADER *msg_header)
{
    // 接受剩余数据
    ADD_FRIEND_MSG add_friend_msg;

    if(receive_remain_message(clientfd, msg_header, &add_friend_msg) != 0)
    {
        return -1;
    }

    RESPONSE_MSG response_msg;
    memset(&response_msg, 0, sizeof(response_msg));
    response_msg.msg_header.msg_type = FRIEND_ADD_RESPONSE;
    response_msg.msg_header.msg_length = sizeof(RESPONSE_MSG) - sizeof(MSG_HEADER);
    response_msg.msg_header.timestamp = time(NULL);

    if (strcmp(add_friend_msg.user_account, add_friend_msg.friend_account) == 0)
    {
        strcpy(response_msg.response, "无法添加自己的账号为好友\n");
        response_msg.success_flag = 0;
        printf("%s 添加用户 %s 失败\n", add_friend_msg.user_account, add_friend_msg.friend_account);
        send(clientfd, &response_msg, sizeof(response_msg), 0);
        return -1;
    }
    
    // strcpy(add_friend_msg.user_account, user_client[clientfd]);
    printf("用户%s 即将添加好友,账号:%s\n", add_friend_msg.user_account, add_friend_msg.friend_account);
    do_addfriend(clientfd, &add_friend_msg, &response_msg);
    // do_register(clientfd, &register_msg, &response_msg);

    if(send(clientfd, &response_msg, sizeof(response_msg), 0) == -1)
    {
        printf("向用户:%s 发送添加好友响应失败", add_friend_msg.user_account);
    }

    //  拉取用户好友单条申请消息
    FRIEND_ASK_NOTICE_MSG *friend_ask_notice_msg = nullptr;  
    if(m_db_handler->consutrct_friend_ask_notice_msg(&add_friend_msg, &friend_ask_notice_msg) != 0)  
    {
        free(friend_ask_notice_msg);
        return -1;
    }

    // 向添加好友的用户发送  好友申请单条消息
    if(send(clientfd, friend_ask_notice_msg, sizeof(MSG_HEADER)+friend_ask_notice_msg->msg_header.msg_length, 0) == -1)
    {
        printf("向用户:%s 推送好友添加通知消息失败\n", add_friend_msg.user_account);
    }
    
    USER_INFO friend_info;
    strcpy(friend_info.user_account, friend_ask_notice_msg->asks[0].friend_acccount);
    strcpy(friend_info.user_name, friend_ask_notice_msg->asks[0].friend_name);

    // 向被用户添加的好友（如果在线的话）发送  好友申请单条消息
    auto it = user_client.find(friend_info);

    if (it != user_client.end())
    {
        friend_info.status = 1;
        auto& friend_client = it->second;
        if(send(friend_client, friend_ask_notice_msg, sizeof(MSG_HEADER)+friend_ask_notice_msg->msg_header.msg_length, 0) == -1)
        {
            printf("向用户:%s 推送好友添加通知消息失败\n", add_friend_msg.friend_account);
        }

    }else{
        friend_info.status = 0;
    }

    free(friend_ask_notice_msg);
    return 0;
}

int Business:: handle_acceptfriend_message(int clientfd, MSG_HEADER *msg_header, int choice)
{
    // 接受剩余数据
    ADD_FRIEND_MSG add_friend_msg;

    if(receive_remain_message(clientfd, msg_header, &add_friend_msg) != 0)
    {
        return -1;
    }

    RESPONSE_MSG response_msg;
    memset(&response_msg, 0, sizeof(response_msg));

    if(choice == ACCEPT_FRIEND_ASK){
        response_msg.msg_header.msg_type = FRIEND_ACCPET_RESPONSE;
    }else{
        response_msg.msg_header.msg_type = FRIEND_REJECT_RESPONSE;
    }

    response_msg.msg_header.msg_length = sizeof(RESPONSE_MSG) - sizeof(MSG_HEADER);
    response_msg.msg_header.timestamp = time(NULL);

    if (choice == ACCEPT_FRIEND_ASK)
    {
        printf("用户%s 同意好友添加请求,账号:%s\n", add_friend_msg.friend_account, add_friend_msg.user_account);
    }else{
        printf("用户%s 拒绝好友添加请求,账号:%s\n", add_friend_msg.friend_account, add_friend_msg.user_account);
    }

    do_acceptfriend(clientfd, &add_friend_msg, &response_msg, choice);
    // do_register(clientfd, &register_msg, &response_msg);

    if(send(clientfd, &response_msg, sizeof(response_msg), 0) == -1)
    {
        printf("向用户:%s 发送添加好友响应失败", add_friend_msg.user_account);
    }


    if(choice == ACCEPT_FRIEND_ASK){
        ACCOUNT_QUERY_MSG user_query_msg, friend_query_msg;
        strcpy(user_query_msg.query_account, add_friend_msg.friend_account);
        stpcpy(user_query_msg.user_account, add_friend_msg.friend_account);

        strcpy(friend_query_msg.query_account, add_friend_msg.user_account);
        stpcpy(friend_query_msg.user_account, add_friend_msg.friend_account);

        USER_QUERY_RESPONSE_MSG user_query_response_msg, friend_query_response_msg;
        m_db_handler->query_user(&user_query_msg, &user_query_response_msg);
        m_db_handler->query_user(&friend_query_msg, &friend_query_response_msg);

        auto it = user_client.find(friend_query_response_msg.user_info);

        if (it != user_client.end())
        {
            friend_query_response_msg.user_info.status = 1;
            auto& friend_client = it->second;
            
            // if(send(friend_client, friend_outline_notice, sizeof(MSG_HEADER)+friend_outline_notice->msg_header.msg_length, 0) == -1)
            // {
            //     printf("向好友：%s 推送用户:%s 离线提醒消息失败\n", friend_list_msg->friends[i].user_account, user_info.user_account);
            // }

        }else{
            friend_query_response_msg.user_info.status = 0;
        }

        //  构造好友状态通知消息
        FRIEND_LIST_MSG* friend_status_notice = nullptr;
        if(m_db_handler->consturct_friend_notice_msg(&friend_query_response_msg.user_info, &friend_status_notice) != 0) 
        {
            free(friend_status_notice);
            return -1;
        }

        if(send(clientfd, friend_status_notice, sizeof(MSG_HEADER)+friend_status_notice->msg_header.msg_length, 0) == -1)
        {
            // printf("向好友：%s 推送用户:%s 离线提醒消息失败\n", friend_list_msg->friends[i].user_account, user_info.user_account);
        }

        free(friend_status_notice);
    }

    return 0;
}



int Business:: do_register(int sockfd, REGISTET_MSG *register_msg, RESPONSE_MSG *response_msg)
{

    if(m_db_handler->register_data_handle(register_msg, response_msg) != 0)
    {
        return -1;
    }
    
    strcpy(response_msg->response, "注册成功!\n");
    response_msg->success_flag = 1;

    printf("%s注册成功\n", register_msg->user_account);
    return 1;
}

int Business:: do_login(int sockfd, LOGIN_MSG *login_msg, USER_QUERY_RESPONSE_MSG *response_msg, USER_INFO *my_user_info)
{
    if(m_db_handler->login_check(login_msg, response_msg, my_user_info) != 0)
    {
        return -1;
    }

    user_client[*my_user_info] = sockfd;
    // user_client[sockfd] = string(login_msg->user_account);
    online_user_count = user_client.size();

    strcpy(response_msg->response, "登录成功!\n");
    response_msg->success_flag = 1;

    return 0;
}

int Business:: do_query(int sockfd, ACCOUNT_QUERY_MSG *query_msg, USER_QUERY_RESPONSE_MSG *response_msg)
{
    if(m_db_handler->query_user(query_msg, response_msg) != 0)
    {
        return -1;
    }

    auto it = user_client.find(response_msg->user_info);
    if (it != user_client.end())
    {
        printf("查询到的用户所处的在线客户端为：%d\n", it->second);
        response_msg->user_info.status = 1;
    }else{
        response_msg->user_info.status = 0;
    }
    
    printf("%s 查询到用户 %s 成功\n", query_msg->user_account, query_msg->query_account);
    return 0;
}

int Business:: do_addfriend(int sockfd, ADD_FRIEND_MSG *add_friend_msg, RESPONSE_MSG *response_msg)
{
    if(m_db_handler->addfriend_data_handle(add_friend_msg, response_msg) != 0)
    {
        return -1;
    }

    strcpy(response_msg->response, "发送好友请求成功！\n");
    response_msg->success_flag = 1;
    printf("%s 向用户 %s 发送好友请求成功\n", add_friend_msg->user_account, add_friend_msg->friend_account);

    return 0;
}

int Business:: do_acceptfriend(int sockfd, ADD_FRIEND_MSG *add_friend_msg, RESPONSE_MSG *response_msg, int choice)
{
    if(m_db_handler->updata_friend_data_handle(add_friend_msg, response_msg, choice) != 0)
    {
        return -1;
    }

    if (choice == ACCEPT_FRIEND_ASK)
    {
        strcpy(response_msg->response, "同意好友请求成功！\n");
    }else{
        strcpy(response_msg->response, "拒绝好友请求成功！\n");
    }

    response_msg->success_flag = 1;
    printf("%s 处理用户 %s 好友请求成功\n", add_friend_msg->friend_account, add_friend_msg->user_account);

    return 0;
}