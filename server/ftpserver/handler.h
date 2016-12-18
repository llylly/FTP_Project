#ifndef HANDLER_H
#define HANDLER_H

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <dirent.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include "globalsettings.h"
#include "msocket.h"

#define DATA_PORT 20

#define CREATE 0
#define RUNNING 1
#define FINISH 2

#define BUFFER_SIZE 1048576

#define INST_USER "USER"
#define INST_PASS "PASS"
#define INST_SYST "SYST"
#define INST_PWD "PWD"
#define INST_QUIT "QUIT"
#define INST_FEAT "FEAT"
#define INST_CDUP "CDUP"
#define INST_TYPE "TYPE"
#define INST_PASV "PASV"
#define INST_CWD "CWD"
#define INST_LIST "LIST"
#define INST_PORT "PORT"
#define INST_MDTM "MDTM"
#define INST_SIZE "SIZE"
#define INST_RETR "RETR"
#define INST_STOR "STOR"
#define INST_MODE "MODE"
#define INST_NOOP "NOOP"
#define INST_ACCT "ACCT"
#define INST_REIN "REIN"
#define INST_ALLO "ALLO"
#define INST_RNFR "RNFR"
#define INST_RNTO "RNTO"
#define INST_DELE "DELE"
#define INST_RMD "RMD"
#define INST_MKD "MKD"
#define INST_HELP "HELP"
#define INST_STRU "STRU"
#define INST_APPE "APPE"


#define TMP_FILE_NAME "tmp.bin"

class Handler
{
public:
    Handler(int _sockfd, sockaddr_in _clientAddr);
    ~Handler();

    static void run(Handler *h);

    long activeTime;
    int sockfd;
    sockaddr_in clientAddr;
    int status;
    char *recvBuf;

    bool logined;
    string nowUser, nowPass;
    vector<string> *pathStack;

    bool isOpen, isListenOn, isPassive;
    int dataFd, dataBaseFd;
    sockaddr_in *dataHostAddr, *dataClientAddr;

    string renameFrom;

private:
    static bool loginCheck(const Handler*, int code = 530);
    static int sendControlPacket(int sockfd, int code, const char *msg, bool hafen = false);
    void closeDataChannel();
    int dataReceive();
    int dataSend();

    string list(string path);
};

#endif // HANDLER_H
