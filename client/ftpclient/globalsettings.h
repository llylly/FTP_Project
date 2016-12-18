#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

using namespace std;

class GlobalSettings
{
public:
    GlobalSettings();

    sockaddr *servAddr;
    string hostPath;

    static sockaddr *genSockAddr(char *_ip, ushort _port);

};

#endif // GLOBALSETTINGS_H
