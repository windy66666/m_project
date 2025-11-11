#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include <sqlite3.h>
#include <stdio.h>
#include <errno.h>
#include "protocol.h"
#include <string.h>
#include <fstream>
#include <vector>
#include <sys/stat.h>  // 用于目录创建
#include <ctime>       // 用于时间戳
#include <cstring>     // 用于字符串操作


using namespace std;

class data_handler {
public:
    data_handler(const char *db_path);
    ~data_handler();
    int query_user(ACCOUNT_QUERY_MSG *account_query_msg, USER_QUERY_RESPONSE_MSG *response_msg);
    int login_check(LOGIN_MSG *login_msg, USER_QUERY_RESPONSE_MSG *response_msg, USER_INFO *user_info);
    int register_data_handle(REGISTET_MSG *register_msg, RESPONSE_MSG *response_msg);
    int addfriend_data_handle(ADD_FRIEND_MSG *add_friend_msg, RESPONSE_MSG *response_msg);
    int get_addfriend_ask(char *user_account, FRIEND_ASK_NOTICE_MSG **friend_ask_notice_msg);
    int updata_friend_data_handle(ADD_FRIEND_MSG *add_friend_msg, RESPONSE_MSG *response_msg, int choice);
    int get_friendlist(char *user_account, FRIEND_LIST_MSG **friend_list_msg);
    int consturct_friend_notice_msg(USER_INFO *user_info, FRIEND_LIST_MSG **friend_list_msg);
    int consutrct_friend_ask_notice_msg(ADD_FRIEND_MSG *add_friend_msg, FRIEND_ASK_NOTICE_MSG **friend_ask_notice_msg);
    int chatmsg_data_handle(CHAT_MSG *chat_msg, RESPONSE_MSG * response_msg);
    int get_history_msg_handle(HISTORY_MSG_GET *HistoryMsgGet_msg , vector<CHAT_MSG*>& chat_msgs);
    int update_chat_msg_status(vector<long long> &chat_msgs);
    string save_avatar_to_disk(REGISTET_MSG *register_msg);
    string save_avatar_to_disk(CREATE_GROUP_MSG *msg);
    int load_avatar_data(const char* avatar_path, USER_INFO* user_info);
    int load_avatar_data(const char* avatar_path, FRIEND_ADD_ASK &friend_add_ask, int type);
    int load_avatar_data(const char* avatar_path, GROUP_INFO* group_info);
    int load_avatar_data(const char* avatar_path, GROUP_ADD_ASK &group_add_ask, int type);
    int create_group_handle(CREATE_GROUP_MSG *msg, RESPONSE_MSG *response_msg);
    int query_group(ACCOUNT_QUERY_MSG *account_query_msg, GROUP_QUERY_RESPONSE_MSG *response_msg);
    int addgroup_data_handle(ADD_GROUP_MSG *add_group_msg, RESPONSE_MSG *response_msg);
    int get_addgroup_ask(char *user_account, GROUP_ASK_NOTICE_MSG **group_ask_notice_msg);
    int consutrct_group_ask_notice_msg(ADD_GROUP_MSG *add_group_msg, GROUP_ASK_NOTICE_MSG **group_ask_notice_msg);
    int updata_group_data_handle(ADD_GROUP_MSG *add_group_msg, RESPONSE_MSG *response_msg, int choice);
    int get_grouplist(char *user_account, GROUP_LIST_MSG **group_list_msg);
    int consturct_group_notice_msg(GROUP_INFO *group_info, GROUP_LIST_MSG **group_list_msg);
    int get_users_in_group(vector<USER_INFO> &users_in_group, char* group_account);
    int group_chatmsg_data_handle(CHAT_MSG *chat_msg, RESPONSE_MSG * response_msg, vector<sqlite3_int64>& message_ids);
    int get_users_account_in_group(vector<char *> &users_account_in_group, char* group_account);
    int get_group_history_msg_handle(HISTORY_MSG_GET *HistoryMsgGet_msg , vector<CHAT_MSG*> &chat_msgs);
    int update_group_chat_msg_status(vector<long long> &chat_msgs);

public:

private:
    sqlite3 *m_db;
    const char *m_db_path;
};

#endif // DATA_HANDLER_H