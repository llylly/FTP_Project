#include "mainsocket.h"

extern GlobalSettings *globalSettings;

int MainSocket::mainFd = 0;
vector<pair<thread*, Handler*>> MainSocket::threadPool;

MainSocket::MainSocket() {

}

int MainSocket::init() {
    mainFd = MSocket::msocket(AF_INET, SOCK_STREAM, 0);
    sockaddr *mainSockAddr = GlobalSettings::genSockAddr(LOCAL_ADDR, globalSettings->port);
    int ret = MSocket::mbind(mainFd, mainSockAddr, sizeof(sockaddr));
    if (ret) {
        printf("Error: bind main socket error\n");
        return ret;
    }
    ret = MSocket::mlisten(mainFd, 0);
    if (ret) {
        printf("Error: listen main socket error\n");
        return ret;
    }
    sockaddr_in *clientAddr = new sockaddr_in();
    int clientAddrLen = sizeof(sockaddr_in);
    while (true) {
        int newSockFd = MSocket::maccept(mainFd, (void*)clientAddr, &clientAddrLen);
        clientAddr->sin_port = ntohs(clientAddr->sin_port);
        clientAddr->sin_addr.s_addr = ntohl(clientAddr->sin_addr.s_addr);
        sockaddr_in clientAddrNew = *clientAddr;
        Handler *newHandler = new Handler(newSockFd, clientAddrNew);
        thread *newThread = new thread(Handler::run, newHandler);
        threadPool.push_back(make_pair(newThread, newHandler));

        int activeCnt = 0;
        for (int i=0; i < threadPool.size(); ++i) {
            pair<thread*, Handler*> &now = threadPool[i];
            if ((now.second) && (now.second->status == FINISH)) {
                now.first->join();
                printf("release a finish thread\n");
                delete now.first;
                delete now.second;
                now.first = 0;
                now.second = 0;
            }
            if ((now.second) && (now.second->status != FINISH))
               ++activeCnt;
        }
        printf("total active: %d\n", activeCnt);
    }

}
