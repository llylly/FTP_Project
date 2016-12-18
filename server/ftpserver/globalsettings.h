#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include <cstring>
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define USER_FILE "users.txt"

using namespace std;

typedef unsigned short ushort;

class GlobalSettings
{
public:
    GlobalSettings();

    ushort port;
        // port for controlling channel
    char *path;
        // path as root of file system

    static sockaddr *genSockAddr(char *_ip, ushort _port);
    static char *getIP(sockaddr_in*);

    bool userExist(const char *s);

    bool identify(const char *user, const char *pass);

    int lastSendFd;
    int toBreakFd;

private:
    map<string, string> userRec;

};

#endif // GLOBALSETTINGS_H
