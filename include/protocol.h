#ifndef PROTOCOL_H
#define PROTOCOL_H

#define NAME_SIZE 64
#define ACCOUNT_SIZE 64
#define PASSWORD_SIZE 64
#define TIME_SIZE 64
#define RESPONSE_SIZE 128
#define MAX_AVATAR_SIZE 10240  // 10kB

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>

// 消息类型枚举
enum MessageType {
    LOGIN_REQUEST = 1,           // 登录请求
    REGISTER_REQUEST = 2,        // 注册请求
    ACCOUNT_QUERY_REQUEST = 3,   // 用户账号查询请求
    FRIEND_LIST_REQUEST = 4,     // 好友列表请求
    GROUP_LIST_REQUEST = 5,      // 群组列表请求
    ADD_FRIEND_REQUEST = 6,      // 好友添加请求
    CREATE_GROUP_REQUEST = 7,    // 创建群聊请求
    ADD_GROUP_REQUEST = 8,       // 添加群聊请求
    SEND_CHAT_MSG = 9,           // 发送聊天消息请求
    ACCEPT_FRIEND_ASK = 10,      // 同意好友申请
    REJECT_FRIEND_ASK = 11,      // 拒绝好友申请
    HISTORY_MSG_REQUEST = 12,      // 历史消息请求
    UPDATE_CHAT_MSG_REQUEST = 13,  // 更新消息阅读状态
    GROUP_QUERY_REQUEST = 14,     // 群组查询响应
    ACCEPT_GROUP_ASK = 15,      // 同意群组申请
    REJECT_GROUP_ASK = 16,      // 拒绝群组申请
    SEND_GROUP_CHAT_MSG = 17,   // 发送群聊聊天消息
    UPDATE_GROUP_CHAT_MSG_REQUEST = 18,  // 更新群聊消息阅读状态
    GROUP_MEMBER_QUERY = 19,      // 查询群成员请求

    LOGIN_RESPONSE = 21,          // 登录响应
    REGISTER_RESPONSE = 22,       // 注册响应
    USER_QUERY_RESPONSE = 23,     // 用户查询响应
    FRIEND_ADD_RESPONSE = 24,     // 好友添加响应
    FRIEND_ACCPET_RESPONSE = 25,  // 好友申请同意响应
    FRIEND_REJECT_RESPONSE = 26,  // 好友申请拒绝响应
    FRIEND_LIST_RESPONSE = 27,    // 好友列表响应   获取整个  好友列表
    ADD_FRIEND_LIST_RESPONSE = 28,  // 好友添加列表响应   获取整个  好友添加列表
    SEND_CHAT_RESPONSE = 29,       // 发送消息响应
    GROUP_LIST_RESPONSE = 30,     // 群组列表响应
    NORMAL_RESPONSE = 31,         // 普通响应
    HISTORY_MSG_RESPONSE = 32,      // 历史消息响应
    CREATE_GROUP_RESPONSE = 33,    // 创建群聊响应
    GROUP_QUERY_RESPONSE = 34,     // 群组查询响应
    GROUP_ADD_RESPONSE = 35,       // 群组添加响应
    ADD_GROUP_LIST_RESPONSE = 36,  // 群组添加列表响应
    GROUP_ACCPET_RESPONSE = 37,    // 群聊申请同意响应
    GROUP_REJECT_RESPONSE = 38,    // 群聊申请拒绝响应
    SEND_GROUP_CHAT_RESPONSE = 39, // 群聊消息响应
    GROUP_MEMBER_QUERY_RESPONSE = 40,  // 群成员信息响应

    ADD_FRIEND_NOTICE = 41,       // 好友添加通知
    FRIEND_STATUS_NOTICE = 42,    // 好友上线/离线通知
    CHAT_MSG_NOTICE = 43,         // 消息通知
    ADD_GROUP_NOTICE = 44,        // 群聊通知
    GROUP_STATUS_NOTICE = 45,     // 群聊状态变化通知
    GROUP_CHAT_MSG_NOTICE = 46,   // 群聊消息通知

    MSG_CHAT_TEXT = 51,               // 文本聊天
    MSG_CHAT_FILE = 52,               // 文件传输
};

// 消息头定义
typedef struct{
    int msg_type;
    int msg_length;
    int timestamp;
    int total_count;  // 好友，群组列表专用
}MSG_HEADER;

     //主动发送：
     // 登录、注册、用户查询、添加好友、创建群聊、添加群、发送消息/文件
//注册消息
typedef struct{
    MSG_HEADER msg_header;
    char user_name[NAME_SIZE];
    char user_account[ACCOUNT_SIZE];
    char user_password[PASSWORD_SIZE];
    int avatar_size;
    char avatar_data[MAX_AVATAR_SIZE]; // 头像二进制数据
}REGISTET_MSG;

//登录
typedef struct{
    MSG_HEADER msg_header;
    char user_account[ACCOUNT_SIZE];
    char user_password[PASSWORD_SIZE];
}LOGIN_MSG;

//用户账号查询
typedef struct{
    MSG_HEADER msg_header;
    char user_account[ACCOUNT_SIZE];
    char query_account[ACCOUNT_SIZE];
}ACCOUNT_QUERY_MSG;

// 好友列表/群组列表请求
typedef struct{
    MSG_HEADER msg_header;
    char user_account[ACCOUNT_SIZE];
}LIST_REQUEST;

//添加好友
typedef struct{
    MSG_HEADER msg_header;
    char user_account[ACCOUNT_SIZE];
    char friend_account[ACCOUNT_SIZE];
}ADD_FRIEND_MSG;

