#include "handler.h"

extern GlobalSettings *globalSettings;

Handler::Handler(int _sockfd, sockaddr_in _clientAddr) {
    activeTime = time(0);
    sockfd = _sockfd;
    clientAddr = _clientAddr;
    status = CREATE;
    recvBuf = new char[BUFFER_SIZE];
    logined = false;
    nowUser = nowPass = "";
    pathStack = new vector<string>();
    pathStack->push_back("/");
    isOpen = isListenOn = isPassive = 0;
    dataFd = dataBaseFd = -1;
    dataHostAddr = dataClientAddr = 0;
    renameFrom = "";
}

Handler::~Handler() {
    delete[] recvBuf;
    delete pathStack;
    closeDataChannel();
}

void Handler::run(Handler *h) {
    printf("Handler started for client: IP: %s, port: %d\n", GlobalSettings::getIP(&(h->clientAddr)), h->clientAddr.sin_port);
    h->status = RUNNING;
    sendControlPacket(h->sockfd, 220, "Service ready");
    while (true) {
        if (h->sockfd == globalSettings->toBreakFd) {
            globalSettings->toBreakFd = -1;
            break;
        }
        int recvRet = MSocket::mrecv(h->sockfd, h->recvBuf, BUFFER_SIZE, 0);
        h->activeTime = time(0);
        if (recvRet < 0) {
            printf("%d: receive error\n", h->sockfd);
            continue;
        }
        if (recvRet == 0)
            break;
        stringstream stream;
        stream << h->recvBuf;
        string instType;
        stream >> instType;
        if (instType == INST_USER) {
            stream.ignore(1);
            stream.getline(h->recvBuf, BUFFER_SIZE);
            h->nowUser = string(h->recvBuf);
            if (globalSettings->userExist(h->nowUser.c_str())) {
                sendControlPacket(h->sockfd, 331, "Username ok, send password");
                h->logined = false;
            } else {
                sendControlPacket(h->sockfd, 530, "Illegal username");
            }
        } else
        if (instType == INST_PASS) {
            stream.ignore(1);
            stream.getline(h->recvBuf, BUFFER_SIZE);
            h->nowPass = string(h->recvBuf);
            if (globalSettings->identify(h->nowUser.c_str(), h->nowPass.c_str())) {
                sendControlPacket(h->sockfd, 230, "welcome!");
                h->logined = true;
            } else {
                sendControlPacket(h->sockfd, 530, "failed to login: malformed data from server");
            }
        } else
        if (instType == INST_SYST) {
            sendControlPacket(h->sockfd, 215, "UNIX Type: L8");
        } else
        if (instType == INST_PWD) {
            string path = "";
            for (int i=0; i < h->pathStack->size(); ++i)
                path += (*h->pathStack)[i];
            string res = "\"";
            res += path; res+= "\""; res += " is the current directory.";
            sendControlPacket(h->sockfd, 257, res.c_str());
        } else
        if (instType == INST_QUIT) {
            h->closeDataChannel();
            MSocket::mclose(h->sockfd);
            break;
        } else
        if (instType == INST_FEAT) {
            sendControlPacket(h->sockfd, 211, "Features supported:", true);

            sendControlPacket(h->sockfd, 211, "End FEAT.");
        } else
        if (instType == INST_CDUP) {
            if (!loginCheck(h)) continue;

            if (h->pathStack->size() > 1)
                h->pathStack->pop_back();
            string showName = "";
            for (int i = 0; i < h->pathStack->size(); ++i)
                showName += (*h->pathStack)[i];
            string res = "\"";
            res += showName;
            res += "\"";
            res += " is the current directory.";
            sendControlPacket(h->sockfd, 250, res.c_str());
        } else
        if (instType == INST_TYPE) {
            if (!loginCheck(h)) continue;

            stream.ignore(1);
            char ch;
            stream >> ch;
            if (ch == 'I') {
                sendControlPacket(h->sockfd, 200, "Type set to: binary.");
            } else {
                sendControlPacket(h->sockfd, 504, "Unsupported mode.");
            }
        } else
        if (instType == INST_PASV) {
            if (!loginCheck(h)) continue;

            if (h->isOpen)
                h->closeDataChannel();

            h->dataHostAddr = new sockaddr_in();
            h->dataHostAddr->sin_port = 0;
            h->dataHostAddr->sin_addr.s_addr = INADDR_ANY;
            h->dataBaseFd = MSocket::msocket(AF_INET, SOCK_STREAM, 0);
            if (MSocket::mbind(h->dataBaseFd, (sockaddr*)h->dataHostAddr, sizeof(sockaddr_in))) {
                sendControlPacket(h->sockfd, 502, "Bind data channel error");
                continue;
            }
            if (MSocket::mlisten(h->dataBaseFd, 0)) {
                sendControlPacket(h->sockfd, 502, "Listen data channel error");
                continue;
            }
            h->isOpen = true;
            h->isPassive = true;
            h->isListenOn = false;

            stringstream stream;
            stream << "Entering passive mode (";
            socklen_t sockaddrlen = sizeof(sockaddr);
            sockaddr_in *ipAddr = new sockaddr_in();
            getsockname(h->sockfd, (sockaddr*)ipAddr, &sockaddrlen);
            getsockname(h->dataBaseFd, (sockaddr*)h->dataHostAddr, &sockaddrlen);
            int p1 = (ipAddr->sin_addr.s_addr >> 24) & 0xFF;
            int p2 = (ipAddr->sin_addr.s_addr >> 16) & 0xFF;
            int p3 = (ipAddr->sin_addr.s_addr >> 8) & 0xFF;
            int p4 = (ipAddr->sin_addr.s_addr) & 0xFF;
            int p5 = (h->dataHostAddr->sin_port) & 0xFF;
            int p6 = (h->dataHostAddr->sin_port >> 8) & 0xFF;
            stream << p4 << "," << p3 << "," << p2 << "," << p1 << "," << p5 << "," << p6;
            stream << ")";
            delete ipAddr;
            printf("PASV data channel port: %d\n", (p5 << 8) + p6);
            sendControlPacket(h->sockfd, 227, stream.str().c_str());
        } else
        if (instType == INST_CWD) {
            if (!loginCheck(h)) continue;

            stream.ignore(1);
            stream.getline(h->recvBuf, BUFFER_SIZE);
            string path = string(h->recvBuf);
            if (path[path.length() - 1] == '\r')
                path.erase(path.length() - 1, 1);
            if (path[path.length() - 1] != '/' && path[path.length() - 1] != '\\')
                path += '/';

            vector<string> *tmpStack = new vector<string>();
            if (path[0] == '/' || path[0] == '\\') {} else {
                // relative path
                for(int i = 0; i < h->pathStack->size(); ++i)
                    tmpStack->push_back((*(h->pathStack))[i]);
            }
            bool legal = true;
            string nowTmp = "";
            for (int i = 0; i < path.length(); ++i) {
                if (path[i] == '/' || path[i] == '\\') {
                    if ((nowTmp == "") && (i)) {
                        legal = false;
                        break;
                    } else
                    if (nowTmp == "..") {
                        if (tmpStack->size() > 1)
                            tmpStack->pop_back();
                    } else {
                        tmpStack->push_back(nowTmp + "/");
                        nowTmp = "";
                    }
                } else {
                    nowTmp += path[i];
                }
            }
            string showName = "";
            if (legal) {
                for (int i = 0; i < tmpStack->size(); ++i)
                    showName += (*tmpStack)[i];
                struct stat *buf = new struct stat();
                string absoName = string(globalSettings->path);
                absoName += showName;
                if (stat(absoName.c_str(), buf) == 0) {
                    if (buf->st_mode & S_IFDIR) {}
                    else
                        legal = false;
                } else
                    legal = false;
                delete buf;
            }
            if (legal) {
                delete h->pathStack;
                h->pathStack = tmpStack;
                string res = "\"";
                res += showName;
                res += "\"";
                res += " is the current directory.";
                sendControlPacket(h->sockfd, 250, res.c_str());
            } else
                sendControlPacket(h->sockfd, 501, "malformed parameter");
        } else
        if (instType == INST_LIST) {
            if (!loginCheck(h)) continue;
            if (!h->isOpen) {
                sendControlPacket(h->sockfd, 500, "Data channel haven't been established");
                continue;
            }

            string listPath = "", append;
            for (int i=0; i < h->pathStack->size(); ++i)
                listPath += (*h->pathStack)[i];
            char tmpCh = stream.get();
            if (tmpCh != '\r') {
                stream >> append;
                printf("append: %s\n", append.c_str());
                if (append != "-a")
                    listPath += append;
            }
            printf("LIST path: %s\n", listPath.c_str());
            string ret = h->list(listPath);
            if (ret == "error") {
                sendControlPacket(h->sockfd, 501, "Illegal path.");
            } else {
                FILE *writeF = fopen(TMP_FILE_NAME, "w");
                fprintf(writeF, "%s\n", ret.c_str());
                fclose(writeF);
                sendControlPacket(h->sockfd, 125, "Data connection already open. Transfer starting.");
                h->dataSend();
                sendControlPacket(h->sockfd, 226, "Transfer complete.");
                h->closeDataChannel();
            }
        } else
        if (instType == INST_PORT) {
            if (!loginCheck(h)) continue;

            int p1, p2, p3, p4, p5, p6;
            char tmpCh;
            stream >> p1 >> tmpCh >> p2 >> tmpCh >> p3 >> tmpCh
                    >> p4 >> tmpCh >> p5 >> tmpCh >> p6;
            h->dataFd = MSocket::msocket(AF_INET, SOCK_STREAM, 0);
            h->dataHostAddr = new sockaddr_in();
            h->dataHostAddr->sin_addr.s_addr = INADDR_ANY;
            h->dataHostAddr->sin_port = htons(DATA_PORT);
            if (MSocket::mbind(h->dataFd, (sockaddr*)h->dataHostAddr, sizeof(sockaddr_in))) {
                sendControlPacket(h->sockfd, 502, "Bind data channel error");
                continue;
            }
            h->dataClientAddr = new sockaddr_in();
            h->dataClientAddr->sin_family = AF_INET;
            h->dataClientAddr->sin_addr.s_addr = (p1) + (p2 << 8) + (p3 << 16) + (p4 << 24);
            h->dataClientAddr->sin_port = htons((p5 << 8) + p6);
            memset(&(h->dataClientAddr->sin_zero), 0, 8);
            if (MSocket::mconnect(h->dataFd, (sockaddr*)h->dataClientAddr, sizeof(sockaddr))) {
                sendControlPacket(h->sockfd, 502, "Connect data channel error");
                continue;
            }
            printf("PORT data channel: %d:%d:%d:%d::%d\n", p1, p2, p3, p4, ((p5 << 8) + p6));
            h->isOpen = true;
            h->isPassive = false;
            sendControlPacket(h->sockfd, 200, "Data link established");
        } else
        if (instType == INST_MDTM) {
            stream.ignore(1);
            stream.getline(h->recvBuf, BUFFER_SIZE);
            string fileName = string(h->recvBuf);
            if (fileName[fileName.length() - 1] == '\r')
                fileName.erase(fileName.length() - 1, 1);
            string absoPath = globalSettings->path;
            absoPath += fileName;
            struct stat *nowStat = new struct stat();
            if (stat(absoPath.c_str(), nowStat) == 0) {
                struct tm *tmptr = localtime(&nowStat->st_mtim.tv_sec);
                char *timeS = new char[100];
                strftime(timeS, 100, "%Y%m%d%H%M%S", tmptr);
                sendControlPacket(h->sockfd, 213, timeS);
                delete[] timeS;
            } else
                sendControlPacket(h->sockfd, 501, "Non-exist file");
            delete nowStat;
        } else
        if (instType == INST_SIZE) {
            stream.ignore(1);
            stream.getline(h->recvBuf, BUFFER_SIZE);
            string fileName = string(h->recvBuf);
            if (fileName[fileName.length() - 1] == '\r')
                fileName.erase(fileName.length() - 1, 1);
            string absoPath = globalSettings->path;
            absoPath += fileName;
            struct stat *nowStat = new struct stat();
            if (stat(absoPath.c_str(), nowStat) == 0) {
                char *sizeS = new char[100];
                sprintf(sizeS, "%d", nowStat->st_size);
                sendControlPacket(h->sockfd, 213, sizeS);
                delete[] sizeS;
            } else
                sendControlPacket(h->sockfd, 501, "Non-exist file");
            delete nowStat;
        } else
        if (instType == INST_RETR) {
            if (!loginCheck(h)) continue;
            if (!h->isOpen) {
                sendControlPacket(h->sockfd, 500, "Data channel haven't been established");
                continue;
            }

            stream.ignore(1);
            stream.getline(h->recvBuf, BUFFER_SIZE);
            string fileName = string(h->recvBuf);
            if (fileName[fileName.length() - 1] == '\r')
                fileName.erase(fileName.length() - 1, 1);
            string absoPath = globalSettings->path;
            if (fileName[0] != '/' && fileName[0] != '\\')
                for (int i=0; i < h->pathStack->size(); ++i)
                    absoPath += (*h->pathStack)[i];
            absoPath += fileName;

            string ret = "";
            FILE* fromFile;
            if (fromFile = fopen(absoPath.c_str(), "rb")) {
                FILE *toFile = fopen(TMP_FILE_NAME, "wb");
                if (toFile) {
                    int readCnt = 0;
                    while ((readCnt = fread(h->recvBuf, 1, BUFFER_SIZE, fromFile)) > 0) {
                        printf("copy %d bytes\n", readCnt);
                        fwrite(h->recvBuf, 1, readCnt, toFile);
                    }
                    fclose(toFile);
                }
                fclose(fromFile);
            } else
                ret = "error";

            if (ret == "error") {
                sendControlPacket(h->sockfd, 501, "Illegal path.");
            } else {
                sendControlPacket(h->sockfd, 125, "Data connection already open. Transfer starting.");
                h->dataSend();
                h->closeDataChannel();
                sendControlPacket(h->sockfd, 226, "Transfer complete.");
            }
        } else
        if ((instType == INST_STOR) || (instType == INST_APPE)) {
            if (!loginCheck(h)) continue;
            if (!h->isOpen) {
                sendControlPacket(h->sockfd, 500, "Data channel haven't been established");
                continue;
            }

            stream.ignore(1);
            stream.getline(h->recvBuf, BUFFER_SIZE);
            string rawName = string(h->recvBuf);
            if (rawName[rawName.length() - 1] == '\r')
                rawName.erase(rawName.length() - 1, 1);
            int leafP = 0;
            for (int i=rawName.length() - 1; i>=0; --i)
                if (rawName[i] == '/' || rawName[i] == '\\') {
                    leafP = i + 1;
                    break;
                }
            if (leafP == rawName.length()) {
                sendControlPacket(h->sockfd, 501, "Wrong file name");
                continue;
            }
            string absoName = globalSettings->path;
            if (rawName[leafP] != '/' && rawName[leafP] != '\\')
                for (int i=0; i < h->pathStack->size(); ++i)
                    absoName += (*h->pathStack)[i];
            absoName += rawName.substr(leafP);
            printf("Upload dest file path: %s\n", absoName.c_str());

            FILE *destFile = fopen(absoName.c_str(), "wb+");
            if (destFile == 0) {
                sendControlPacket(h->sockfd, 501, "Wrong file name");
            }
            fclose(destFile);

            sendControlPacket(h->sockfd, 125, "Data connection already open. Transfer starting.");
            h->dataReceive();

            if (instType == INST_STOR)
                destFile = fopen(absoName.c_str(), "wb");
            else
                destFile = fopen(absoName.c_str(), "wb+");
            FILE *fromFile = fopen(TMP_FILE_NAME, "rb");
            int tmpCnt = 0;
            while ((tmpCnt = fread(h->recvBuf, 1, BUFFER_SIZE, fromFile)) > 0) {
                fwrite(h->recvBuf, 1, tmpCnt, destFile);
            }
            fclose(destFile);
            fclose(fromFile);

            h->closeDataChannel();
            sendControlPacket(h->sockfd, 226, "Transfer complete.");
        } else
        if (instType == INST_MODE) {
            if (!loginCheck(h)) continue;
            if (!h->isOpen) {
                sendControlPacket(h->sockfd, 500, "Data channel haven't been established");
                continue;
            }

            char chTmp;
            stream >> chTmp;
            if (chTmp == 'S') {
                sendControlPacket(h->sockfd, 200, "Ok.");
            } else {
                sendControlPacket(h->sockfd, 501, "Unsupported mode.");
            }
        } else
        if (instType == INST_NOOP) {
            sendControlPacket(h->sockfd, 200, "Ok.");
        } else
        if (instType == INST_ACCT) {
            stream.ignore(1);
            stream.getline(h->recvBuf, BUFFER_SIZE);
            h->nowUser = string(h->recvBuf);
            if (globalSettings->identify(h->nowUser.c_str(), "")) {
                h->logined = true;
                sendControlPacket(h->sockfd, 230, "Welcome!");
            } else {
                sendControlPacket(h->sockfd, 530, "failed to login: malformed data from server");
            }
        } else
        if (instType == INST_REIN) {
            h->closeDataChannel();
            h->logined = false;
            sendControlPacket(h->sockfd, 220, "ready for new user");
        } else
        if (instType == INST_ALLO) {
            sendControlPacket(h->sockfd, 202, "no buffer needed");
        } else
        if (instType == INST_RNFR) {
            if (!loginCheck(h)) continue;

            stream.ignore(1);
            stream.getline(h->recvBuf, BUFFER_SIZE);
            string rawName = string(h->recvBuf);
            if (rawName[rawName.length() - 1] == '\r')
                rawName.erase(rawName.length() - 1, 1);
            string absoName = globalSettings->path;
            for (int i=0; i < h->pathStack->size(); ++i)
                absoName += (*h->pathStack)[i];
            absoName += rawName;
            printf("Rename from file path: %s\n", absoName.c_str());
            h->renameFrom = absoName;
            FILE *testF = fopen(absoName.c_str(), "rb");
            if (testF == 0) {
                sendControlPacket(h->sockfd, 550, "no such file");
            } else {
                sendControlPacket(h->sockfd, 350, "need for info for the operation");
            }
            if (testF) fclose(testF);
        } else
        if (instType == INST_RNTO) {
            if (!loginCheck(h, 532)) continue;

            stream.ignore(1);
            stream.getline(h->recvBuf, BUFFER_SIZE);
            string rawName = string(h->recvBuf);
            if (rawName[rawName.length() - 1] == '\r')
                rawName.erase(rawName.length() - 1, 1);
            string absoName = globalSettings->path;
            for (int i=0; i < h->pathStack->size(); ++i)
                absoName += (*h->pathStack)[i];
            absoName += rawName;
            printf("Rename to file path: %s\n", absoName.c_str());

            if ((h->renameFrom == "") || (rename(h->renameFrom.c_str(), absoName.c_str()))) {
                sendControlPacket(h->sockfd, 501, "bad argument for rename");
            } else {
                h->renameFrom = "";
                sendControlPacket(h->sockfd, 250, "rename success");
            }
        } else
        if (instType == INST_DELE) {
            if (!loginCheck(h, 532)) continue;

            stream.ignore(1);
            stream.getline(h->recvBuf, BUFFER_SIZE);
            string rawName = string(h->recvBuf);
            if (rawName[rawName.length() - 1] == '\r')
                rawName.erase(rawName.length() - 1, 1);
            string absoName = globalSettings->path;
            if (rawName[0] != '/' && rawName[0] != '\\')
                for (int i=0; i < h->pathStack->size(); ++i)
                    absoName += (*h->pathStack)[i];
            absoName += rawName;
            printf("delete file path: %s\n", absoName.c_str());

            if (remove(absoName.c_str()) == 0) {
                sendControlPacket(h->sockfd, 250, "delete success");
            } else {
                sendControlPacket(h->sockfd, 501, "bad argument for delete");
            }
        } else
        if (instType == INST_RMD) {
            if (!loginCheck(h, 532)) continue;

            stream.ignore(1);
            stream.getline(h->recvBuf, BUFFER_SIZE);
            string rawName = string(h->recvBuf);
            if (rawName[rawName.length() - 1] == '\r')
                rawName.erase(rawName.length() - 1, 1);
            string absoName = globalSettings->path;
            if (rawName[0] != '/' && rawName[0] != '\\')
                for (int i=0; i < h->pathStack->size(); ++i)
                    absoName += (*h->pathStack)[i];
            absoName += rawName;
            printf("delete directory path: %s\n", absoName.c_str());

            if (rmdir(absoName.c_str()) == 0) {
                sendControlPacket(h->sockfd, 250, "delete success");
            } else {
                sendControlPacket(h->sockfd, 501, "bad argument for delete");
            }
        } else
        if (instType == INST_MKD) {
            if (!loginCheck(h, 532)) continue;

            stream.ignore(1);
            stream.getline(h->recvBuf, BUFFER_SIZE);
            string rawName = string(h->recvBuf);
            if (rawName[rawName.length() - 1] == '\r')
                rawName.erase(rawName.length() - 1, 1);
            string absoName = globalSettings->path;
            if (rawName[0] != '/' && rawName[0] != '\\')
                for (int i=0; i < h->pathStack->size(); ++i)
                    absoName += (*h->pathStack)[i];
            absoName += rawName;
            printf("make directory path: %s\n", absoName.c_str());

            if (mkdir(absoName.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH) == 0) {
                sendControlPacket(h->sockfd, 257, "make directory success");
            } else {
                sendControlPacket(h->sockfd, 501, "bad argument for delete");
            }
        } else
        if (instType == INST_HELP) {
            string defShow;
            defShow = "A simple & naive FTP server written by llylly | ";
            defShow += "Supported instructions: | ";
            defShow += "USER PASS SYST PWD QUIT FEAT CDUP TYPE | ";
            defShow += "PASV CWD LIST PORT SIZE RETR STOR MODE NOOP | ";
            defShow += "ACCT REIN ALLO RNFR RNTO DELE RMD MKD | ";
            defShow += "HELP STRU APPE";
            sendControlPacket(h->sockfd, 211, defShow.c_str());
        } else
        if (instType == INST_STRU) {
            if (!loginCheck(h)) continue;

            char ch;
            stream >> ch;
            if (ch == 'F') {
                sendControlPacket(h->sockfd, 200, "Use file structure");
            } else {
                sendControlPacket(h->sockfd, 501, "Unsupported structure");
            }
        } else {
            sendControlPacket(h->sockfd, 500, "Unknown intruction");
        }
    }
    printf("1 session finished\n");
    h->status = FINISH;
}

