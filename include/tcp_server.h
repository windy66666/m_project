#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <netinet/in.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>       
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include "business.h"

class TcpServer {
public:
    TcpServer(const char *ip, const char *port, Business * business);
    ~TcpServer();
    int start();    // 启动服务器
    int stop();     // 停止服务器
    
private:
    int setup_server();             // 初始化服务器
    int set_nonblocking(int fd);    // 设置非阻塞
    int main_loop();                // 主循环

private:
    int m_domain = AF_INET;
    int m_type = SOCK_STREAM;
    const char* m_port;
    const char* m_ip;
    int m_listen_fd;
    struct sockaddr_in m_server_addr, m_client_addr;
    int epollfd;
    int MAX_EVENTS = 64;
    Business * m_business = NULL;
    // map<string, int> 
};

#endif // TCP_SERVER_H