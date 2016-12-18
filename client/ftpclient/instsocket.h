#ifndef INSTSOCKET_H
#define INSTSOCKET_H

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <sstream>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include "msocket.h"
#include "globalsettings.h"

#define CODE_125 125
#define CODE_200 200
#define CODE_202 202
#define CODE_211 211
#define CODE_213 213
#define CODE_215 215
#define CODE_220 220
#define CODE_227 227
#define CODE_230 230
#define CODE_250 250
#define CODE_257 257
#define CODE_331 331
#define CODE_350 350

#define BUFFER_SIZE 1048576

#define INST_USER 0
#define INST_PASS 1
#define INST_SYST 2
#define INST_PWD 3
#define INST_QUIT 4
#define INST_FEAT 5
#define INST_CDUP 6
#define INST_TYPE 7
#define INST_PASV 8
#define INST_CWD 9
#define INST_LIST 10
#define INST_PORT 11
#define INST_MDTM 12
#define INST_SIZE 13
#define INST_RETR 14
#define INST_STOR 15
#define INST_MODE 16
#define INST_NOOP 17
#define INST_ACCT 18
#define INST_REIN 19
#define INST_ALLO 20
#define INST_RNFR 21
#define INST_RNTO 22
#define INST_DELE 23
#define INST_RMD 24
#define INST_MKD 25
#define INST_HELP 26
#define INST_STRU 27
#define INST_APPE 28

#define TMP_FILE_NAME "tmp.bin"

using namespace std;

class InstSocket
{
public:
    InstSocket();

    static int connectServer();
    static int work(int instType, char *inst);
    static int close();

    static int responseParse(const void *originData, char *text, int textLen);

    static void closeDataChannel();

private:
    static int instFd;
    static char *recvBuf, *textBuf;

    static int dataFd, dataBaseFd;
    static sockaddr_in *dataHostAddr, *dataServAddr;
    static bool isOpen, isListenOn, isPassive;

    static int dataReceive();

    static int dataSend();

};

#endif // INSTSOCKET_H