bool Handler::loginCheck(const Handler *h, int code) {
    if (!h->logined)
        sendControlPacket(h->sockfd, code, "Please login first.");
    return h->logined;
}

int Handler::sendControlPacket(int sockfd, int code, const char *msg, bool hafen) {
    stringstream stream;
    if (hafen)
        stream << code << "-" << msg << "\r\n";
    else
        stream << code << " " << msg << "\r\n";
    const char *data = stream.str().c_str();
    int length = stream.str().length();
    printf("SEND %s", data);
    return MSocket::msend(sockfd, data, length, 0);
}

void Handler::closeDataChannel() {
    if (isOpen) {
        if (!isPassive) {
            MSocket::mclose(dataFd);
        } else {
            if (isListenOn)
                MSocket::mclose(dataFd);
            MSocket::mclose(dataBaseFd);
        }
    }
    isOpen = isListenOn = isPassive = 0;
    if (dataHostAddr) delete dataHostAddr;
    if (dataClientAddr) delete dataClientAddr;
    dataHostAddr = dataClientAddr = 0;
}

int Handler::dataReceive() {
    if (!isOpen) return -1;
    FILE *tmpF = fopen(TMP_FILE_NAME, "wb");
    int recvByte = 0, totSize = 0;
    int sockAddrLen = sizeof(sockaddr_in);
    if (isPassive) {
        dataClientAddr = new sockaddr_in();
        dataFd = MSocket::maccept(dataBaseFd, (void*)dataClientAddr, &sockAddrLen);
        isListenOn = true;
    }

    while ((recvByte = MSocket::mrecv(dataFd, recvBuf, BUFFER_SIZE, 0)) >= 0) {
        if (recvByte == 0) break;
        totSize += recvByte;
        fwrite(recvBuf, 1, recvByte, tmpF);
    }
    if (recvByte < 0)
        printf("recv error: %d\n", recvByte);

    printf("%d bytes(s) received\n", totSize);

    fclose(tmpF);
    return totSize;
}

