#include "data_handler.h"

// #define db_name "my.db" 

data_handler::data_handler(const char *db_path)
{
    m_db_path = db_path;
    if(sqlite3_open(m_db_path, &m_db) != SQLITE_OK)
    {
        perror("sqlite3_open");
    }
}

data_handler::~data_handler()
{
    sqlite3_close(m_db);
}

int data_handler::login_check(LOGIN_MSG *login_msg, RESPONSE_MSG *response_msg)
{
    char sql[1024];
    char *errmsg = NULL;
    char **resultp;
    int nrow;
    int ncolumn;

    sprintf(sql, "select *from user_info where (user_account='%s');", login_msg->user_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }

    if (nrow == 0)
    {
        printf("用户:%s 不存在\n", login_msg->user_account);
        strcpy(response_msg->response, "用户不存在\n");
        response_msg->success_flag = 0;
        return -1;
    }else{
        int index = ncolumn;
        // printf("用户存储在数据库的密码：%s\n", resultp[index+1]);
        if (strcmp(login_msg->user_password, resultp[index+2]) != 0)
        {
            printf("用户:%s 密码错误\n", login_msg->user_account);
            strcpy(response_msg->response, "密码错误\n");
            response_msg->success_flag = 0;
            return -1;
        }
    }

    return 0;
}

int data_handler:: register_data_handle(REGISTET_MSG *register_msg, RESPONSE_MSG *response_msg)
{
    char sql[1024];
    char *errmsg = NULL;
    char **resultp;
    int nrow;
    int ncolumn;

    sprintf(sql, "select *from user_info where (user_account='%s');", register_msg->user_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }

    if (nrow > 0)
    {
        printf("用户:%s 已经存在\n", register_msg->user_account);
        strcpy(response_msg->response, "用户已经存在\n");
        response_msg->success_flag = 0;
        return -1;
    }

    string now_time = TimeUtils::getCurrentTimeString("%Y-%m-%d %H:%M");
    sprintf(sql, "insert into user_info values('%s', '%s', '%s', '%s');", register_msg->user_name, register_msg->user_account, register_msg->user_password, now_time.c_str());
    int res = sqlite3_exec(m_db, sql, NULL, NULL, &errmsg);
    if (res != SQLITE_OK)
    {
        if (errmsg != NULL)
        {
            printf("数据库用户注册插入错误:%s\n", errmsg);
            sqlite3_free(errmsg);
            errmsg = NULL;
        }
        return -1;
    }

    return 0;
}
