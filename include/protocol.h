#ifndef PROTOCOL_H
#define PROTOCOL_H

#define NAME_SIZE 64
#define ACCOUNT_SIZE 64
#define PASSWORD_SIZE 64
#define RESPONSE_SIZE 128

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

// 消息类型枚举
enum MessageType {
    LOGIN_REQUEST = 1,           // 登录请求
    REGISTER_REQUEST = 2,        // 注册请求
    FRIEND_LIST_REQUEST = 3,     // 好友列表请求
    GROUP_LIST_REQUEST = 4,      // 群组列表请求
    ADD_FRIEND_REQUEST = 5,      // 好友添加请求
    CREATE_GROUP_REQUEST = 6,    // 创建群聊请求
    ADD_GROUP_REQUEST = 7,       // 添加群聊请求
    SEND_CHAT_MSG = 8,           // 发送聊天消息请求

    NORMAL_RESPONSE = 11,         // 普通响应
    FRIEND_LIST_RESPONSE = 12,    // 好友列表响应
    GROUP_LIST_RESPONSE = 13,     // 群组列表响应

    MSG_CHAT_TEXT = 21,               // 文本聊天
    MSG_CHAT_FILE = 22,               // 文件传输
};

// 消息头定义
typedef struct{
    int msg_type;
    int msg_length;
    int timestamp;
    int total_count;  // 好友，群组列表专用
}MSG_HEADER;

     //主动发送：
     // 登录、注册、添加好友、创建群聊、添加群、发送消息/文件
//注册消息
typedef struct{
    MSG_HEADER msg_header;
    char user_name[NAME_SIZE];
    char user_account[ACCOUNT_SIZE];
    char user_password[PASSWORD_SIZE];
}REGISTET_MSG;

//登录
typedef struct{
    MSG_HEADER msg_header;
    char user_account[ACCOUNT_SIZE];
    char user_password[PASSWORD_SIZE];
}LOGIN_MSG;

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
}CREATE_GROUP_MSG;

//添加群
typedef struct{
    MSG_HEADER msg_header;
    char user_account[ACCOUNT_SIZE];
    char group_name[NAME_SIZE];
}ADD_GROUP_MSG;

//发送消息
typedef struct{
    MSG_HEADER msg_header;
    char sender_account[ACCOUNT_SIZE];
    char receiver_account[ACCOUNT_SIZE];
    int chat_msg_id;     // 记录消息号，便于查找
    int content_type;    // 内容类型：文本、图片、文件等
    int read_status;     // 阅读状态
    char content[];
}CHAT_MSG;

    // 被动接收：
    // 好友列表、群聊列表、好友上线提醒、好友离线信息、新消息提醒、好友添加提醒
//好友信息
typedef struct{
    char friend_name[NAME_SIZE];
    char friend_account[ACCOUNT_SIZE];
    int status;
}FRIEND_INFO;

//群组信息
typedef struct{
    char group_name[NAME_SIZE];
    char group_account[ACCOUNT_SIZE];
    char group_creator[ACCOUNT_SIZE];
    int member_count;
}GROUP_INFO;

// 好友列表信息
typedef struct {
    MSG_HEADER msg_header;
    FRIEND_INFO friends[];  // 好友列表柔性数组
} FRIEND_LIST_MSG;

// 群组列表信息
typedef struct {
    MSG_HEADER msg_header;
    GROUP_INFO groups[];    // 群组列表柔性数组
} GROUP_LIST_MSG;

// 好友上线，离线信息，好友添加提醒
typedef struct{
    MSG_HEADER msg_header;
    FRIEND_INFO friend_info;
}FRIEND_NOTICE_MSG;

// 用户请求响应
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


#endif // PROTOCOL_H
