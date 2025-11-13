#include "tcp_server.h"


TcpServer::TcpServer(const char *ip, const char *port, Business * business)
    : m_ip(ip), m_port(port), m_listen_fd(-1), epollfd(-1),m_business(business)
{
    memset(&m_server_addr, 0, sizeof(m_server_addr));
    memset(&m_client_addr, 0, sizeof(m_client_addr));
}

TcpServer::~TcpServer()
{

}

int TcpServer:: start()
{
    int res = setup_server();

    main_loop();

    if (res == -1)
    {
        perror("setup_server 错误");
    }
    return 1;
}


int TcpServer:: stop()
{
    // 关闭socket以唤醒epoll_wait
    if (m_listen_fd != -1) {
        close(m_listen_fd);
        m_listen_fd = -1;
    }

    g_thread_pool_free(m_business->m_pool, FALSE, TRUE);

    if (epollfd != -1) {
        close(epollfd);
        epollfd = -1;
    }
    return 1;
}

int TcpServer:: setup_server()
{
    m_listen_fd = socket(m_domain, m_type, 0);
    if (m_listen_fd == -1)
    {
        perror("socket");
    }

    m_server_addr.sin_family = AF_INET;
    m_server_addr.sin_addr.s_addr = inet_addr(m_ip);
    m_server_addr.sin_port = htons(atoi(m_port));

    socklen_t server_addr_len = sizeof(m_server_addr);

    if(bind(m_listen_fd, (struct sockaddr*)&m_server_addr, server_addr_len) == -1)
    {
        perror("bind");
        close(m_listen_fd);
        return -1;
    }

    if(listen(m_listen_fd, 10) == -1)
    {
        perror("listen");
        close(m_listen_fd);
        return -1;
    }

    // 创建epoll实例
    epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        perror("epoll_create1 failed");
        close(m_listen_fd);
        return -1;
    }

    // 把服务器socket设置为非阻塞，并添加到epoll监听（接收新连接）
    if (set_nonblocking(m_listen_fd) == -1)
    {
        close(m_listen_fd);
        close(epollfd);
        return -1;
    }

    // 定义监听事件
    struct epoll_event event;
    event.events = EPOLLIN;     //监听“有数据可读”事件
    event.data.fd = m_listen_fd;

    // 将服务器socket加入监听
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, m_listen_fd, &event) == -1)
    {
        perror("epoll_ctl add server_fd failed");
        close(m_listen_fd);
        close(epollfd);
        return -1;
    }
    return 1;
}

// 设置socket为非阻塞模式
int TcpServer:: set_nonblocking(int fd) 
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL failed");
        return -1;
    }
    return 0;
}

int TcpServer:: main_loop()
{
    // 事件循环
    struct epoll_event events[MAX_EVENTS];
    
    while (1)
    {
        // 等待事件，超时时间-1表示无限等待
        int num_events = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (num_events == -1)
        {
            perror("epoll_wait failed");
            close(m_listen_fd);
            close(epollfd);
            exit(EXIT_FAILURE);
        }

        // 处理每个事件
        for (int i = 0; i < num_events; i++)
        {
            int fd = events[i].data.fd;
            if (fd == m_listen_fd)
            {
                int client_fd = accept(m_listen_fd, NULL, NULL);
                if (client_fd == -1)
                {
                    perror("accept failed");
                    continue;
                }
                printf("客户端 %d 连接成功！\n", client_fd);

                // 为新客户端添加锁
                m_business->add_client_lock(client_fd);

                // 客户端socket设为非阻塞，并添加到epoll监听
                if (set_nonblocking(client_fd) == -1) {
                    close(client_fd);
                    continue;
                }

                epoll_event ev;
                ev.data.fd = client_fd;
                ev.events = EPOLLIN | EPOLLET;    // 边缘触发模式（高效）
                if(epoll_ctl(epollfd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
                {
                    perror("epoll_ctl add client_fd failed");
                    close(client_fd);
                    continue;
                }
            }else if (events[i].events & EPOLLIN)
            {
                // 尝试获取读锁，如果获取不到说明该客户端正在被其他线程处理
                auto read_lock = m_business->get_client_read_lock(fd);
                if (read_lock.owns_lock()) {
                    int *clientfd_ptr = new int(fd);
                    m_business->push_task(clientfd_ptr);
                } else {
                    printf("客户端 %d 正在被其他线程处理，跳过此次事件\n", fd);
                }

                // int *clientfd_ptr = new int(events[i].data.fd);
                // m_business->push_task(clientfd_ptr);
                // g_thread_pool_push(pool, clientfd_ptr, NULL);
            }
        }
    }
} 