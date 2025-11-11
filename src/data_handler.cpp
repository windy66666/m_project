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

    char* db_avatar_path = resultp[index+4];
    // 处理头像数据
    if (db_avatar_path != NULL && strlen(db_avatar_path) > 0) {
        load_avatar_data(db_avatar_path, &response_msg->user_info);
    } else {
        response_msg->user_info.avatar_size = 0;
        memset(response_msg->user_info.avatar_data, 0, MAX_AVATAR_SIZE);
    }

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

        char* db_avatar_path = resultp[index+4];
        // 处理头像数据
        if (db_avatar_path != NULL && strlen(db_avatar_path) > 0) {
            load_avatar_data(db_avatar_path, &response_msg->user_info);
        } else {
            response_msg->user_info.avatar_size = 0;
            memset(response_msg->user_info.avatar_data, 0, MAX_AVATAR_SIZE);
        }

        // // 同时填充到user_info参数中（如果调用方需要）
        // if (user_info != NULL) {
        //     user_info->status = 1;
        //     strcpy(user_info->user_account, login_msg->user_account);
        //     strcpy(user_info->user_name, db_username);
        //     user_info->avatar_size = response_msg->user_info.avatar_size;
        //     memcpy(user_info->avatar_data, response_msg->user_info.avatar_data, response_msg->user_info.avatar_size);
        // }
    }

    return 0;
}

