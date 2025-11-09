#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include <sqlite3.h>
#include <stdio.h>
#include <errno.h>
#include "protocol.h"
#include <string.h>
#include <vector>

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

public:

private:
    sqlite3 *m_db;
    const char *m_db_path;
};

#endif // DATA_HANDLER_H