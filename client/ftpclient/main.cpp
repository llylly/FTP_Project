/**
  By llylly
  2016.12.16
  FTP client main
*/

#include <cstdio>
#include <cstring>
#include <csignal>
#include <thread>
#include <iostream>
#include <sstream>
#include "globalsettings.h"
#include "instsocket.h"

#define LOCAL_HOST "127.0.0.1"
#define DEFAULT_PORT 21

typedef unsigned short ushort;

using namespace std;

GlobalSettings *globalSettings = new GlobalSettings();

void initParam(int, char**);
void printHelp();
void unConnectHandle(int signo);

int main(int argc, char **argv) {
    signal(SIGPIPE, unConnectHandle);
    initParam(argc, argv);
    int ret = InstSocket::connectServer();
    if (ret == 0) {
        char *userIn = new char[256];
        while (true) {
            printf("> ");
            cin.getline(userIn, 256);
            stringstream stream;
            stream << userIn;
            string instType;
            stream >> instType;
            if (instType == "USER") {
                InstSocket::work(INST_USER, userIn);
            } else
            if (instType == "PASS") {
                InstSocket::work(INST_PASS, userIn);
            } else
            if (instType == "SYST") {
                InstSocket::work(INST_SYST, userIn);
            } else
            if (instType == "PWD") {
                InstSocket::work(INST_PWD, userIn);
            } else
            if (instType == "FEAT") {
                InstSocket::work(INST_FEAT, userIn);
            } else
            if (instType == "CDUP") {
                InstSocket::work(INST_CDUP, userIn);
            } else
            if (instType == "TYPE") {
                InstSocket::work(INST_TYPE, userIn);
            } else
            if (instType == "PASV") {
               InstSocket::work(INST_PASV, userIn);
            } else
            if (instType == "CWD") {
                InstSocket::work(INST_CWD, userIn);
            } else
            if (instType == "LIST") {
                InstSocket::work(INST_LIST, userIn);
            } else
            if (instType == "PORT") {
                InstSocket::work(INST_PORT, userIn);
            } else
            if (instType == "MDTM") {
                InstSocket::work(INST_MDTM, userIn);
            } else
            if (instType == "SIZE") {
                InstSocket::work(INST_SIZE, userIn);
            } else
            if (instType == "RETR") {
                InstSocket::work(INST_RETR, userIn);
            } else
            if (instType == "STOR") {
                InstSocket::work(INST_STOR, userIn);
            } else
            if (instType == "MODE") {
                InstSocket::work(INST_MODE, userIn);
            } else
            if (instType == "NOOP") {
                InstSocket::work(INST_NOOP, userIn);
            } else
            if (instType == "ACCT") {
                InstSocket::work(INST_ACCT, userIn);
            } else
            if (instType == "REIN") {
                InstSocket::work(INST_REIN, userIn);
            } else
            if (instType == "ALLO") {
                InstSocket::work(INST_ALLO, userIn);
            } else
            if (instType == "RNFR") {
                InstSocket::work(INST_RNFR, userIn);
            } else
            if (instType == "RNTO") {
                InstSocket::work(INST_RNTO, userIn);
            } else
            if (instType == "DELE") {
                InstSocket::work(INST_DELE, userIn);
            } else
            if (instType == "RMD") {
                InstSocket::work(INST_RMD, userIn);
            } else
            if (instType == "MKD") {
                InstSocket::work(INST_MKD, userIn);
            } else
            if (instType == "HELP") {
                InstSocket::work(INST_HELP, userIn);
            } else
            if (instType == "STRU") {
                InstSocket::work(INST_STRU, userIn);
            } else
            if (instType == "APPE") {
                InstSocket::work(INST_APPE, userIn);
            } else
            if (instType == "QUIT") {
                InstSocket::work(INST_QUIT, userIn);
                InstSocket::closeDataChannel();
                InstSocket::close();
                break;
            }
        }
    }

    return 0;
}

void initParam(int argc, char **argv) {
    char *servAddr = LOCAL_HOST;
    ushort servPort = DEFAULT_PORT;
    globalSettings->hostPath = "";
    int stat = 0;
    for (int i=1; i < argc; ++i) {
        if (stat == 0) {
            if (strcmp(argv[i], "-host") == 0) {
                stat = 1;
            }
            if (strcmp(argv[i], "-port") == 0) {
                stat = 2;
            }
            if (strcmp(argv[i], "-path") == 0) {
                stat = 3;
            }
            if (strcmp(argv[i], "-help") == 0) {
                printHelp();
            }
        } else
        if (stat == 1) {
            // set host
            servAddr = argv[i];
            stat = 0;
        } else
        if (stat == 2) {
            // set port
            int t = atoi(argv[i]);
            if (t) servPort = (short)t;
            stat = 0;
        } else
        if (stat == 3) {
            // set root path for upload
            globalSettings->hostPath = string(argv[i]);
            stat = 0;
        }
    }
    globalSettings->servAddr = globalSettings->genSockAddr(servAddr, servPort);
    printf("Setting host to %s:%d\n", servAddr, servPort);
}

void printHelp() {
    printf("Usage: \n");
    printf("-host [IP address] : specify IP of server(default: localhost) \n");
    printf("-port [port] : specify port of server for control channel \n");
    printf("-path [path] : set prefix for specifying uploading files \n");
    printf("-help : print help info \n");
    printf("-----\n");
    printf("Mini FTP client written by llylly \n");
}

void unConnectHandle(int signo) {
    printf("The connect has been shutdown\n");
    printf("----FTP client may exit----\n");
    exit(0);
}
