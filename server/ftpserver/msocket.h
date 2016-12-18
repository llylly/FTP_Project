#ifndef MSOCKET_H
#define MSOCKET_H

#define NO_BUFFER

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include "globalsettings.h"

#define BUFFER_SIZE 1048576

class BufferNode {
public:
    BufferNode() {
        l = r = 0;
        packTot = 0;
        buf = new unsigned char[BUFFER_SIZE];
        nex = 0;
    }

    ~BufferNode() {
        delete[] buf;
    }

    int sockFd;
    unsigned char *buf;
    unsigned int l, r, packTot;
    BufferNode *nex;
};

class MSocket {
public:
    MSocket();

    static int msocket(int domain, int type, int protocol);

    static int mbind(int sockfd, struct sockaddr *my_addr, int addrlen);

    static int mconnect(int sockfd, sockaddr *serv_addr, int addrlen);

    static int mlisten(int sockfd, int backlog);

    static int maccept(int sockfd, void *addr, int *addrlen);

    static int msend(int sockfd, const void *msg, int len, int flags);

    static int mrecv(int sockfd, void *buf, int len, unsigned int flags);

    // use shutdown to close all
    static int mclose(int sockfd);

private:
    static BufferNode *first;

};

#endif // MSOCKET_H
