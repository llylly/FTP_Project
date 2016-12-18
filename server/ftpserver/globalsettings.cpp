#include "globalsettings.h"

GlobalSettings::GlobalSettings() {
    lastSendFd = toBreakFd = -1;
    userRec.clear();
    ifstream userFin(USER_FILE);
    string user, pass;
    while (userFin >> user >> pass)
        userRec[user] = pass;
    userFin.close();
    userRec["anonymous"] = "";
}

sockaddr *GlobalSettings::genSockAddr(char *_ip, ushort _port) {
    sockaddr_in *myaddr = new sockaddr_in();
    myaddr->sin_family = AF_INET;
    myaddr->sin_port = htons(_port);
    myaddr->sin_addr.s_addr = inet_addr(_ip);
    memset(&(myaddr->sin_zero), 0, 8);
    return (sockaddr*)myaddr;
}

char *GlobalSettings::getIP(sockaddr_in *sockIn) {
    string s = "";
    unsigned int ip = sockIn->sin_addr.s_addr;
    // in host order
    unsigned int p4 = (ip >> 24) & 0xFF;
    unsigned int p3 = (ip >> 16) & 0xFF;
    unsigned int p2 = (ip >> 8) & 0xFF;
    unsigned int p1 = (ip) & 0xFF;
    stringstream stream;
    stream << p4 << ":" << p3 << ":" << p2 << ":" << p1;
    s = stream.str();
    char *ret = new char[s.length() + 1];
    for (int i=0; i < s.length() + 1; ++i)
        ret[i] = s[i];
    return ret;
}

bool GlobalSettings::userExist(const char *s) {
    string newS(s);
    if (newS[newS.size() - 1] == '\r') newS.erase(newS.size() - 1, 1);
    if (userRec.find(newS) == userRec.end()) return false;
    return true;
}

bool GlobalSettings::identify(const char *user, const char *pass) {
    string newUser(user);
    string newPass(pass);
    if (newUser[newUser.size() - 1] == '\r') newUser.erase(newUser.size() - 1, 1);
    if (newPass[newPass.size() - 1] == '\r') newPass.erase(newPass.size() - 1, 1);
    if (userRec.find(newUser) == userRec.end()) return false;
    if (userRec[newUser] == "" || userRec[newUser] == newPass) return true;
    return false;
}
