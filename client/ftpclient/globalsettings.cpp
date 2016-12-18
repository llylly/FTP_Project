#include "globalsettings.h"

GlobalSettings::GlobalSettings() {
}

sockaddr *GlobalSettings::genSockAddr(char *_ip, ushort _port) {
    sockaddr_in *myaddr = new sockaddr_in();
    myaddr->sin_family = AF_INET;
    myaddr->sin_port = htons(_port);
    myaddr->sin_addr.s_addr = inet_addr(_ip);
    memset(&(myaddr->sin_zero), 0, 8);
    return (sockaddr*)myaddr;
}
