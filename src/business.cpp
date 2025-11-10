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
    case REGISTER_REQUEST:         // 处理用户注册请求
        // printf("即将注册用户,账号:%s  密码:%s\n", msg_info.account_num, msg_info.password);
        self->handle_register_message(clientfd, &msg_header);
        break;
    case LOGIN_REQUEST:           // 处理用户登录请求
        // printf("-------------------执行登录-----------\n");
        self->handle_login_message(clientfd, &msg_header);
        break;
    case ACCOUNT_QUERY_REQUEST:       // 处理用户查询请求
        printf("查询用户\n");
        self->handle_AccountQuery_message(clientfd, &msg_header);
        break;
    case ADD_FRIEND_REQUEST:        // 处理用户添加好友请求
        self->handle_addfriend_message(clientfd, &msg_header);
        break;
    case ACCEPT_FRIEND_ASK:        // 处理用户通过好友申请
        self->handle_acceptfriend_message(clientfd, &msg_header, ACCEPT_FRIEND_ASK);
        break;
    case REJECT_FRIEND_ASK:       // 处理用户拒绝好友申请
        self->handle_acceptfriend_message(clientfd, &msg_header, REJECT_FRIEND_ASK);
        break;
    case SEND_CHAT_MSG:           // 处理聊天消息 
        self->handle_chat_message(clientfd, &msg_header);
        break;
    case HISTORY_MSG_REQUEST:      // 拉取历史消息
        self->handle_HistoryMsgGet_message(clientfd, &msg_header);
        break;
    case UPDATE_CHAT_MSG_REQUEST:    // 更新消息阅读状态
        self->handle_updateMsg_message(clientfd, &msg_header);
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

// 添加一个辅助函数来安全发送大数据
int Business::send_large_data(int sockfd, const void* data, size_t total_size) {
    const char* buffer = (const char*)data;
    size_t sent_total = 0;
    
    while (sent_total < total_size) {
        ssize_t sent = send(sockfd, buffer + sent_total, 
                           total_size - sent_total, 0);
        if (sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 缓冲区满，稍等再试
                usleep(1000); // 1ms
                continue;
            }
            return -1; // 发送失败
        }
        sent_total += sent;
    }
    return 0;
}

