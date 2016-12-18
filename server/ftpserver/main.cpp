/**
  By llylly
  2016.12.16
  FTP server main
*/

#include <cstdio>
#include <cstring>
#include <csignal>
#include <thread>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include "globalsettings.h"
#include "mainsocket.h"

#define DEFAULT_PORT 21

typedef unsigned short ushort;

using namespace std;

GlobalSettings *globalSettings = new GlobalSettings();

void initParams(int argc, char **argv);
void printHelp();
void unConnectHandle(int signo);

int main(int argc, char** argv) {
    signal(SIGPIPE, unConnectHandle);
    initParams(argc, argv);
    MainSocket::init();
    return 0;
}

void initParams(int argc, char **argv) {
    globalSettings->path = "";
    globalSettings->port = DEFAULT_PORT;
    int stat = 0;
    for (int i = 1; i < argc; ++i) {
        if (stat == 0) {
            if (strcmp(argv[i], "-path") == 0)
                stat = 1;
            if (strcmp(argv[i], "-port") == 0)
                stat = 2;
            if (strcmp(argv[i], "-help") == 0)
                printHelp();
        } else
        if (stat == 1) {
            // path of file system
            globalSettings->path = argv[i];
            stat = 0;
        } else
        if (stat == 2) {
            // port
            int t = atoi(argv[i]);
            if (t)
                globalSettings->port = (ushort)t;
            stat = 0;
        }
    }
    printf("File system path: %s\n", globalSettings->path);
    printf("Controlling channel port: %d\n", globalSettings->port);
}

void printHelp() {
    printf("Usage: \n");
    printf("-path [path] : specify root of file system to share \n");
    printf("-port [port] : specify port of server for control channel \n");
    printf("-help : print help info \n");
    printf("-----\n");
    printf("Mini FTP server written by llylly \n");
    printf("User info is read from users.txt \n");
}

void unConnectHandle(int signo) {
    printf("A connect closed exceptionally\n");
    printf("It will close immediately\n");
    if (signo == SIGPIPE)
        globalSettings->toBreakFd = globalSettings->lastSendFd;
}
