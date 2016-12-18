#include "msocket.h"

BufferNode *MSocket::first = 0;

MSocket::MSocket() {
}

int MSocket::msocket(int domain, int type, int protocol) {
    int fd = socket(domain, type, protocol);
    // insert new fd to linklist
    BufferNode *p = first;
    while (p) {
        if (p->sockFd == fd) break;
        p = p->nex;
    }
    if (p == 0) {
        BufferNode *newBuf = new BufferNode();
        newBuf->sockFd = fd;
        newBuf->nex = first;
        first = newBuf;
    }
    return fd;
}

int MSocket::mbind(int sockfd, sockaddr *my_addr, int addrlen) {
    return bind(sockfd, my_addr, addrlen);
}

int MSocket::mconnect(int sockfd, sockaddr *serv_addr, int addrlen) {
    return connect(sockfd, serv_addr, addrlen);
}

int MSocket::mlisten(int sockfd, int backlog) {
    return listen(sockfd, backlog);
}

int MSocket::maccept(int sockfd, void *addr, int *addrlen) {
    int newfd = accept(sockfd, (sockaddr*)addr, (socklen_t*)addrlen);
    // insert new fd to linklist
    BufferNode *p = first;
    while (p) {
        if (p->sockFd == newfd) break;
        p = p->nex;
    }
    if (p == 0) {
        BufferNode *newBuf = new BufferNode();
        newBuf->sockFd = newfd;
        newBuf->nex = first;
        first = newBuf;
    }
    return newfd;
}

int MSocket::msend(int sockfd, const void *msg, int len, int flags) {
    // add a head to label length(in bytes) of packet
#ifdef NO_BUFFER
    return send(sockfd, msg, len, flags);
#else
    unsigned char *newMsg = new unsigned char[len + 4];
    unsigned int *head = (unsigned int*)newMsg;
    *head = htonl(len + 4);
    unsigned char *data = newMsg + 4;
    memcpy(data, msg, len);
    int ret = send(sockfd, newMsg, len + 4, flags);
    delete[] newMsg;
    return ret;
#endif
}

int MSocket::mrecv(int sockfd, void *buf, int len, unsigned int flags) {
    BufferNode *p = first;
    while (p) {
        if (p->sockFd == sockfd)
            break;
        p = p->nex;
    }
    if (p) {
#ifdef NO_BUFFER
        return recv(sockfd, buf, len, flags);
#else
        if (p->packTot == 0) {
            unsigned char *revBuf = new unsigned char[BUFFER_SIZE];
            ssize_t revLen = recv(sockfd, revBuf, BUFFER_SIZE, flags);
            if (revLen >= 0) {
                unsigned int *revLenP = (unsigned int*)revBuf;
                *revLenP = ntohl(*revLenP);
                for (int i=0; i<revLen; ++i, ++(p->r)) {
                    p->buf[(p->r) % BUFFER_SIZE] = revBuf[i];
                }

                p->packTot = 0;
                unsigned int visitor = p->l;
                while (true) {
                    unsigned int nowLen = *((unsigned int*)(p->buf + (visitor % BUFFER_SIZE)));
                    if (visitor + nowLen > (p->r)) break;
                    ++p->packTot;
                    if (visitor + nowLen == (p->r)) break;
                    visitor += nowLen;
                }
            }
            delete[] revBuf;
        }
        if (p->packTot) {
            unsigned int lenP = *((unsigned int*)(p->buf + ((p->l) % BUFFER_SIZE)));
            unsigned int &pl = p->l;
            --p->packTot;
            lenP -= 4;
            if (lenP > len) {
                printf("# Error: buffer too small\n");
                return -1;
            }
            pl += 4;
            for (int i = 0; i < lenP; ++i, ++pl) {
                *((unsigned char*)buf + i) = p->buf[pl % BUFFER_SIZE];
            }
            return lenP;
        } else {
            printf("# Error: unknown error\n");
            return -1;
        }
#endif
    } else {
        printf("# Error: illegal receiving socket fd\n");
        return -1;
    }
}

int MSocket::mclose(int sockfd) {
    // totally close
    int ret = shutdown(sockfd, 2);
    if (ret == 0) {
        // successfully closed
        // delete from linklist
        BufferNode *p = first, *pre;
        while (p) {
            if (p->sockFd == sockfd) {
                if (p == first)
                    first = p->nex;
                else
                    pre->nex = p->nex;
                delete p;
                break;
            }
            pre = p, p = p->nex;
        }
    }
    return  ret;
}