int Business:: handle_login_message(int clientfd, MSG_HEADER *msg_header)
{
    USER_INFO my_user_info;

    // *********************** 登录检查 ***************************
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

    // *********************** 拉取用户 好友申请列表 ***************************
    //  拉取用户好友申请列表
    FRIEND_ASK_NOTICE_MSG *friend_ask_notice_msgs = nullptr;  
    if(m_db_handler->get_addfriend_ask(login_msg.user_account, &friend_ask_notice_msgs) != 0)  // 注意！！！ 这里得是二级指针，因为需要修改其地址值
    {
        free(friend_ask_notice_msgs);
        return -1;
    }

    if(send_large_data(clientfd, friend_ask_notice_msgs, sizeof(MSG_HEADER)+friend_ask_notice_msgs->msg_header.msg_length) == -1)
    {
        printf("向用户:%s 推送好友请求通知消息失败\n", login_msg.user_account);
    }

    free(friend_ask_notice_msgs);

    // *********************** 拉取用户 好友列表    向在线好友发送上线通知  ***************************
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

    if(send_large_data(clientfd, friend_list_msg, sizeof(MSG_HEADER)+friend_list_msg->msg_header.msg_length) == -1)
    {
        printf("向用户:%s 推送好友请求通知消息失败\n", login_msg.user_account);
    }

    free(friend_list_msg);
    free(friend_online_notice);

    // *********************** 拉取完毕，向用户发送登录响应  ***************************
    //  发送用户登录响应
    if(send(clientfd, &response_msg, sizeof(response_msg), 0) == -1)
    {
        printf("向用户:%s 发送登录响应失败\n", login_msg.user_account);
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
    // ***********************  处理用户添加好友请求  【此时用户为添加方】***************************
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

    if(send(clientfd, &response_msg, sizeof(response_msg), 0) == -1)     //  向操作用户发送添加好友是否成功的响应
    {
        printf("向用户:%s 发送添加好友响应失败", add_friend_msg.user_account);
    }

    printf("success_flag:%d\n", response_msg.success_flag);
    if (response_msg.success_flag == 0)      // 如果此次添加失败，则直接返回
    {
        return -1;
    }

    // ***********************  拉取这条申请消息，向用户，好友分别发送这条申请消息，进行客户端UI更新 ***************************
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
    // friend_ask_notice_msg->asks[0].friend_status = 0;
    printf("发送过去的好友状态为：%d", friend_ask_notice_msg->asks[0].friend_status);

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
    // ***********************  处理用户同意/拒绝好友申请 消息  【此时用户为被添加方】***************************
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

    if(send(clientfd, &response_msg, sizeof(response_msg), 0) == -1)   //  向操作用户发送处理好友申请是否成功的响应
    {
        printf("向用户:%s 发送处理好友申请 响应失败", add_friend_msg.user_account);
    }


    // **********************   向双方发送好友申请更新消息，更新客户端UI  ************************
    //  拉取用户好友单条申请消息
    FRIEND_ASK_NOTICE_MSG *friend_ask_notice_msg = nullptr;  
    if(m_db_handler->consutrct_friend_ask_notice_msg(&add_friend_msg, &friend_ask_notice_msg) != 0)  
    {
        free(friend_ask_notice_msg);
        return -1;
    }

    // 向  处理好友请求的用户（被添加方） 发送  好友申请单条消息
    if(send(clientfd, friend_ask_notice_msg, sizeof(MSG_HEADER)+friend_ask_notice_msg->msg_header.msg_length, 0) == -1)
    {
        printf("向用户:%s 推送好友处理消息失败\n", add_friend_msg.friend_account);
    }
    
    USER_INFO friend_info;
    strcpy(friend_info.user_account, friend_ask_notice_msg->asks[0].user_account);
    strcpy(friend_info.user_name, friend_ask_notice_msg->asks[0].user_name);
    // friend_ask_notice_msg->asks[0].friend_status = 0;
    printf("发送过去的好友状态为：%d", friend_ask_notice_msg->asks[0].friend_status);

    // 向  被用户处理申请请求的好友（添加方）（如果在线的话）发送  好友申请处理更新单条消息
    auto it = user_client.find(friend_info);

    if (it != user_client.end())
    {
        friend_info.status = 1;
        auto& friend_client = it->second;
        if(send(friend_client, friend_ask_notice_msg, sizeof(MSG_HEADER)+friend_ask_notice_msg->msg_header.msg_length, 0) == -1)
        {
            printf("向用户:%s 推送好友添加通知消息失败\n", add_friend_msg.user_account);
        }

    }else{
        friend_info.status = 0;
    }

    free(friend_ask_notice_msg);

    // *****************************   如果添加成功，则需向双方分别发送好友列表更新消息，更新客户端好友列表 *************************************

    if(choice == ACCEPT_FRIEND_ASK){
        ACCOUNT_QUERY_MSG user_query_msg, friend_query_msg;
        strcpy(user_query_msg.query_account, add_friend_msg.friend_account);
        stpcpy(user_query_msg.user_account, add_friend_msg.friend_account);

        strcpy(friend_query_msg.query_account, add_friend_msg.user_account);   // add_friend_msg里的user_account是添加这个用户的好友
        stpcpy(friend_query_msg.user_account, add_friend_msg.friend_account);  // add_friend_msg里的friend_account才是这里的用户

        USER_QUERY_RESPONSE_MSG user_query_response_msg, friend_query_response_msg;
        m_db_handler->query_user(&user_query_msg, &user_query_response_msg);        // 查询这个用户的信息，通知其好友使用
        m_db_handler->query_user(&friend_query_msg, &friend_query_response_msg);    // 查询用户的这个好友信息，通知用户使用

  
        //  构造用户状态通知消息
        FRIEND_LIST_MSG* user_status_notice = nullptr;
        if(m_db_handler->consturct_friend_notice_msg(&user_query_response_msg.user_info, &user_status_notice) != 0) 
        {
            free(user_status_notice);
            return -1;
        }

        user_status_notice->friends[0].status = 1;

        auto it = user_client.find(friend_query_response_msg.user_info);

        if (it != user_client.end())
        {
            friend_query_response_msg.user_info.status = 1;
            auto& friend_client = it->second;
            
            // 如果好友在线，则提醒它好友申请通过/拒绝通知
            if(send(friend_client, user_status_notice, sizeof(MSG_HEADER)+user_status_notice->msg_header.msg_length, 0) == -1)
            {
                printf("向用户：%s 推送好友:%s 状态消息失败\n", add_friend_msg.user_account, add_friend_msg.friend_account);
            }

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

        // 向用户发送好友申请通过/拒绝通知
        if(send(clientfd, friend_status_notice, sizeof(MSG_HEADER)+friend_status_notice->msg_header.msg_length, 0) == -1)
        {
            printf("向用户：%s 推送好友:%s 状态消息失败\n", add_friend_msg.friend_account, add_friend_msg.user_account);
        }

        free(friend_status_notice);
    }

    return 0;
}

int Business:: handle_chat_message(int clientfd, MSG_HEADER * msg_header)
{
    int total_size = sizeof(MSG_HEADER) + msg_header->msg_length;
    CHAT_MSG *chat_msg = (CHAT_MSG*)malloc(total_size);

    int remain_data = msg_header->msg_length;
    
    memcpy(&chat_msg->msg_header, msg_header, sizeof(MSG_HEADER));

    char *msg_data = (char*)(chat_msg) + sizeof(MSG_HEADER); 
    int recv_size = recv(clientfd, msg_data, remain_data, MSG_WAITALL);
    if (recv_size != remain_data)
    {
        printf("消息不完整\n");
        return -1;
    }

    RESPONSE_MSG response_msg;
    memset(&response_msg, 0, sizeof(response_msg));
    response_msg.msg_header.msg_type = SEND_CHAT_RESPONSE;
    response_msg.msg_header.msg_length = sizeof(RESPONSE_MSG) - sizeof(MSG_HEADER);
    response_msg.msg_header.timestamp = time(NULL);

    printf("用户:%s 即将给用户:%s  发送消息\n", chat_msg->sender_account, chat_msg->receiver_account);
    do_send_chat_msg(clientfd, chat_msg, &response_msg);

    if(send(clientfd, &response_msg, sizeof(response_msg), 0) == -1)
    {
        printf("向用户:%s 发送 消息发送响应失败", chat_msg->sender_account);
    }

    // 如果接收者用户在线，则立马转发消息给ta
    USER_INFO friend_info;
    strcpy(friend_info.user_account, chat_msg->receiver_account);

    chat_msg->msg_header.msg_type = CHAT_MSG_NOTICE;
    auto it = user_client.find(friend_info);

    if (it != user_client.end())
    {
        auto& friend_client = it->second;
        if(send(friend_client, chat_msg, sizeof(MSG_HEADER)+chat_msg->msg_header.msg_length, 0) == -1)
        {
            printf("向用户:%s 转发聊天消息失败\n", chat_msg->receiver_account);
        }
    }

    return 0;
}

int Business::handle_HistoryMsgGet_message(int clientfd, MSG_HEADER * msg_header)
{
    // 接受剩余数据
    HISTORY_MSG_GET HisrotyMsg_get_msg;

    if(receive_remain_message(clientfd, msg_header, &HisrotyMsg_get_msg) != 0)
    {
        return -1;
    }

    printf("即将获取用户:%s 历史消息\n", HisrotyMsg_get_msg.user_account);

    vector<CHAT_MSG*> chat_msgs;
    int res = m_db_handler->get_history_msg_handle(&HisrotyMsg_get_msg, chat_msgs);
    if (res <= 0) {
        printf("用户没有历史消息或查询失败\n");
        return 0;
    }

    int sent_count = 0;
    int failed_count = 0;

    for (CHAT_MSG* chat_msg : chat_msgs) {
        size_t msg_size = sizeof(MSG_HEADER) + chat_msg->msg_header.msg_length;
        int bytes_sent = send(clientfd, (const char*)chat_msg, msg_size, 0);
        
        if (bytes_sent <= 0) {
            printf("发送消息失败，消息ID: %lld\n", chat_msg->chat_msg_id);
            failed_count++;
        } else {
            sent_count++;
        }
        
        // 释放当前消息内存
        free(chat_msg);
        
        // 添加小延迟，避免网络拥堵（可选）
        // usleep(1000); // 1ms 延迟
    }
    
    // 清空vector
    chat_msgs.clear();
    printf("历史消息发送完成: 成功 %d/%d, 失败 %d\n", 
           sent_count, res, failed_count);

    return (failed_count == 0) ? 0 : -1;    
}

int Business::handle_updateMsg_message(int clientfd, MSG_HEADER *msg_header)
{
    int total_size = sizeof(MSG_HEADER) + msg_header->msg_length;
    
    // 使用 new 创建对象，会调用构造函数
    UPDATE_CHAT_MSG *msg = new UPDATE_CHAT_MSG;
    
    // 复制消息头
    msg->msg_header = *msg_header;
    
    // 接收消息数据
    std::vector<char> buffer(msg_header->msg_length);
    int recv_size = recv(clientfd, buffer.data(), msg_header->msg_length, MSG_WAITALL);
    if (recv_size != msg_header->msg_length)
    {
        printf("消息不完整\n");
        delete msg; // 记得释放内存
        return -1;
    }
    
    // 解析消息ID列表
    parseMessageIds(buffer, msg->chat_msgs);
    
    printf("更新消息阅读状态，共 %zu 条消息\n", msg->chat_msgs.size());
    m_db_handler->update_chat_msg_status(msg->chat_msgs);
    
    delete msg; // 释放内存
    return 0;
}

// 解析消息ID的辅助函数
void Business::parseMessageIds(const std::vector<char>& buffer, std::vector<long long>& chat_msgs)
{
    const char* data = buffer.data();
    size_t size = buffer.size();
    
    // 假设协议格式：消息数量(4字节) + 多个消息ID(每个8字节)
    if (size < sizeof(int)) {
        return;
    }
    
    int count = *(int*)data;
    data += sizeof(int);
    size -= sizeof(int);
    
    if (size < count * sizeof(long long)) {
        return;
    }
    
    chat_msgs.resize(count);
    for (int i = 0; i < count; i++) {
        chat_msgs[i] = *(long long*)data;
        data += sizeof(long long);
    }
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

int Business:: do_send_chat_msg(int sockfd, CHAT_MSG *chat_msg, RESPONSE_MSG *response_msg)
{
    if(m_db_handler->chatmsg_data_handle(chat_msg, response_msg) != 0)
    {
        response_msg->success_flag = 0;
        strcpy(response_msg->response, "发送消息失败\n");
        return -1;
    }
    
    strcpy(response_msg->response, "发送成功!\n");
    response_msg->success_flag = 1;

    printf("%s发送成功\n", chat_msg->sender_account);
    return 1;
}