int Handler::dataSend() {
    if (!isOpen) return -1;
    FILE *tmpF = fopen(TMP_FILE_NAME, "rb");
    int nowRead = 0, totSend = 0;
    int sockAddrLen = sizeof(sockaddr_in);
    if (isPassive) {
        dataClientAddr = new sockaddr_in();
        dataFd = MSocket::maccept(dataBaseFd, (void*)dataClientAddr, &sockAddrLen);
        isListenOn = true;
    }

    while ((nowRead = fread(recvBuf, 1, BUFFER_SIZE, tmpF)) > 0) {
        int tt;
        if ((tt = MSocket::msend(dataFd, (void*)recvBuf, nowRead, 0)) < 0) {
            printf("send error: %d %d\n", tt, errno);
            break;
        }
        totSend += nowRead;
    }
    MSocket::msend(dataFd, (void*)recvBuf, 0, 0);

    printf("%d byte(s) sent\n", totSend);

    fclose(tmpF);
    return totSend;
}

string Handler::list(string path) {
    stringstream stream;
    path = globalSettings->path + path;
    DIR *dir = opendir(path.c_str());
    if (dir == 0) return "error";
    direct *ptr;
    while (ptr = readdir(dir)) {
        struct stat *nowStat = new struct stat();
        char *timeS = new char[100];
        struct tm *tmptr;
        if (stat((path + ptr->d_name).c_str(), nowStat) == 0) {
            if (nowStat->st_mode & S_IFDIR)
                stream << "d";
            else
                stream << "-";
            if (nowStat->st_mode & S_IRUSR)
                stream << "r";
            else
                stream << "-";
            if (nowStat->st_mode & S_IWUSR)
                stream << "w";
            else
                stream << "-";
            if (nowStat->st_mode & S_IXUSR)
                stream << "x";
            else
                stream << "-";
            if (nowStat->st_mode & S_IRGRP)
                stream << "r";
            else
                stream << "-";
            if (nowStat->st_mode & S_IWGRP)
                stream << "w";
            else
                stream << "-";
            if (nowStat->st_mode & S_IXGRP)
                stream << "x";
            else
                stream << "-";
            if (nowStat->st_mode & S_IROTH)
                stream << "r";
            else
                stream << "-";
            if (nowStat->st_mode & S_IWOTH)
                stream << "w";
            else
                stream << "-";
            if (nowStat->st_mode & S_IXOTH)
                stream << "x";
            else
                stream << "-";
            stream << "";
            stream << std::setw(4) << nowStat->st_nlink;
            stream << "  ftp   ftp";
            stream << "   ";
            stream << nowStat->st_size << "  ";
            tmptr = localtime(&nowStat->st_mtim.tv_sec);
            strftime(timeS, 100, "%b %d  %Y", tmptr);
            stream << timeS << " ";
            stream << ptr->d_name;
            stream << endl;
        }
        delete nowStat;
        delete timeS;
        //stream << ptr->d_name << endl;
    }
    closedir(dir);
    return stream.str();
}
