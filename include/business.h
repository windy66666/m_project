#ifndef BUSINESS_H
#define BUSINESS_H

#include <string.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include <glib.h>
// #include <sqlite3.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include "protocol.h"
#include "data_handler.h"
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

using namespace std;

class Business {
public:
    Business(data_handler * DataHandler);
    ~Business();
    static void thread_handle(gpointer data, gpointer user_data);
    void push_task(int *clientfd);

    static int callback(void *arg, int col, char** value, char** key);
    static int callback_query(void *arg, int col, char** value, char** key);
    int search_user(sqlite3 *db, LOGIN_MSG *login_msg);

    // 接收消息函数
    int receive_message_header(int sockfd, MSG_HEADER *header, Business *business);
    template<typename T>
    int receive_remain_message(int clientfd, MSG_HEADER *msg_header, T *total_msg);

    // 辅助处理函数
    void parseMessageIds(const std::vector<char>& buffer, std::vector<long long>& chat_msgs);
    int send_large_data(int sockfd, const void* data, size_t total_size);

    // 处理用户各种消息函数
    int handle_AccountQuery_message(int clientfd, MSG_HEADER *msg_header);
    int handle_login_message(int clientfd, MSG_HEADER *msg_header);
    int handle_register_message(int clientfd, MSG_HEADER *msg_header);
    int handle_addfriend_message(int clientfd, MSG_HEADER *msg_header);
    int handle_acceptfriend_message(int clientfd, MSG_HEADER *msg_header, int choice);
    int handle_chat_message(int clientfd, MSG_HEADER * msg_header);
    int handle_HistoryMsgGet_message(int clientfd, MSG_HEADER * msg_header);
    int handle_updateMsg_message(int clientfd, MSG_HEADER * msg_header);
    int handle_create_group_message(int clientfd, MSG_HEADER *msg_header);
    int handle_GroupQuery_message(int clientfd, MSG_HEADER *msg_header);
    int handle_addgroup_message(int clientfd, MSG_HEADER *msg_header);
    int handle_acceptgroup_message(int clientfd, MSG_HEADER *msg_header, int choice);
    int handle_group_chat_message(int clientfd, MSG_HEADER * msg_header);
    int handle_query_users_in_group_message(int clientfd, MSG_HEADER * msg_header);

    // 执行用户操作指令函数
    int do_query(int sockfd, ACCOUNT_QUERY_MSG *query_msg, USER_QUERY_RESPONSE_MSG *response_msg);
    int do_register(int sockfd, REGISTET_MSG *register_msg, RESPONSE_MSG *response_msg);
    int do_login(int sockfd, LOGIN_MSG *login_msg, USER_QUERY_RESPONSE_MSG *response_msg, USER_INFO *my_user_info);
    int do_addfriend(int sockfd, ADD_FRIEND_MSG *add_friend_msg, RESPONSE_MSG *response_msg);
    int do_acceptfriend(int sockfd, ADD_FRIEND_MSG *add_friend_msg, RESPONSE_MSG *response_msg, int choice);
    int do_send_chat_msg(int sockfd, CHAT_MSG *chat_msg, RESPONSE_MSG *response_msg);
    int do_acceptgroup(int sockfd, ADD_GROUP_MSG *add_group_msg, RESPONSE_MSG *response_msg, int choice);
    int do_send_group_chat_msg(int sockfd, CHAT_MSG *chat_msg, RESPONSE_MSG *response_msg, vector<sqlite3_int64>& message_ids);
    int handle_update_groupMsg_message(int clientfd, MSG_HEADER *msg_header);

    // 添加和移除客户端锁的方法
    void add_client_lock(int clientfd);
    void remove_client_lock(int clientfd);
    // 获取客户端的读锁（用于读取消息）
    std::shared_lock<std::shared_mutex> get_client_read_lock(int clientfd);
    // 获取客户端的写锁（用于处理消息）
    std::unique_lock<std::shared_mutex> get_client_write_lock(int clientfd);

public:
    GThreadPool *m_pool;
    int online_user_count = 0;
    map <USER_INFO, int> user_client;

private:
    data_handler *m_db_handler;
    // 为每个客户端fd维护一个读写锁
    std::unordered_map<int, std::shared_mutex> client_mutex_map;
    std::mutex map_mutex; // 保护client_mutex_map的互斥锁
};

#endif // BUSINESS_H