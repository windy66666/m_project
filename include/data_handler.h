#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include <sqlite3.h>
#include <stdio.h>
#include <errno.h>
#include "protocol.h"
#include <string.h>

using namespace std;

class data_handler {
public:
    data_handler();
    ~data_handler();
    int login_check(LOGIN_MSG *login_msg, RESPONSE_MSG *response_msg);
    
public:

private:
    sqlite3 *m_db;
};

#endif // DATA_HANDLER_H