int data_handler::load_avatar_data(const char* avatar_path, USER_INFO* user_info)
{
    if (avatar_path == NULL || strlen(avatar_path) == 0) {
        user_info->avatar_size = 0;
        return 0;
    }

    // 打开头像文件
    FILE* file = fopen(avatar_path, "rb");
    if (!file) {
        printf("无法打开头像文件: %s\n", avatar_path);
        user_info->avatar_size = 0;
        return -1;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 检查文件大小是否超过限制
    if (file_size > MAX_AVATAR_SIZE) {
        printf("头像文件过大: %s, 大小: %ld, 限制: %d\n", 
               avatar_path, file_size, MAX_AVATAR_SIZE);
        fclose(file);
        user_info->avatar_size = 0;
        return -1;
    }

    // 读取文件内容
    size_t bytes_read = fread(user_info->avatar_data, 1, file_size, file);
    fclose(file);

    if (bytes_read != (size_t)file_size) {
        printf("头像文件读取不完整: %s\n", avatar_path);
        user_info->avatar_size = 0;
        return -1;
    }

    user_info->avatar_size = (int)file_size;
    printf("成功加载头像: %s, 大小: %d 字节\n", avatar_path, user_info->avatar_size);
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


    // 处理头像保存
    std::string avatar_path = "";
    if (register_msg->avatar_size > 0) {
        avatar_path = save_avatar_to_disk(register_msg);
        if (avatar_path.empty()) {
            printf("头像保存失败，但继续完成用户注册\n");
            // 可以选择继续注册，只是没有头像
        }
    }

    string now_time = TimeUtils::getCurrentTimeString("%Y-%m-%d %H:%M");
    sprintf(sql, "insert into user_info values('%s', '%s', '%s', '%s', '%s');", 
        register_msg->user_name, register_msg->user_account, register_msg->user_password, now_time.c_str(), avatar_path.c_str());
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

string data_handler::save_avatar_to_disk(REGISTET_MSG *register_msg)
{
    const char* avatar_dir = "avatars";
    
    // 创建头像目录（如果不存在）
    struct stat st;
    if (stat(avatar_dir, &st) != 0) {
        if (mkdir(avatar_dir, 0755) != 0) {
            printf("创建头像目录失败: %s\n", avatar_dir);
            return "";
        }
    }
    
    // 直接用用户名作为文件名
    string filename = std::string(register_msg->user_account) + ".jpg";
    string full_path = std::string(avatar_dir) + "/" + filename;
    
    // 保存头像文件
    FILE* file = fopen(full_path.c_str(), "wb");
    if (!file) {
        printf("无法创建头像文件: %s\n", full_path.c_str());
        return "";
    }
    
    size_t written = fwrite(register_msg->avatar_data, 1, register_msg->avatar_size, file);
    fclose(file);
    
    if (written != static_cast<size_t>(register_msg->avatar_size)) {
        printf("头像文件写入不完整\n");
        remove(full_path.c_str());
        return "";
    }
    
    printf("头像保存成功: %s\n", full_path.c_str());
    return full_path;
}

string data_handler::save_avatar_to_disk(CREATE_GROUP_MSG *msg)
{
    const char* avatar_dir = "avatars_group";
    
    // 创建头像目录（如果不存在）
    struct stat st;
    if (stat(avatar_dir, &st) != 0) {
        if (mkdir(avatar_dir, 0755) != 0) {
            printf("创建头像目录失败: %s\n", avatar_dir);
            return "";
        }
    }
    
    // 直接用用户名作为文件名
    string filename = std::string(msg->group_account) + ".jpg";
    string full_path = std::string(avatar_dir) + "/" + filename;
    
    // 保存头像文件
    FILE* file = fopen(full_path.c_str(), "wb");
    if (!file) {
        printf("无法创建头像文件: %s\n", full_path.c_str());
        return "";
    }
    
    size_t written = fwrite(msg->avatar_data, 1, msg->avatar_size, file);
    fclose(file);
    
    if (written != static_cast<size_t>(msg->avatar_size)) {
        printf("头像文件写入不完整\n");
        remove(full_path.c_str());
        return "";
    }
    
    printf("头像保存成功: %s\n", full_path.c_str());
    return full_path;
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
    char* user_avatar_path = resultp[ncolumn+4];

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
    char* friend_avatar_path = resultp[ncolumn+4];


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
    sprintf(sql, "insert into friend_info values('%s', '%s', '%s', '%s', '%s', '%s', '%d', '%s');", add_friend_msg->user_account, user_name , user_avatar_path, add_friend_msg->friend_account, friend_name,  friend_avatar_path, 0, now_time.c_str());
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
        load_avatar_data(resultp[index++], (*friend_ask_notice_msg)->asks[i], 1);
        strcpy((*friend_ask_notice_msg)->asks[i].friend_acccount, resultp[index++]);
        strcpy((*friend_ask_notice_msg)->asks[i].friend_name, resultp[index++]);
        load_avatar_data(resultp[index++], (*friend_ask_notice_msg)->asks[i], 2);
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

int data_handler::load_avatar_data(const char* avatar_path, FRIEND_ADD_ASK &friend_add_ask, int type)
{
    if (avatar_path == NULL || strlen(avatar_path) == 0) {
        if(type == 1){
            friend_add_ask.user_avatar_size = 0;
        }else{
            friend_add_ask.friend_avatar_size = 0;
        }
        return 0;
    }

    // 打开头像文件
    FILE* file = fopen(avatar_path, "rb");
    if (!file) {
        printf("无法打开头像文件: %s\n", avatar_path);
        if(type == 1){
            friend_add_ask.user_avatar_size = 0;
        }else{
            friend_add_ask.friend_avatar_size = 0;
        }
        return -1;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 检查文件大小是否超过限制
    if (file_size > MAX_AVATAR_SIZE) {
        printf("头像文件过大: %s, 大小: %ld, 限制: %d\n", 
               avatar_path, file_size, MAX_AVATAR_SIZE);
        fclose(file);
        if(type == 1){
            friend_add_ask.user_avatar_size = 0;
        }else{
            friend_add_ask.friend_avatar_size = 0;
        }
        return -1;
    }

    // 读取文件内容
    size_t bytes_read;
    if(type == 1){
        bytes_read = fread(friend_add_ask.user_avatar_data, 1, file_size, file);
    }else{
        bytes_read = fread(friend_add_ask.friend_avatar_data, 1, file_size, file);
    }

    fclose(file);

    if (bytes_read != (size_t)file_size) {
        printf("头像文件读取不完整: %s\n", avatar_path);
        if(type == 1){
            friend_add_ask.user_avatar_size = 0;
        }else{
            friend_add_ask.friend_avatar_size = 0;
        }
        return -1;
    }

    if(type == 1){
        friend_add_ask.user_avatar_size = (int)file_size;
    }else{
        friend_add_ask.friend_avatar_size = (int)file_size;
    }

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
        if(strcmp(resultp[index], user_account) == 0){     // 添加者为用户
            strcpy((*friend_list_msg)->friends[i].user_account, resultp[index+3]);   // 好友为对方
            strcpy((*friend_list_msg)->friends[i].user_name, resultp[index+4]);
            load_avatar_data(resultp[index+5], &(*friend_list_msg)->friends[i]);
            index += ncolumn;
        }else{
            strcpy((*friend_list_msg)->friends[i].user_account, resultp[index]);
            strcpy((*friend_list_msg)->friends[i].user_name, resultp[index+1]);
            load_avatar_data(resultp[index+2], &(*friend_list_msg)->friends[i]);
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

    // strcpy((*friend_list_msg)->friends->user_account, user_info->user_account);
    // strcpy((*friend_list_msg)->friends->user_name, user_info->user_name);
    // (*friend_list_msg)->friends->status = user_info->status;
    memcpy(&(*friend_list_msg)->friends[0], user_info, sizeof(USER_INFO));

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
    load_avatar_data(resultp[index++], (*friend_ask_notice_msg)->asks[0], 1);
    strcpy((*friend_ask_notice_msg)->asks[0].friend_acccount, resultp[index++]);
    strcpy((*friend_ask_notice_msg)->asks[0].friend_name, resultp[index++]);
    load_avatar_data(resultp[index++], (*friend_ask_notice_msg)->asks[0], 2);
    (*friend_ask_notice_msg)->asks[0].friend_status = atoi(resultp[index++]);
    strcpy((*friend_ask_notice_msg)->asks[0].add_time, resultp[index++]);

    // strcpy((*friend_ask_notice_msg)->asks[0].user_account, resultp[index++]);
    // strcpy((*friend_ask_notice_msg)->asks[0].user_name, resultp[index++]);
    // strcpy((*friend_ask_notice_msg)->asks[0].friend_acccount, resultp[index++]);
    // strcpy((*friend_ask_notice_msg)->asks[0].friend_name, resultp[index++]);
    // (*friend_ask_notice_msg)->asks[0].friend_status = atoi(resultp[index++]);
    // strcpy((*friend_ask_notice_msg)->asks[0].add_time, resultp[index++]);


    printf("添加账号：%s\t添加账号昵称: %s\t被加账号: %s\t被加昵称: %s\t添加状态:%d\t添加时间: %s\t\n", (*friend_ask_notice_msg)->asks[0].user_account, 
    (*friend_ask_notice_msg)->asks[0].user_name,
    (*friend_ask_notice_msg)->asks[0].friend_acccount,(*friend_ask_notice_msg)->asks[0].friend_name,
    (*friend_ask_notice_msg)->asks[0].friend_status, (*friend_ask_notice_msg)->asks[0].add_time);


    sqlite3_free_table(resultp);
    return 0;
}

int data_handler::chatmsg_data_handle(CHAT_MSG *chat_msg, RESPONSE_MSG *response_msg)
{
    char *errmsg = NULL;
    sqlite3_stmt* stmt = nullptr;
    
    // SQL 插入语句
    const char* sql = "INSERT INTO msg_info_from_friend (sendtime, sender_account, receiver_account, msg_type, file_size, read_status, msg_content) "
                     "VALUES (?, ?, ?, ?, ?, ?, ?)";

    // 准备 SQL 语句
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        printf("准备SQL语句失败:%s\n", sqlite3_errmsg(m_db));
        return -1;
    }

    // 绑定参数
    sqlite3_bind_int(stmt, 1, chat_msg->msg_header.timestamp);  // sendtime
    sqlite3_bind_text(stmt, 2, chat_msg->sender_account, -1, SQLITE_TRANSIENT);  // sender_account
    sqlite3_bind_text(stmt, 3, chat_msg->receiver_account, -1, SQLITE_TRANSIENT); // receiver_account
    sqlite3_bind_int(stmt, 4, chat_msg->content_type);   // msg_type
    sqlite3_bind_int(stmt, 5, chat_msg->file_size);    // file_size
    sqlite3_bind_int(stmt, 6, chat_msg->read_status);        // read_status
    sqlite3_bind_text(stmt, 7, chat_msg->content, -1, SQLITE_TRANSIENT); // msg_content
    
    // 执行插入
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        printf("插入消息失败:%s\n", sqlite3_errmsg(m_db));
        sqlite3_finalize(stmt);
        return false;
    }

    // 获取插入的消息ID
    sqlite3_int64 outMessageId = sqlite3_last_insert_rowid(m_db);
    chat_msg->chat_msg_id = outMessageId;
    
    // 清理语句
    sqlite3_finalize(stmt);
    
    printf("插入消息成功，消息ID:%lld\n", outMessageId);

    return 0;
}

int data_handler::get_history_msg_handle(HISTORY_MSG_GET *HistoryMsgGet_msg , vector<CHAT_MSG*> &chat_msgs)
{
    char sql[1024];
    char *errmsg = NULL;
    char **resultp;
    int nrow;
    int ncolumn;

    sprintf(sql, "select *from msg_info_from_friend where (sender_account='%s' or receiver_account='%s');", HistoryMsgGet_msg->user_account, HistoryMsgGet_msg->user_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }

    int index = ncolumn;
    for (int i = 0; i < nrow; i++)
    {
        // 读取数据库字段
        int chat_msg_id = atoi(resultp[index++]);
        int sendtime = atoi(resultp[index++]);
        const char* sender_account = resultp[index++];
        const char* receiver_account = resultp[index++];
        int msg_type = atoi(resultp[index++]);
        int file_size = atoi(resultp[index++]);
        int read_status = atoi(resultp[index++]);
        const char* msg_content = resultp[index++];

        // 计算 CHAT_MSG 大小
        size_t content_length = strlen(msg_content) + 1;
        size_t total_size = offsetof(CHAT_MSG, content) + content_length;

        // 分配内存
        CHAT_MSG* chat_msg = (CHAT_MSG*)malloc(total_size);
        if (!chat_msg) {
            printf("内存分配失败，跳过消息 ID: %d\n", chat_msg_id);
            continue;
        }

        // 初始化内存
        memset(chat_msg, 0, total_size);

        // 填充消息头
        chat_msg->msg_header.msg_type = CHAT_MSG_NOTICE;
        chat_msg->msg_header.msg_length = total_size - sizeof(MSG_HEADER);
        chat_msg->msg_header.timestamp = sendtime;
        chat_msg->msg_header.total_count = 0;
        
        // 填充消息内容
        chat_msg->chat_msg_id = chat_msg_id;
        strncpy(chat_msg->sender_account, sender_account, ACCOUNT_SIZE-1);
        strncpy(chat_msg->receiver_account, receiver_account, ACCOUNT_SIZE-1);
        chat_msg->content_type = msg_type;
        chat_msg->file_size = file_size;
        chat_msg->read_status = read_status;
        strncpy(chat_msg->content, msg_content, content_length-1);
        
        // 添加到列表
        chat_msgs.push_back(chat_msg);
    }
    
    printf("历史消息查询完成，共 %d 条消息\n", nrow);
    sqlite3_free_table(resultp);

    return nrow;
}

int data_handler::update_chat_msg_status(vector<long long> &chat_msgs)
{

    int total_count = chat_msgs.size();

    printf("total_count:%d\n", total_count);
    int failed_count = 0;
    int success_count = 0;
    for (int i = 0; i < total_count; i++)
    {
        char sql[1024];
        char *errmsg = NULL;
        // printf("%lld\n", chat_msgs[i]);
        sprintf(sql, "update msg_info_from_friend set read_status=1 where (chat_msg_id=%lld);", chat_msgs[i]);

        int res = sqlite3_exec(m_db, sql, NULL, NULL, &errmsg);
        if (res != SQLITE_OK)
        {
            if (errmsg != NULL)
            {
                printf("更新消息id:%lld 失败:%s\n", chat_msgs[i], errmsg);
                sqlite3_free(errmsg);
                errmsg = NULL;
            }
            failed_count++;
        }
        success_count++;
    }
    
    printf("消息状态更新完成，完成 %d/%d 条消息，失败 %d 条\n", success_count, total_count, failed_count);

    return 0;
}

int data_handler::update_group_chat_msg_status(vector<long long> &chat_msgs)
{

    int total_count = chat_msgs.size();

    printf("total_count:%d\n", total_count);
    int failed_count = 0;
    int success_count = 0;
    for (int i = 0; i < total_count; i++)
    {
        char sql[1024];
        char *errmsg = NULL;
        // printf("%lld\n", chat_msgs[i]);
        sprintf(sql, "update msg_info_from_group set read_status=1 where (chat_msg_id=%lld);", chat_msgs[i]);

        int res = sqlite3_exec(m_db, sql, NULL, NULL, &errmsg);
        if (res != SQLITE_OK)
        {
            if (errmsg != NULL)
            {
                printf("更新群聊消息id:%lld 失败:%s\n", chat_msgs[i], errmsg);
                sqlite3_free(errmsg);
                errmsg = NULL;
            }
            failed_count++;
        }
        success_count++;
    }
    
    printf("群聊消息状态更新完成，完成 %d/%d 条消息，失败 %d 条\n", success_count, total_count, failed_count);

    return 0;
}

int data_handler:: create_group_handle(CREATE_GROUP_MSG *msg, RESPONSE_MSG *response_msg)
{
    char sql[1024];
    char *errmsg = NULL;
    char **resultp;
    int nrow;
    int ncolumn;

    sprintf(sql, "select *from group_info where (group_account='%s');", msg->group_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }

    if (nrow > 0)
    {
        printf("群聊:%s 已经存在\n", msg->group_account);
        strcpy(response_msg->response, "群聊已经存在\n");
        response_msg->success_flag = 0;
        return -1;
    }


    // 处理头像保存
    std::string avatar_path = "";
    if (msg->avatar_size > 0) {
        avatar_path = save_avatar_to_disk(msg);
        if (avatar_path.empty()) {
            printf("头像保存失败，但继续完成创建群聊\n");
            // 可以选择继续注册，只是没有头像
        }
    }

    string now_time = TimeUtils::getCurrentTimeString("%Y-%m-%d %H:%M:%S");
    sprintf(sql, "insert into group_info values('%s', '%s', '%s', '%s',  '%s',  %d);", 
        msg->group_name, msg->group_account, avatar_path.c_str(),  msg->creator_account, now_time.c_str(), 1);
    int res = sqlite3_exec(m_db, sql, NULL, NULL, &errmsg);
    if (res != SQLITE_OK)
    {
        if (errmsg != NULL)
        {
            printf("数据库用户创建群聊插入错误:%s\n", errmsg);
            sqlite3_free(errmsg);
            errmsg = NULL;
        }

        strcpy(response_msg->response, "数据库插入失败\n");
        response_msg->success_flag = 0;
        return -1;
    }


    sprintf(sql, "select *from user_info where (user_account='%s');", msg->creator_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }

    char user_name[NAME_SIZE];
    strcpy(user_name, resultp[ncolumn]);
    char* user_avatar_path = resultp[ncolumn+4];

    sprintf(sql, "insert into user_group values('%s', '%s', '%s', '%s',  '%s', '%s', '%s', %d, '%s');", 
      msg->creator_account,  user_name, user_avatar_path, msg->group_account, msg->group_name, avatar_path.c_str(),  msg->creator_account,  1, now_time.c_str());
    res = sqlite3_exec(m_db, sql, NULL, NULL, &errmsg);
    if (res != SQLITE_OK)
    {
        if (errmsg != NULL)
        {
            printf("数据库用户创建群聊插入错误:%s\n", errmsg);
            sqlite3_free(errmsg);
            errmsg = NULL;
        }

        strcpy(response_msg->response, "数据库插入失败\n");
        response_msg->success_flag = 0;
        return -1;
    }

    strcpy(response_msg->response, "创建群聊成功！\n");
    response_msg->success_flag = 1;

    return 0;
}

int data_handler::query_group(ACCOUNT_QUERY_MSG *account_query_msg, GROUP_QUERY_RESPONSE_MSG *response_msg)
{
    char sql[1024];
    char *errmsg = NULL;
    char **resultp;
    int nrow;
    int ncolumn;

    sprintf(sql, "select *from group_info where (group_account='%s');", account_query_msg->query_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }

    if (nrow == 0)
    {
        printf("查询的群组:%s 不存在\n", account_query_msg->query_account);
        strcpy(response_msg->response, "查询的群组不存在\n");
        response_msg->success_flag = 0;
        return -1;
    }

    int index = ncolumn;
    strcpy(response_msg->group_info.group_name, resultp[index]);
    strcpy(response_msg->group_info.group_account, resultp[index+1]);
    strcpy(response_msg->group_info.group_creator, resultp[index+3]);
    response_msg->group_info.member_count = atoi(resultp[index+5]);

    char* db_avatar_path = resultp[index+2];
    // 处理头像数据
    if (db_avatar_path != NULL && strlen(db_avatar_path) > 0) {
        load_avatar_data(db_avatar_path, &response_msg->group_info);
    } else {
        response_msg->group_info.avatar_size = 0;
        memset(response_msg->group_info.avatar_data, 0, MAX_AVATAR_SIZE);
    }

    // response_msg->user_info.status = *(int *)resultp[index+3];
    // printf("用户存储在数据库的密码：%s\n", resultp[index+1]);
    strcpy(response_msg->response, "成功查询到群组\n");
    response_msg->success_flag = 1;
    return 0;
}

int data_handler::load_avatar_data(const char* avatar_path, GROUP_INFO* group_info)
{
    if (avatar_path == NULL || strlen(avatar_path) == 0) {
        group_info->avatar_size = 0;
        return 0;
    }

    // 打开头像文件
    FILE* file = fopen(avatar_path, "rb");
    if (!file) {
        printf("无法打开头像文件: %s\n", avatar_path);
        group_info->avatar_size = 0;
        return -1;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 检查文件大小是否超过限制
    if (file_size > MAX_AVATAR_SIZE) {
        printf("头像文件过大: %s, 大小: %ld, 限制: %d\n", 
               avatar_path, file_size, MAX_AVATAR_SIZE);
        fclose(file);
        group_info->avatar_size = 0;
        return -1;
    }

    // 读取文件内容
    size_t bytes_read = fread(group_info->avatar_data, 1, file_size, file);
    fclose(file);

    if (bytes_read != (size_t)file_size) {
        printf("头像文件读取不完整: %s\n", avatar_path);
        group_info->avatar_size = 0;
        return -1;
    }

    group_info->avatar_size = (int)file_size;
    printf("成功加载头像: %s, 大小: %d 字节\n", avatar_path, group_info->avatar_size);
    return 0;
}

int data_handler:: addgroup_data_handle(ADD_GROUP_MSG *add_group_msg, RESPONSE_MSG *response_msg)
{
    char sql[1024];
    char *errmsg = NULL;
    char **resultp;
    int nrow;
    int ncolumn;

    // ************************  获取到用户昵称，用于填充添加好友消息字段  ******************************
    sprintf(sql, "select *from user_info where (user_account='%s');", add_group_msg->user_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }

    char user_name[NAME_SIZE];
    strcpy(user_name, resultp[ncolumn]);
    char* user_avatar_path = resultp[ncolumn+4];

    // ************************  查询添加的群组账号是否存在  ******************************
    resultp = NULL;
    nrow = 0;
    printf("group_account:%s\n", add_group_msg->group_account);
    sprintf(sql, "select *from group_info where (group_account='%s');", add_group_msg->group_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }

    if (nrow == 0)
    {
        printf("添加的群组:%s 不存在\n", add_group_msg->group_account);
        strcpy(response_msg->response, "添加的群组不存在\n");
        response_msg->success_flag = 0;
        return -1;
    }

    char group_name[NAME_SIZE];
    strcpy(group_name, resultp[ncolumn]);
    char* creator_account = resultp[ncolumn+3];
    char* group_avatar_path = resultp[ncolumn+2];


    // ************************  查询是否已经存在添加记录（针对正在等待回应status:0/已添加status：1）, 用户为添加者时 ******************************
    resultp = NULL;
    errmsg = NULL;
    sprintf(sql, "select *from user_group where (group_account='%s' and user_account='%s' and status!=-1);", add_group_msg->group_account, add_group_msg->user_account);
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
        int status = atoi(resultp[index+7]);
        printf("status:%d\n", status);
        if (status == 1)
        {
            printf("已经是群成员了\n");
            strcpy(response_msg->response, "你已是该群成员！\n");
            response_msg->success_flag = 0;
        }else if(status == 0){
            printf("已经添加过群聊了\n");
            strcpy(response_msg->response, "你已经请求添加过该群聊了, 请耐心等待群主回应\n");
            response_msg->success_flag = 0;
        }
        return -1;
    }

    resultp = NULL;
    errmsg = NULL;

    // ************************ 通过所有检查后，说明不存在status!=-1的记录，则进行插入新添加记录 ******************************
    string now_time = TimeUtils::getCurrentTimeString("%Y-%m-%d %H:%M:%S");
    sprintf(sql, "insert into user_group values('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%d', '%s');", 
        add_group_msg->user_account, user_name , user_avatar_path, add_group_msg->group_account, group_name,  group_avatar_path,  creator_account, 0, now_time.c_str());
    int res = sqlite3_exec(m_db, sql, NULL, NULL, &errmsg);
    if (res != SQLITE_OK)
    {
        if (errmsg != NULL)
        {
            printf("数据库用户:%s 添加群组插入错误:%s\n", add_group_msg->user_account, errmsg);
            sqlite3_free(errmsg);
            errmsg = NULL;
        }
        return -1;
    }

    sqlite3_free_table(resultp);
    response_msg->success_flag = 1;
    strcpy(response_msg->response, "发送加入群聊请求成功！\n");

    return 0;
}

int data_handler::get_addgroup_ask(char *user_account, GROUP_ASK_NOTICE_MSG **group_ask_notice_msg)
{
    char sql[1024];
    char *errmsg = NULL;
    char **resultp;
    int nrow;
    int ncolumn;

    //  1.数据库user_account ==  user_account and 数据库creator_account != user_account
    //  2.数据库user_account != user_account and 数据库creator_account == user_account

    // ************************  查询用户user_account的所有群组添加记录，不管是添加方还是群主，不管添加状态如何 ******************************
    sprintf(sql, "SELECT * FROM user_group WHERE (user_account = '%s' OR creator_account = '%s') AND user_account <> creator_account;", user_account, user_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }
    
    printf("nrow: %d\n", nrow);
    int header_size = sizeof(MSG_HEADER);
    int data_size = sizeof(GROUP_ADD_ASK) * nrow;
    int total_size = header_size + data_size;
    *group_ask_notice_msg = (GROUP_ASK_NOTICE_MSG*)malloc(total_size);
    memset(*group_ask_notice_msg, 0, total_size);
    // friend_ask_notice_msg->ask_num = nrow;
    (*group_ask_notice_msg)->msg_header.msg_length = data_size;
    (*group_ask_notice_msg)->msg_header.msg_type = ADD_GROUP_LIST_RESPONSE;
    (*group_ask_notice_msg)->msg_header.timestamp = time(NULL);
    (*group_ask_notice_msg)->msg_header.total_count = nrow;

    int index = ncolumn;
    for (int i = 0; i < nrow; i++)
    {
        strcpy((*group_ask_notice_msg)->asks[i].user_account, resultp[index++]);
        strcpy((*group_ask_notice_msg)->asks[i].user_name, resultp[index++]);
        load_avatar_data(resultp[index++], (*group_ask_notice_msg)->asks[i], 1);
        strcpy((*group_ask_notice_msg)->asks[i].group_acccount, resultp[index++]);
        strcpy((*group_ask_notice_msg)->asks[i].group_name, resultp[index++]);
        load_avatar_data(resultp[index++], (*group_ask_notice_msg)->asks[i], 2);
        strcpy((*group_ask_notice_msg)->asks[i].creator_account, resultp[index++]);
        (*group_ask_notice_msg)->asks[i].status = atoi(resultp[index++]);
        strcpy((*group_ask_notice_msg)->asks[i].add_time, resultp[index++]);
    }

    for (int i = 0; i < (*group_ask_notice_msg)->msg_header.total_count; i++)
    {
        printf("添加账号：%s\t添加者昵称: %s\t群聊账号: %s\t群聊名称: %s\t添加状态:%d\t添加时间: %s\t\n", (*group_ask_notice_msg)->asks[i].user_account, 
        (*group_ask_notice_msg)->asks[i].user_name,
        (*group_ask_notice_msg)->asks[i].group_acccount,(*group_ask_notice_msg)->asks[i].group_name,
        (*group_ask_notice_msg)->asks[i].status, (*group_ask_notice_msg)->asks[i].add_time);
    }

    sqlite3_free_table(resultp);
    return 0;
}

int data_handler::load_avatar_data(const char* avatar_path, GROUP_ADD_ASK &group_add_ask, int type)
{
        if (avatar_path == NULL || strlen(avatar_path) == 0) {
        if(type == 1){
            group_add_ask.user_avatar_size = 0;
        }else{
            group_add_ask.group_avatar_size = 0;
        }
        return 0;
    }

    // 打开头像文件
    FILE* file = fopen(avatar_path, "rb");
    if (!file) {
        printf("无法打开头像文件: %s\n", avatar_path);
        if(type == 1){
            group_add_ask.user_avatar_size = 0;
        }else{
            group_add_ask.group_avatar_size = 0;
        }
        return -1;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 检查文件大小是否超过限制
    if (file_size > MAX_AVATAR_SIZE) {
        printf("头像文件过大: %s, 大小: %ld, 限制: %d\n", 
               avatar_path, file_size, MAX_AVATAR_SIZE);
        fclose(file);
        if(type == 1){
            group_add_ask.user_avatar_size = 0;
        }else{
            group_add_ask.group_avatar_size = 0;
        }
        return -1;
    }

    // 读取文件内容
    size_t bytes_read;
    if(type == 1){
        bytes_read = fread(group_add_ask.user_avatar_data, 1, file_size, file);
    }else{
        bytes_read = fread(group_add_ask.group_avatar_data, 1, file_size, file);
    }

    fclose(file);

    if (bytes_read != (size_t)file_size) {
        printf("头像文件读取不完整: %s\n", avatar_path);
        if(type == 1){
            group_add_ask.user_avatar_size = 0;
        }else{
            group_add_ask.group_avatar_size = 0;
        }
        return -1;
    }

    if(type == 1){
        group_add_ask.user_avatar_size = (int)file_size;
    }else{
        group_add_ask.group_avatar_size = (int)file_size;
    }

    return 0;
}

int data_handler::consutrct_group_ask_notice_msg(ADD_GROUP_MSG *add_group_msg, GROUP_ASK_NOTICE_MSG **group_ask_notice_msg)
{
    char sql[1024];
    char *errmsg = NULL;
    char **resultp;
    int nrow;
    int ncolumn;

    // ************************  查询时间最近的这条群聊添加记录   用于群聊新申请、群聊通过/拒绝 后发送群聊申请列表的更新  ******************************
    sprintf(sql, "select *from user_group where (user_account='%s' and group_account='%s') order by add_time desc limit 1;", add_group_msg->user_account, add_group_msg->group_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }
    
    int header_size = sizeof(MSG_HEADER);
    int data_size = sizeof(GROUP_ADD_ASK) * nrow;
    int total_size = header_size + data_size;
    *group_ask_notice_msg = (GROUP_ASK_NOTICE_MSG*)malloc(total_size);
    memset(*group_ask_notice_msg, 0, total_size);
    // friend_ask_notice_msg->ask_num = nrow;
    (*group_ask_notice_msg)->msg_header.msg_length = data_size;
    (*group_ask_notice_msg)->msg_header.msg_type = ADD_GROUP_NOTICE;
    (*group_ask_notice_msg)->msg_header.timestamp = time(NULL);
    (*group_ask_notice_msg)->msg_header.total_count = nrow;

    int index = ncolumn;

    strcpy((*group_ask_notice_msg)->asks[0].user_account, resultp[index++]);
    strcpy((*group_ask_notice_msg)->asks[0].user_name, resultp[index++]);
    load_avatar_data(resultp[index++], (*group_ask_notice_msg)->asks[0], 1);
    strcpy((*group_ask_notice_msg)->asks[0].group_acccount, resultp[index++]);
    strcpy((*group_ask_notice_msg)->asks[0].group_name, resultp[index++]);
    load_avatar_data(resultp[index++], (*group_ask_notice_msg)->asks[0], 2);
    strcpy((*group_ask_notice_msg)->asks[0].creator_account, resultp[index++]);
    (*group_ask_notice_msg)->asks[0].status = atoi(resultp[index++]);
    strcpy((*group_ask_notice_msg)->asks[0].add_time, resultp[index++]);


    for (int i = 0; i < (*group_ask_notice_msg)->msg_header.total_count; i++)
    {
        printf("添加账号：%s\t添加者昵称: %s\t群聊账号: %s\t群聊名称: %s\t添加状态:%d\t添加时间: %s\t\n", (*group_ask_notice_msg)->asks[i].user_account, 
        (*group_ask_notice_msg)->asks[i].user_name,
        (*group_ask_notice_msg)->asks[i].group_acccount,(*group_ask_notice_msg)->asks[i].group_name,
        (*group_ask_notice_msg)->asks[i].status, (*group_ask_notice_msg)->asks[i].add_time);
    }

    sqlite3_free_table(resultp);
    return 0;
}

int data_handler:: updata_group_data_handle(ADD_GROUP_MSG *add_group_msg, RESPONSE_MSG *response_msg, int choice)
{
    char sql[1024];
    char *errmsg = NULL;

    // ************************  查询用户的群聊添加记录(作为群主)，添加状态为正在验证：status：0 ******************************
    if (choice == ACCEPT_GROUP_ASK)
    {
        sprintf(sql, "update user_group set status=1 where (user_account='%s' and group_account='%s' and status=0);", add_group_msg->user_account, add_group_msg->group_account);
    }else{
        sprintf(sql, "update user_group set status=-1 where (user_account='%s' and group_account='%s' and status=0);", add_group_msg->user_account, add_group_msg->group_account);
    }

    int res = sqlite3_exec(m_db, sql, NULL, NULL, &errmsg);
    if (res != SQLITE_OK)
    {
        if (errmsg != NULL)
        {
            printf("数据库群聊:%s 更新用户入群申请信息错误:%s\n", add_group_msg->group_account, errmsg);
            sqlite3_free(errmsg);
            errmsg = NULL;
        }
        strcpy(response_msg->response, errmsg);
        response_msg->success_flag = 0;
        return -1;
    }

    if (choice == ACCEPT_GROUP_ASK){
        // 群组成员数加1
        char sql2[1024];
        sprintf(sql2, "UPDATE group_info SET member_num = member_num + 1 WHERE group_account = '%s';", 
                add_group_msg->group_account);

        if(sqlite3_exec(m_db, sql2, NULL, NULL, &errmsg) != SQLITE_OK) {
            printf("更新成员数失败: %s\n", errmsg);
            sqlite3_free(errmsg);
            return -1;
        }
    }
    return 0;
}

int data_handler::get_grouplist(char *user_account, GROUP_LIST_MSG **group_list_msg)
{
    char sql[1024];
    char *errmsg = NULL;
    char **resultp;
    int nrow;
    int ncolumn;

    // ************************  查询用户的群组列表，状态为已添加 ******************************
    sprintf(sql, "select *from user_group where user_account='%s' and status=1;", user_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }
    
    printf("nrow: %d\n", nrow);
    int header_size = sizeof(MSG_HEADER);
    int data_size = sizeof(GROUP_INFO) * nrow;
    int total_size = header_size + data_size;
    *group_list_msg = (GROUP_LIST_MSG*)malloc(total_size);
    memset(*group_list_msg, 0, total_size);
    // friend_ask_notice_msg->ask_num = nrow;
    (*group_list_msg)->msg_header.msg_length = data_size;
    (*group_list_msg)->msg_header.msg_type = GROUP_LIST_RESPONSE;
    (*group_list_msg)->msg_header.timestamp = time(NULL);
    (*group_list_msg)->msg_header.total_count = nrow;

    int index = ncolumn;
    for (int i = 0; i < nrow; i++)
    {
        strcpy((*group_list_msg)->groups[i].group_account, resultp[index+3]);
        strcpy((*group_list_msg)->groups[i].group_name, resultp[index+4]);
        load_avatar_data(resultp[index+5], &(*group_list_msg)->groups[i]);
        strcpy((*group_list_msg)->groups[i].group_creator, resultp[index+6]);

        char sql2[1024];
        char *errmsg2 = NULL;
        char **resultp2 = NULL;
        int nrow2 = 0;
        int ncolumn2 = 0;
        sprintf(sql2, "select *from group_info where group_account='%s';", resultp[index+3]);
        if(sqlite3_get_table(m_db, sql2, &resultp2, &nrow2, &ncolumn2, &errmsg2) != SQLITE_OK)
        {
            printf("%s", errmsg2);
            sqlite3_free(errmsg2);
            errmsg2 = NULL;
            return -1;
        }

        if(nrow2 > 0){
            printf("群人数: %d\n", atoi(resultp2[ncolumn2+5]));
            (*group_list_msg)->groups[i].member_count = atoi(resultp2[ncolumn2+5]);
        }

        index += ncolumn;
        sqlite3_free_table(resultp2);
    }

    for (int i = 0; i < (*group_list_msg)->msg_header.total_count; i++)
    {
        printf("群聊账号：%s\t 群聊昵称: %s\t \n", (*group_list_msg)->groups[i].group_account, (*group_list_msg)->groups[i].group_name);
    }

    sqlite3_free_table(resultp);
    return 0;
}

int data_handler::consturct_group_notice_msg(GROUP_INFO *group_info, GROUP_LIST_MSG **group_list_msg)
{
    int header_size = sizeof(MSG_HEADER);
    int data_size = sizeof(GROUP_INFO);
    int total_size = header_size + data_size;
    *group_list_msg = (GROUP_LIST_MSG*)malloc(total_size);
    memset(*group_list_msg, 0, total_size);
    // friend_ask_notice_msg->ask_num = nrow;
    (*group_list_msg)->msg_header.msg_length = data_size;
    (*group_list_msg)->msg_header.msg_type = GROUP_STATUS_NOTICE;
    (*group_list_msg)->msg_header.timestamp = time(NULL);
    (*group_list_msg)->msg_header.total_count = 1;

    memcpy(&(*group_list_msg)->groups[0], group_info, sizeof(GROUP_INFO));

    return 0;
}

int data_handler::get_users_in_group(vector<USER_INFO> &users_in_group, char* group_account)
{
    char sql[1024];
    char *errmsg = NULL;
    char **resultp;
    int nrow;
    int ncolumn;

    sprintf(sql, "select *from user_group where (group_account='%s' and status=1);", group_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }

    int index = ncolumn;
    for (int i = 0; i < nrow; i++)
    {
        // 读取数据库字段
        char* user_name = resultp[index+1];
        char* user_account = resultp[index];
        char* avatar_path = resultp[index+2];

        USER_INFO user_info; 
        // 初始化内存
        memset(&user_info, 0, sizeof(USER_INFO));
        
        // 填充消息内容
        strncpy(user_info.user_name, user_name, NAME_SIZE-1);
        strncpy(user_info.user_account, user_account, ACCOUNT_SIZE-1);
        load_avatar_data(avatar_path, &user_info);
        
        // 添加到列表
        users_in_group.push_back(user_info);
        index += ncolumn;
    }
    
    printf("群成员获取完成，共 %d 个成员\n", nrow);
    sqlite3_free_table(resultp);

    return nrow;
}

int data_handler::get_users_account_in_group(vector<char *> &users_account_in_group, char* group_account)
{
    char sql[1024];
    char *errmsg = NULL;
    char **resultp;
    int nrow;
    int ncolumn;

    sprintf(sql, "select *from user_group where (group_account='%s' and status=1);", group_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }

    int index = ncolumn;
    for (int i = 0; i < nrow; i++)
    {
        // 深拷贝：分配新内存并复制字符串
        char* user_account = resultp[index];
        char* account_copy = (char*)malloc(strlen(user_account) + 1);
        if (account_copy) {
            strcpy(account_copy, user_account);
            users_account_in_group.push_back(account_copy);
        }
        index += ncolumn;
    }
    
    printf("群成员获取完成，共 %d 个成员\n", nrow);
    sqlite3_free_table(resultp);

    return nrow;
}

// int data_handler::group_chatmsg_data_handle(CHAT_MSG *chat_msg, RESPONSE_MSG *response_msg)
// {
//     vector<char *> users_account_in_group;
//     get_users_account_in_group(users_account_in_group, chat_msg->receiver_account);

//     char *errmsg = NULL;
//     sqlite3_stmt* stmt = nullptr;
    
//     // SQL 插入语句
//     const char* sql = "INSERT INTO msg_info_from_friend (sendtime, sender_account, group_receiver_account, receiver_account, msg_type, file_size, read_status, msg_content) "
//                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

//     // 准备 SQL 语句
//     int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
//     if (rc != SQLITE_OK) {
//         printf("准备SQL语句失败:%s\n", sqlite3_errmsg(m_db));
//         return -1;
//     }

//     // 绑定参数
//     sqlite3_bind_int(stmt, 1, chat_msg->msg_header.timestamp);  // sendtime
//     sqlite3_bind_text(stmt, 2, chat_msg->sender_account, -1, SQLITE_TRANSIENT);  // sender_account
//     sqlite3_bind_text(stmt, 3, chat_msg->receiver_account, -1, SQLITE_TRANSIENT); // group_receiver_account
//     // sqlite3_bind_text(stmt, 4, chat_msg->receiver_account, -1, SQLITE_TRANSIENT); // receiver_account
//     sqlite3_bind_int(stmt, 5, chat_msg->content_type);   // msg_type
//     sqlite3_bind_int(stmt, 6, chat_msg->file_size);    // file_size
//     sqlite3_bind_int(stmt, 7, chat_msg->read_status);        // read_status
//     sqlite3_bind_text(stmt, 8, chat_msg->content, -1, SQLITE_TRANSIENT); // msg_content
    
//     // 执行插入
//     rc = sqlite3_step(stmt);
//     if (rc != SQLITE_DONE) {
//         printf("插入消息失败:%s\n", sqlite3_errmsg(m_db));
//         sqlite3_finalize(stmt);
//         return false;
//     }

//     // 获取插入的消息ID
//     sqlite3_int64 outMessageId = sqlite3_last_insert_rowid(m_db);
//     chat_msg->chat_msg_id = outMessageId;
    
//     // 清理语句
//     sqlite3_finalize(stmt);
    
//     printf("插入消息成功，消息ID:%lld\n", outMessageId);

//     return 0;
// }

int data_handler::group_chatmsg_data_handle(CHAT_MSG *chat_msg, RESPONSE_MSG *response_msg, vector<sqlite3_int64>& message_ids)
{
    vector<char *> users_account_in_group;
    get_users_account_in_group(users_account_in_group, chat_msg->receiver_account);

    if (users_account_in_group.empty()) {
        return -1;
    }

    char *errmsg = NULL;
    sqlite3_stmt* stmt = nullptr;
    
    // SQL 插入语句
    const char* sql = "INSERT INTO msg_info_from_group (sendtime, sender_account, group_receiver_account, receiver_account, msg_type, file_size, read_status, msg_content) "
                     "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

    // 开始事务
    if (sqlite3_exec(m_db, "BEGIN TRANSACTION", NULL, NULL, &errmsg) != SQLITE_OK) {
        printf("开始事务失败:%s\n", errmsg);
        return -1;
    }

    // 准备 SQL 语句
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        printf("准备SQL语句失败:%s\n", sqlite3_errmsg(m_db));
        sqlite3_exec(m_db, "ROLLBACK", NULL, NULL, NULL);
        return -1;
    }

    bool success = true;
    sqlite3_int64 last_insert_id = 0;

    // 为每个群成员插入消息
    for (size_t i = 0; i < users_account_in_group.size(); i++) {
        // 重置语句以便重新绑定参数
        sqlite3_reset(stmt);
        
        // 绑定参数
        sqlite3_bind_int(stmt, 1, chat_msg->msg_header.timestamp);
        sqlite3_bind_text(stmt, 2, chat_msg->sender_account, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, chat_msg->receiver_account, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, users_account_in_group[i], -1, SQLITE_TRANSIENT); // 使用数组中的user_account
        sqlite3_bind_int(stmt, 5, chat_msg->content_type);
        sqlite3_bind_int(stmt, 6, chat_msg->file_size);
        if (strcmp(chat_msg->sender_account, users_account_in_group[i]) == 0)
        {
            sqlite3_bind_int(stmt, 7, 1);
        }else{
            sqlite3_bind_int(stmt, 7, chat_msg->read_status);
        }

        sqlite3_bind_text(stmt, 8, chat_msg->content, -1, SQLITE_TRANSIENT);
        
        // 执行插入
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            printf("插入群消息失败:%s\n", sqlite3_errmsg(m_db));
            success = false;
            break;
        }
        
        // 获取并保存当前插入的消息ID
        sqlite3_int64 current_msg_id = sqlite3_last_insert_rowid(m_db);
        message_ids.push_back(current_msg_id);
    }

    // 提交或回滚事务
    if (success) {
        if (sqlite3_exec(m_db, "COMMIT", NULL, NULL, &errmsg) != SQLITE_OK) {
            printf("提交事务失败:%s\n", errmsg);
            success = false;
        } else {
            printf("成功插入 %zu 条群消息，获取到 %zu 个消息ID\n", 
                   users_account_in_group.size(), message_ids.size());
        }
    } else {
        sqlite3_exec(m_db, "ROLLBACK", NULL, NULL, NULL);
    }

    // 清理语句
    sqlite3_finalize(stmt);

    // 清理内存（如果使用深拷贝的话）
    for (char* account : users_account_in_group) {
        free(account);
    }
    users_account_in_group.clear();

    return success ? 0 : -1;
}

int data_handler::get_group_history_msg_handle(HISTORY_MSG_GET *HistoryMsgGet_msg , vector<CHAT_MSG*> &chat_msgs)
{
    char sql[1024];
    char *errmsg = NULL;
    char **resultp;
    int nrow;
    int ncolumn;

    sprintf(sql, "select *from msg_info_from_group where receiver_account='%s';", HistoryMsgGet_msg->user_account);
    if(sqlite3_get_table(m_db, sql, &resultp, &nrow, &ncolumn, &errmsg) != SQLITE_OK)
    {
        printf("%s", errmsg);
        sqlite3_free(errmsg);
        errmsg = NULL;
        return -1;
    }

    int index = ncolumn;
    for (int i = 0; i < nrow; i++)
    {
        // 读取数据库字段
        int chat_msg_id = atoi(resultp[index++]);
        int sendtime = atoi(resultp[index++]);
        const char* sender_account = resultp[index++];
        const char* receiver_account = resultp[index++];
        index++;
        int msg_type = atoi(resultp[index++]);
        int file_size = atoi(resultp[index++]);
        int read_status = atoi(resultp[index++]);
        const char* msg_content = resultp[index++];

        // 计算 CHAT_MSG 大小
        size_t content_length = strlen(msg_content) + 1;
        size_t total_size = offsetof(CHAT_MSG, content) + content_length;

        // 分配内存
        CHAT_MSG* chat_msg = (CHAT_MSG*)malloc(total_size);
        if (!chat_msg) {
            printf("内存分配失败，跳过消息 ID: %d\n", chat_msg_id);
            continue;
        }

        // 初始化内存
        memset(chat_msg, 0, total_size);

        // 填充消息头
        chat_msg->msg_header.msg_type = GROUP_CHAT_MSG_NOTICE;
        chat_msg->msg_header.msg_length = total_size - sizeof(MSG_HEADER);
        chat_msg->msg_header.timestamp = sendtime;
        chat_msg->msg_header.total_count = 0;
        
        // 填充消息内容
        chat_msg->chat_msg_id = chat_msg_id;
        strncpy(chat_msg->sender_account, sender_account, ACCOUNT_SIZE-1);
        strncpy(chat_msg->receiver_account, receiver_account, ACCOUNT_SIZE-1);
        chat_msg->content_type = msg_type;
        chat_msg->file_size = file_size;
        chat_msg->read_status = read_status;
        strncpy(chat_msg->content, msg_content, content_length-1);
        
        // 添加到列表
        chat_msgs.push_back(chat_msg);
    }
    
    printf("群聊历史消息查询完成，共 %d 条消息\n", nrow);
    sqlite3_free_table(resultp);

    return nrow;
}