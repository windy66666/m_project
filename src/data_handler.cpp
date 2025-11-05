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

int data_handler::query_user(ACCOUNT_QUERY_MSG *account_query_msg, USER_QUERY_RESPONSE_MSG *response_msg)
{
    char sql[1024];
    char *errmsg = NULL;
    char **resultp;
    int nrow;
    int ncolumn;

    sprintf(sql, "select *from user_info where (user_account='%s');", account_query_msg->query_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }

    if (nrow == 0)
    {
        printf("查询的用户:%s 不存在\n", account_query_msg->query_account);
        strcpy(response_msg->response, "查询的用户用户不存在\n");
        response_msg->success_flag = 0;
        return -1;
    }

    int index = ncolumn;
    strcpy(response_msg->user_info.user_name ,resultp[index]);
    strcpy(response_msg->user_info.user_account ,resultp[index+1]);

    // response_msg->user_info.status = *(int *)resultp[index+3];
    // printf("用户存储在数据库的密码：%s\n", resultp[index+1]);
    strcpy(response_msg->response, "成功查询到用户\n");
    response_msg->success_flag = 1;
    return 0;
}

int data_handler:: login_check(LOGIN_MSG *login_msg, USER_QUERY_RESPONSE_MSG *response_msg, USER_INFO *user_info)
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
        if (strcmp(login_msg->user_password, resultp[index+2]) != 0)
        {
            printf("用户:%s 密码错误\n", login_msg->user_account);
            strcpy(response_msg->response, "密码错误\n");
            response_msg->success_flag = 0;
            return -1;
        }
        response_msg->user_info.status = 1;
        strcpy(response_msg->user_info.user_account, login_msg->user_account);
        strcpy(response_msg->user_info.user_name, resultp[index]);

        user_info->status = 1;
        strcpy(user_info->user_account, login_msg->user_account);
        strcpy(user_info->user_name, resultp[index]);
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