//创建群
typedef struct{
    MSG_HEADER msg_header;
    char group_account[ACCOUNT_SIZE];
    char group_name[NAME_SIZE];
    int avatar_size;
    char avatar_data[MAX_AVATAR_SIZE]; // 头像二进制数据
    char creator_account[ACCOUNT_SIZE];  // 群主账号
}CREATE_GROUP_MSG;

//添加群
typedef struct{
    MSG_HEADER msg_header;
    char user_account[ACCOUNT_SIZE];
    char group_account[ACCOUNT_SIZE];
}ADD_GROUP_MSG;

//发送消息
typedef struct{
    MSG_HEADER msg_header;
    char sender_account[ACCOUNT_SIZE];
    char receiver_account[ACCOUNT_SIZE];
    long long chat_msg_id;     // 记录消息号，便于查找
    int content_type;    // 内容类型：文本、图片、文件等
    int file_size;       // 文件大小
    int read_status;     // 阅读状态
    char content[];
}CHAT_MSG;

typedef struct {
    MSG_HEADER msg_header;
    char user_account[ACCOUNT_SIZE];
    // char session_id[ACCOUNT_SIZE];  // 会话ID
} HISTORY_MSG_GET;

//更新消息阅读状态
typedef struct{
    MSG_HEADER msg_header;
    std::vector<long long> chat_msgs;     // 记录消息号，便于查找
}UPDATE_CHAT_MSG;

    // 被动接收：
    // 好友列表、群聊列表、好友上线提醒、好友离线信息、新消息提醒、好友添加提醒
//好友信息
typedef struct{
    char user_name[NAME_SIZE];
    char user_account[ACCOUNT_SIZE];
    int status;
    int avatar_size;
    char avatar_data[MAX_AVATAR_SIZE]; // 头像二进制数据
}USER_INFO;

//群组信息
typedef struct{
    char group_name[NAME_SIZE];
    char group_account[ACCOUNT_SIZE];
    char group_creator[ACCOUNT_SIZE];
    int member_count;
    int avatar_size;
    char avatar_data[MAX_AVATAR_SIZE]; // 头像二进制数据
}GROUP_INFO;

// 好友列表信息
typedef struct {
    MSG_HEADER msg_header;
    USER_INFO friends[];  // 好友列表柔性数组
} FRIEND_LIST_MSG;

// 群组列表信息
typedef struct {
    MSG_HEADER msg_header;
    GROUP_INFO groups[];    // 群组列表柔性数组
} GROUP_LIST_MSG;

// 单个好友申请请求
typedef struct 
{
    char user_account[ACCOUNT_SIZE];
    char user_name[NAME_SIZE];
    int user_avatar_size;
    char user_avatar_data[MAX_AVATAR_SIZE]; // 头像二进制数据
    char friend_acccount[ACCOUNT_SIZE];
    char friend_name[NAME_SIZE];
    int friend_avatar_size;
    char friend_avatar_data[MAX_AVATAR_SIZE]; // 头像二进制数据
    int friend_status;
    char add_time[TIME_SIZE];
}FRIEND_ADD_ASK;

// 好友添加请求提醒
typedef struct{
    MSG_HEADER msg_header;
    FRIEND_ADD_ASK asks[];
}FRIEND_ASK_NOTICE_MSG;

// 单个群聊申请请求
typedef struct
{
    char user_account[ACCOUNT_SIZE];
    char user_name[NAME_SIZE];
    int user_avatar_size;
    char user_avatar_data[MAX_AVATAR_SIZE]; // 头像二进制数据
    char group_acccount[ACCOUNT_SIZE];
    char group_name[NAME_SIZE];
    int group_avatar_size;
    char group_avatar_data[MAX_AVATAR_SIZE]; // 头像二进制数据
    char creator_account[ACCOUNT_SIZE];      // 创建者账号
    int status;
    char add_time[TIME_SIZE];
}GROUP_ADD_ASK;

// 群聊添加请求提醒
typedef struct{
    MSG_HEADER msg_header;
    GROUP_ADD_ASK asks[];
}GROUP_ASK_NOTICE_MSG;

// 好友上线，离线信息
typedef struct{
    MSG_HEADER msg_header;
    USER_INFO user_info;
}FRIEND_NOTICE_MSG;

// 用户登录响应，查询用户响应
typedef struct{
    MSG_HEADER msg_header;
    USER_INFO user_info;
    char response[RESPONSE_SIZE];
    int success_flag;
}USER_QUERY_RESPONSE_MSG;

// 群组查询响应
typedef struct{
    MSG_HEADER msg_header;
    GROUP_INFO group_info;
    char response[RESPONSE_SIZE];
    int success_flag;
}GROUP_QUERY_RESPONSE_MSG;

//群成员查询响应消息
typedef struct{
    MSG_HEADER msg_header;
    char group_account[ACCOUNT_SIZE];
    USER_INFO users[];
}GROUP_MEMBER_QUERY_RESPONSE_MSG;

// 用户请求普通响应
typedef struct{
    MSG_HEADER msg_header;
    char response[RESPONSE_SIZE];
    int success_flag;
}RESPONSE_MSG;

class TimeUtils{
public:
    // 获取当前时间字符串
    static std::string getCurrentTimeString(const std::string& format = "%Y-%m-%d %H:%M:%S");

};

// 定义小于运算符 - 基于 user_account
inline bool operator<(const USER_INFO& a, const USER_INFO& b) {
    return strcmp(a.user_account, b.user_account) < 0;
}

#endif // PROTOCOL_H
