#include <stdio.h>
#include "tcp_server.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage:%s serverip port\n", argv[0]);
        return -1;
    }
    data_handler datahandler("new.db");
    Business business(&datahandler);
    TcpServer tcpserver(argv[1], argv[2], &business);

    tcpserver.start();

                             
}