int data_handler:: addfriend_data_handle(ADD_FRIEND_MSG *add_friend_msg, RESPONSE_MSG *response_msg)
{
    char sql[1024];
    char *errmsg = NULL;
    char **resultp;
    int nrow;
    int ncolumn;

    // ************************  获取到用户昵称，用于填充添加好友消息字段  ******************************
    sprintf(sql, "select *from user_info where (user_account='%s');", add_friend_msg->user_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }

    char user_name[NAME_SIZE];
    strcpy(user_name, resultp[ncolumn]);

    // ************************  查询添加的好友账号是否存在  ******************************
    resultp = NULL;
    nrow = 0;
    printf("friend_account:%s\n", add_friend_msg->friend_account);
    sprintf(sql, "select *from user_info where (user_account='%s');", add_friend_msg->friend_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }

    if (nrow == 0)
    {
        printf("添加的好友:%s 不存在\n", add_friend_msg->friend_account);
        strcpy(response_msg->response, "添加的用户不存在\n");
        response_msg->success_flag = 0;
        return -1;
    }

    char friend_name[NAME_SIZE];
    strcpy(friend_name, resultp[ncolumn]);


    // ************************  查询是否已经存在添加记录（针对正在等待回应status:0/已添加status：1）, 用户为添加者时 ******************************
    resultp = NULL;
    errmsg = NULL;
    sprintf(sql, "select *from friend_info where (friend_account='%s' and user_account='%s' and friend_status!=-1);", add_friend_msg->friend_account, add_friend_msg->user_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }

    if (nrow > 0)
    {
        int index = ncolumn;
        // printf("用户存储在数据库的密码：%s\n", resultp[index+1]);
        int friend_status = atoi(resultp[index+4]);
        printf("friend_status:%d\n", friend_status);
        if (friend_status == 1)
        {
            printf("已经是好友了\n");
            strcpy(response_msg->response, "你们已经是好友了！\n");
            response_msg->success_flag = 0;
        }else if(friend_status == 0){
            printf("已经添加过好友了\n");
            strcpy(response_msg->response, "你已经请求添加过该好友了, 请耐心等待ta回应\n");
            response_msg->success_flag = 0;
        }
        return -1;
    }

    resultp = NULL;
    errmsg = NULL;

    // ************************  查询是否已经存在添加记录（针对正在等待回应status:0/已添加status：1）, 用户为被添加者时 ******************************
    sprintf(sql, "select *from friend_info where (friend_account='%s' and user_account='%s' and friend_status!=-1);", add_friend_msg->user_account, add_friend_msg->friend_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }

    if (nrow > 0)
    {
        int index = ncolumn;
        // printf("用户存储在数据库的密码：%s\n", resultp[index+1]);
        int friend_status = atoi(resultp[index+4]);
        if (friend_status == 1)
        {
            printf("已经是好友了\n");
            strcpy(response_msg->response, "你们已经是好友了！\n");
            response_msg->success_flag = 0;
        }else if (friend_status == 0){
            printf("对方已经添加过你了\n");
            strcpy(response_msg->response, "该好友已经向你发送过好友请求了, 请回应ta\n");
            response_msg->success_flag = 0;
        }
        return -1;
    }

    // ************************ 通过所有检查后，说明不存在status!=-1的记录，则进行插入新添加记录 ******************************
    string now_time = TimeUtils::getCurrentTimeString("%Y-%m-%d %H:%M:%S");
    sprintf(sql, "insert into friend_info values('%s', '%s', '%s', '%s', '%d', '%s');", add_friend_msg->user_account, user_name ,add_friend_msg->friend_account, friend_name, 0, now_time.c_str());
    int res = sqlite3_exec(m_db, sql, NULL, NULL, &errmsg);
    if (res != SQLITE_OK)
    {
        if (errmsg != NULL)
        {
            printf("数据库用户:%s 添加好友插入错误:%s\n", add_friend_msg->user_account, errmsg);
            sqlite3_free(errmsg);
            errmsg = NULL;
        }
        return -1;
    }

    sqlite3_free_table(resultp);
    return 0;
}

int data_handler::get_addfriend_ask(char *user_account, FRIEND_ASK_NOTICE_MSG **friend_ask_notice_msg)
{
    char sql[1024];
    char *errmsg = NULL;
    char **resultp;
    int nrow;
    int ncolumn;

    // ************************  查询用户user_account的所有好友添加记录，不管是添加方还是被添加方，不管添加状态如何 ******************************
    sprintf(sql, "select *from friend_info where (user_account='%s' or friend_account='%s');", user_account, user_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }
    
    printf("nrow: %d\n", nrow);
    int header_size = sizeof(MSG_HEADER);
    int data_size = sizeof(FRIEND_ADD_ASK) * nrow;
    int total_size = header_size + data_size;
    *friend_ask_notice_msg = (FRIEND_ASK_NOTICE_MSG*)malloc(total_size);
    memset(*friend_ask_notice_msg, 0, total_size);
    // friend_ask_notice_msg->ask_num = nrow;
    (*friend_ask_notice_msg)->msg_header.msg_length = data_size;
    (*friend_ask_notice_msg)->msg_header.msg_type = ADD_FRIEND_LIST_RESPONSE;
    (*friend_ask_notice_msg)->msg_header.timestamp = time(NULL);
    (*friend_ask_notice_msg)->msg_header.total_count = nrow;

    int index = ncolumn;
    for (int i = 0; i < nrow; i++)
    {
        strcpy((*friend_ask_notice_msg)->asks[i].user_account, resultp[index++]);
        strcpy((*friend_ask_notice_msg)->asks[i].user_name, resultp[index++]);
        strcpy((*friend_ask_notice_msg)->asks[i].friend_acccount, resultp[index++]);
        strcpy((*friend_ask_notice_msg)->asks[i].friend_name, resultp[index++]);
        (*friend_ask_notice_msg)->asks[i].friend_status = atoi(resultp[index++]);
        strcpy((*friend_ask_notice_msg)->asks[i].add_time, resultp[index++]);
    }


    for (int i = 0; i < (*friend_ask_notice_msg)->msg_header.total_count; i++)
    {
        printf("添加账号：%s\t添加账号昵称: %s\t被加账号: %s\t被加昵称: %s\t添加状态:%d\t添加时间: %s\t\n", (*friend_ask_notice_msg)->asks[i].user_account, 
        (*friend_ask_notice_msg)->asks[i].user_name,
        (*friend_ask_notice_msg)->asks[i].friend_acccount,(*friend_ask_notice_msg)->asks[i].friend_name,
        (*friend_ask_notice_msg)->asks[i].friend_status, (*friend_ask_notice_msg)->asks[i].add_time);
    }

    sqlite3_free_table(resultp);
    return 0;
}

