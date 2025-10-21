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
#include <sqlite3.h>
#include <sys/socket.h>
#include <errno.h>
#include <iostream>
#include "protocol.h"

using namespace std;

class Business {
public:
    Business();
    ~Business();
    static void thread_handle(gpointer data, gpointer user_data);
    void push_task(int *clientfd_ptr);

    static int callback(void *arg, int col, char** value, char** key);
    static int callback_query(void *arg, int col, char** value, char** key);
    int search_user(sqlite3 *db, MSG_INFO *msg_info);
    int do_register(int sockfd, MSG_INFO *msg_info);
    int do_login(int sockfd, MSG_INFO *msg_info);

public:
    GThreadPool *m_pool;
    int online_user_count = 0;
    map <int, string> user_client;

private:
    sqlite3 *m_db;
};

#endif // BUSINESS_H