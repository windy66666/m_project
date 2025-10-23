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

    int do_register(int sockfd, REGISTET_MSG *register_msg, RESPONSE_MSG *response_msg);
    int do_login(int sockfd, LOGIN_MSG *login_msg, RESPONSE_MSG *response_msg);

    int receive_message_header(int sockfd, MSG_HEADER *header, Business *business);
    int handle_login_message(int clientfd, MSG_HEADER *msg_header);
    int handle_register_message(int clientfd, MSG_HEADER *msg_header);

    template<typename T>
    int receive_remain_message(int clientfd, MSG_HEADER *msg_header, T *total_msg);

public:
    GThreadPool *m_pool;
    int online_user_count = 0;
    map <int, string> user_client;

private:
    data_handler *m_db_handler;
};

#endif // BUSINESS_H