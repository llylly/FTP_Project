#ifndef MAINSOCKET_H
#define MAINSOCKET_H

#include <cstring>
#include <vector>
#include <thread>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include "msocket.h"
#include "globalsettings.h"
#include "handler.h"

#define LOCAL_ADDR "127.0.0.1"

#define TIMEOUT 1000L
    // 1000s more, think it's timeout and close

using namespace std;

class MainSocket
{
public:
    MainSocket();

    static int init();

private:
    static int mainFd;
    static vector<pair<thread*, Handler*>> threadPool;

};

#endif // MAINSOCKET_H