int data_handler:: updata_friend_data_handle(ADD_FRIEND_MSG *add_friend_msg, RESPONSE_MSG *response_msg, int choice)
{
    char sql[1024];
    char *errmsg = NULL;

    // ************************  查询用户的好友添加记录(作为被添加方)，添加状态为正在验证：status：0 ******************************
    if (choice == ACCEPT_FRIEND_ASK)
    {
        sprintf(sql, "update friend_info set friend_status=1 where (friend_account='%s' and user_account='%s' and friend_status=0);", add_friend_msg->friend_account, add_friend_msg->user_account);
    }else{
        sprintf(sql, "update friend_info set friend_status=-1 where (friend_account='%s' and user_account='%s' and friend_status=0);", add_friend_msg->friend_account, add_friend_msg->user_account);
    }

    int res = sqlite3_exec(m_db, sql, NULL, NULL, &errmsg);
    if (res != SQLITE_OK)
    {
        if (errmsg != NULL)
        {
            printf("数据库用户:%s 更新好友申请信息错误:%s\n", add_friend_msg->friend_account, errmsg);
            sqlite3_free(errmsg);
            errmsg = NULL;
        }
        strcpy(response_msg->response, errmsg);
        response_msg->success_flag = 0;
        return -1;
    }
    return 0;
}

int data_handler::get_friendlist(char *user_account, FRIEND_LIST_MSG **friend_list_msg)
{
    char sql[1024];
    char *errmsg = NULL;
    char **resultp;
    int nrow;
    int ncolumn;

    // ************************  查询用户的好友列表，即不论是添加方还是被添加方，状态为已添加 ******************************
    sprintf(sql, "select *from friend_info where (user_account='%s' or friend_account='%s') and friend_status=1;", user_account, user_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }
    
    printf("nrow: %d\n", nrow);
    int header_size = sizeof(MSG_HEADER);
    int data_size = sizeof(USER_INFO) * nrow;
    int total_size = header_size + data_size;
    *friend_list_msg = (FRIEND_LIST_MSG*)malloc(total_size);
    memset(*friend_list_msg, 0, total_size);
    // friend_ask_notice_msg->ask_num = nrow;
    (*friend_list_msg)->msg_header.msg_length = data_size;
    (*friend_list_msg)->msg_header.msg_type = FRIEND_LIST_RESPONSE;
    (*friend_list_msg)->msg_header.timestamp = time(NULL);
    (*friend_list_msg)->msg_header.total_count = nrow;

    int index = ncolumn;
    for (int i = 0; i < nrow; i++)
    {
        if(strcmp(resultp[index], user_account) == 0){
            strcpy((*friend_list_msg)->friends[i].user_account, resultp[index+2]);
            strcpy((*friend_list_msg)->friends[i].user_name, resultp[index+3]);
            index += ncolumn;
        }else{
            strcpy((*friend_list_msg)->friends[i].user_account, resultp[index]);
            strcpy((*friend_list_msg)->friends[i].user_name, resultp[index+1]);
            index += ncolumn;
        }
    }

    for (int i = 0; i < (*friend_list_msg)->msg_header.total_count; i++)
    {
        printf("好友账号：%s\t好友昵称: %s\t \n", (*friend_list_msg)->friends[i].user_account, (*friend_list_msg)->friends[i].user_name);
    }

    sqlite3_free_table(resultp);
    return 0;
}

int data_handler::consturct_friend_notice_msg(USER_INFO *user_info, FRIEND_LIST_MSG **friend_list_msg)
{
    int header_size = sizeof(MSG_HEADER);
    int data_size = sizeof(USER_INFO);
    int total_size = header_size + data_size;
    *friend_list_msg = (FRIEND_LIST_MSG*)malloc(total_size);
    memset(*friend_list_msg, 0, total_size);
    // friend_ask_notice_msg->ask_num = nrow;
    (*friend_list_msg)->msg_header.msg_length = data_size;
    (*friend_list_msg)->msg_header.msg_type = FRIEND_STATUS_NOTICE;
    (*friend_list_msg)->msg_header.timestamp = time(NULL);
    (*friend_list_msg)->msg_header.total_count = 1;

    strcpy((*friend_list_msg)->friends->user_account, user_info->user_account);
    strcpy((*friend_list_msg)->friends->user_name, user_info->user_name);
    (*friend_list_msg)->friends->status = user_info->status;

    return 0;
}


int data_handler::consutrct_friend_ask_notice_msg(ADD_FRIEND_MSG *add_friend_msg, FRIEND_ASK_NOTICE_MSG **friend_ask_notice_msg)
{
    char sql[1024];
    char *errmsg = NULL;
    char **resultp;
    int nrow;
    int ncolumn;

    // ************************  查询时间最近的这条好友添加记录   用于好友新申请、好友通过/拒绝 后发送好友申请列表的更新  ******************************
    sprintf(sql, "select *from friend_info where (user_account='%s' and friend_account='%s') order by add_time desc limit 1;", add_friend_msg->user_account, add_friend_msg->friend_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }
    
    int header_size = sizeof(MSG_HEADER);
    int data_size = sizeof(FRIEND_ADD_ASK) * nrow;
    int total_size = header_size + data_size;
    *friend_ask_notice_msg = (FRIEND_ASK_NOTICE_MSG*)malloc(total_size);
    memset(*friend_ask_notice_msg, 0, total_size);
    // friend_ask_notice_msg->ask_num = nrow;
    (*friend_ask_notice_msg)->msg_header.msg_length = data_size;
    (*friend_ask_notice_msg)->msg_header.msg_type = ADD_FRIEND_NOTICE;
    (*friend_ask_notice_msg)->msg_header.timestamp = time(NULL);
    (*friend_ask_notice_msg)->msg_header.total_count = nrow;

    int index = ncolumn;

    strcpy((*friend_ask_notice_msg)->asks[0].user_account, resultp[index++]);
    strcpy((*friend_ask_notice_msg)->asks[0].user_name, resultp[index++]);
    strcpy((*friend_ask_notice_msg)->asks[0].friend_acccount, resultp[index++]);
    strcpy((*friend_ask_notice_msg)->asks[0].friend_name, resultp[index++]);
    (*friend_ask_notice_msg)->asks[0].friend_status = atoi(resultp[index++]);
    strcpy((*friend_ask_notice_msg)->asks[0].add_time, resultp[index++]);


    printf("添加账号：%s\t添加账号昵称: %s\t被加账号: %s\t被加昵称: %s\t添加状态:%d\t添加时间: %s\t\n", (*friend_ask_notice_msg)->asks[0].user_account, 
    (*friend_ask_notice_msg)->asks[0].user_name,
    (*friend_ask_notice_msg)->asks[0].friend_acccount,(*friend_ask_notice_msg)->asks[0].friend_name,
    (*friend_ask_notice_msg)->asks[0].friend_status, (*friend_ask_notice_msg)->asks[0].add_time);


    sqlite3_free_table(resultp);
    return 0;
}