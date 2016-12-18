#include "instsocket.h"

extern GlobalSettings *globalSettings;
extern int errno;
int InstSocket::instFd = 0;
char *InstSocket::recvBuf = new char[BUFFER_SIZE];
char *InstSocket::textBuf = new char[BUFFER_SIZE];

int InstSocket::dataFd = -1;
int InstSocket::dataBaseFd = -1;
sockaddr_in *InstSocket::dataHostAddr, *InstSocket::dataServAddr;
bool InstSocket::isOpen = 0;
bool InstSocket::isListenOn = 0;
bool InstSocket::isPassive = 0;

InstSocket::InstSocket() {

}

int InstSocket::connectServer() {
    instFd = MSocket::msocket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in *mySockAddr = new sockaddr_in();
    mySockAddr->sin_addr.s_addr = INADDR_ANY;
    mySockAddr->sin_port = 0;
    int ret = MSocket::mbind(instFd, (sockaddr*)mySockAddr, sizeof(sockaddr_in));
    if (ret) {
        printf("Error: bind error\n");
        return ret;
    }
    sockaddr *serverAddr = globalSettings->servAddr;
    ret = MSocket::mconnect(instFd, serverAddr, sizeof(sockaddr));
    if (ret) {
        printf("Error: connect error ErrorNo:%d \n", errno);
        return ret;
    }
    MSocket::mrecv(instFd, recvBuf, BUFFER_SIZE, 0);
    int code = responseParse(recvBuf, textBuf, BUFFER_SIZE);
    if (code == CODE_220)
        return 0;
    else {
        printf("Illegal server response\n");
        return -1;
    }
}

int InstSocket::work(int instType, char *inst) {
    strcat(inst, "\r\n");
    int code;
    if ((instType != INST_PORT) && (instType != INST_STOR)) {
        if (MSocket::msend(instFd, inst, strlen(inst), 0) == 0) {
            printf("Error: send failed\n");
            return -1;
        }
        if (instType == INST_QUIT) {
            // no need response
            return 0;
        }
        if (MSocket::mrecv(instFd, recvBuf, BUFFER_SIZE, 0) < 0) {
            printf("Error: receive failed\n");
            return -1;
        }
        code = responseParse(recvBuf, textBuf, BUFFER_SIZE);
    }
    switch (instType) {
        case INST_USER: {
            if (code == CODE_331) {
                printf("User passed, continue\n");
                return 0;
            } else {
                printf("Wrong user\n");
                return -1;
            }
            break;
        }
        case INST_PASS: {
            if (code == CODE_230) {
                printf("Login succeed\n");
                return 0;
            } else {
                printf("Login failed\n");
                return -1;
            }
            break;
        }
        case INST_SYST: {
            if (code == CODE_215) {
                printf("%s\n", textBuf);
                return 0;
            } else {
                printf("SYST failed\n");
                return -1;
            }
            break;
        }
        case INST_PWD: {
            if (code == CODE_257) {
                printf("%s\n", textBuf);
                return 0;
            } else {
                printf("PWD failed\n");
                return -1;
            }
            break;
        }
        case INST_FEAT: {
            if (code == CODE_211) {
                if (strstr(recvBuf, "End FEAT.") == 0)
                    do {
                        MSocket::mrecv(instFd, recvBuf, BUFFER_SIZE, 0);
                        responseParse(recvBuf, textBuf, BUFFER_SIZE);
                        textBuf[strlen(textBuf)-1] = '\0';
                    } while (strstr(textBuf, "End FEAT.") == 0);
                return 0;
            }
            return -1;
            break;
        }
        case INST_CDUP: {
            if (code == CODE_200) {
                return 0;
            }
            return -1;
            break;
        }
        case INST_TYPE: {
           if (code == CODE_200) {
               return 0;
           }
           return -1;
           break;
        }
        case INST_PASV: {
            if (code == CODE_227) {
                int lp = -1, rp = -1, len = strlen(textBuf);
                for (int i = 0; i < len; ++i) {
                    if (textBuf[i] == '(') lp = i;
                    if (textBuf[i] == ')') rp = i;
                }
                if (lp > 0 && rp > 0) {
                    closeDataChannel();
                    stringstream stream;
                    for (int i = lp + 1; i < rp; ++i) stream << textBuf[i];
                    int p1, p2, p3, p4, p5, p6;
                    char tmpCh;
                    stream >> p1 >> tmpCh >> p2 >> tmpCh >> p3 >> tmpCh >> p4
                            >> tmpCh >> p5 >> tmpCh >> p6;
                    p1 &= 0xFF, p2 &= 0xFF, p3 &= 0xFF, p4 &= 0xFF;
                    p5 &= 0xFF, p6 &= 0xFF;
                    dataServAddr = new sockaddr_in();
                    dataServAddr->sin_family = AF_INET;
                    dataServAddr->sin_addr.s_addr
                            = (p4 << 24) + (p3 << 16) + (p2 << 8) + (p1);
                    dataServAddr->sin_port = (p5 << 8) + (p6);
                    printf("PASV connect port: %d\n", dataServAddr->sin_port);
                    dataServAddr->sin_port = htons(dataServAddr->sin_port);
                    memset(&(dataServAddr->sin_zero), 0, 8);

                    dataFd = MSocket::msocket(AF_INET, SOCK_STREAM, 0);
                    isPassive = 1;
                    dataHostAddr = new sockaddr_in();
                    dataHostAddr->sin_port = 0;
                    dataHostAddr->sin_addr.s_addr = INADDR_ANY;
                    if (MSocket::mbind(dataFd, (sockaddr*)dataHostAddr, sizeof(sockaddr_in))) {
                        printf("Bind data channel error");
                        return -1;
                    }
                    if (MSocket::mconnect(dataFd, (sockaddr*)dataServAddr, sizeof(sockaddr_in))) {
                        printf("Connect data channel error");
                        return -1;
                    }
                    isOpen = 1;
                    return 0;
                } else {
                    printf("Malformed PASV response\n");
                    return -1;
                }
            }
            return -1;
            break;
        }
        case INST_CWD: {
           if (code == CODE_250) {
               return 0;
           }
           return -1;
           break;
        }
        case INST_LIST: {
            if (!isOpen) {
                printf("Data channel haven't established\n");
                return -1;
            }
            if (code == CODE_125) {
                dataReceive();
                while ((strstr(textBuf, "226") == 0) && (code != 226)) {
                    MSocket::mrecv(instFd, recvBuf, BUFFER_SIZE, 0);
                    code = responseParse(recvBuf, textBuf, BUFFER_SIZE);
                }
                closeDataChannel();
                FILE *readF = fopen(TMP_FILE_NAME, "r");
                while (fgets(textBuf, BUFFER_SIZE, readF)) {
                    printf("%s\n", textBuf);
                }
                fclose(readF);
                return 0;
            }
            break;
        }
        case INST_PORT: {
            stringstream stream;
            int instLen = strlen(inst);
            for (int i = 5; i < instLen; ++i) {
                stream << inst[i];
            }
            int p1, p2, p3, p4, p5, p6;
            char tmpCh;
            stream >> p1 >> tmpCh >> p2 >> tmpCh >> p3 >> tmpCh
                    >> p4 >> tmpCh >> p5 >> tmpCh >> p6;
            p1 &= 0xFF, p2 &= 0xFF, p3 &= 0xFF,
                    p4 &= 0xFF, p5 &= 0xFF, p6 &= 0xFF;
            closeDataChannel();
            dataBaseFd = MSocket::msocket(AF_INET, SOCK_STREAM, 0);
            dataHostAddr = new sockaddr_in();
            dataHostAddr->sin_port = htons((p5 << 8) + p6);
            dataHostAddr->sin_addr.s_addr = INADDR_ANY;
            if (MSocket::mbind(dataBaseFd, (sockaddr*)dataHostAddr, sizeof(sockaddr_in))) {
                printf("Bind data channel error\n");
                return -1;
            }
            if (MSocket::mlisten(dataBaseFd, 0)) {
                printf("Listen data channel error\n");
                return -1;
            }

            if (MSocket::msend(instFd, inst, strlen(inst), 0) == 0) {
                printf("Error: send failed\n");
                return -1;
            }
            if (MSocket::mrecv(instFd, recvBuf, BUFFER_SIZE, 0) < 0) {
                printf("Error: receive failed\n");
                return -1;
            }
            code = responseParse(recvBuf, textBuf, BUFFER_SIZE);

            if (code == CODE_200) {
                isOpen = 1, isPassive = 0;
                return 0;
            }
            return -1;
            break;
        }
        case INST_MDTM: {
            if (code == CODE_213) {
                return 0;
            } else {
                return -1;
            }
            break;
        }
        case INST_SIZE: {
            if (code == CODE_213) {
                return 0;
            } else {
                return -1;
            }
            break;
        }
        case INST_RETR: {
            if (!isOpen) {
                printf("Data channel haven't established\n");
                return -1;
            }
            if (code == CODE_125) {
                dataReceive();
                while ((strstr(textBuf, "226") == 0) && (code != 226)) {
                    MSocket::mrecv(instFd, recvBuf, BUFFER_SIZE, 0);
                    code = responseParse(recvBuf, textBuf, BUFFER_SIZE);
                }
                closeDataChannel();

                stringstream stream;
                int instLen = strlen(inst);
                for (int i = 5; i < instLen; ++i) {
                    stream << inst[i];
                }
                string writeStrFile;
                stream >> writeStrFile;
                FILE *readF = fopen(TMP_FILE_NAME, "rb");
                FILE *writeF = fopen(writeStrFile.c_str(), "wb");
                int nowRead = 0;
                while ((nowRead = fread(recvBuf, 1, BUFFER_SIZE, readF)) > 0) {
                    fwrite(recvBuf, 1, nowRead, writeF);
                }
                fclose(readF);
                fclose(writeF);
                printf("Download finish!\n");

                return 0;
            } else return -1;
            break;
        }
        case INST_STOR: {
            if (!isOpen) {
                printf("Data channel haven't established\n");
                return -1;
            }

            string filePath = globalSettings->hostPath;
            if ((filePath.length() > 0) &&
                    (filePath[filePath.length() - 1] != '/') &&
                    (filePath[filePath.length() - 1] != '\\'))
                filePath += "/";
            int instLen = strlen(inst);
            if (inst[5] == '/' || inst[5] == '\\')
                filePath = "";
            for (int i=5; i<instLen; ++i)
                filePath += inst[i];
            filePath.erase(filePath.length() - 2);
            printf("Upload file local path: %s\n", filePath.c_str());
            FILE *rout = fopen(TMP_FILE_NAME, "wb");
            errno = 0;
            FILE *rin = fopen(filePath.c_str(), "rb");
            if ((rin == 0) || (rout == 0)) {
                printf("Error: illegal file Errno: %d\n", errno);
                return -1;
            }
            int readCnt, totCnt = 0;
            while ((readCnt = fread(recvBuf, 1, BUFFER_SIZE, rin)) > 0) {
                fwrite(recvBuf, 1, readCnt, rout);
                totCnt += readCnt;
            }
            fclose(rin);
            fclose(rout);
            printf("%d bytes copied\n", totCnt);

            if (MSocket::msend(instFd, inst, strlen(inst), 0) == 0) {
                printf("Error: send failed\n");
                return -1;
            }
            if (MSocket::mrecv(instFd, recvBuf, BUFFER_SIZE, 0) < 0) {
                printf("Error: receive failed\n");
                return -1;
            }

            code = responseParse(recvBuf, textBuf, BUFFER_SIZE);

            if (code == CODE_125) {
                dataSend();
                closeDataChannel();
                while ((strstr(textBuf, "226") == 0) && (code != 226)) {
                    MSocket::mrecv(instFd, recvBuf, BUFFER_SIZE, 0);
                    code = responseParse(recvBuf, textBuf, BUFFER_SIZE);
                }

                printf("Upload finish!\n");
                return 0;
            } else return -1;
            break;
        }
        case INST_MODE: {
            if (code == CODE_200) {
                return 0;
            } else {
                return -1;
            }
            break;
        }
        case INST_NOOP: {
            if (code == CODE_200) {
                return 0;
            } else {
                return -1;
            }
            break;
        }
        case INST_ACCT: {
            if (code == CODE_230) {
                return 0;
            } else {
                return -1;
            }
            break;
        }
        case INST_REIN: {
            if (code == CODE_220) {
                return 0;
            } else {
                return -1;
            }
        }
        case INST_ALLO: {
            if (code == CODE_200 || code == CODE_202) {
                return 0;
            } else {
                return -1;
            }
        }
        case INST_RNFR: {
            if (code == CODE_350) {
                return 0;
            } else {
                return -1;
            }
        }
        case INST_RNTO: {
            if (code == CODE_250) {
                return 0;
            } else {
                return -1;
            }
        }
        case INST_DELE: {
            if (code == CODE_250) {
                return 0;
            } else {
                return -1;
            }
        }
    }
    return -1;
}

int InstSocket::close() {
    int ret = MSocket::mclose(instFd);
    if (ret) {
        printf("Error: close error\n");
    }
    return ret;
}

int InstSocket::responseParse(const void *originData, char *text, int textLen) {
    stringstream s;
    s << (const char*)originData;
    int retCode = 0;
    s >> retCode;
    s.ignore(1);
    s.getline(text, textLen);
    printf("RECEIVE %d %s\n", retCode, text);
    return retCode;
}

void InstSocket::closeDataChannel() {
    if (isOpen) {
        if (isPassive) {
            MSocket::mclose(dataFd);
        } else {
            if (isListenOn)
                MSocket::mclose(dataFd);
            MSocket::mclose(dataBaseFd);
        }
    }
    isOpen = isListenOn = isPassive = 0;
    if (dataHostAddr) delete dataHostAddr;
    if (dataServAddr) delete dataServAddr;
    dataHostAddr = dataServAddr = 0;
}

int InstSocket::dataReceive() {
    if (!isOpen) return -1;
    FILE *tmpF = fopen(TMP_FILE_NAME, "wb");
    int recvByte = 0, totSize = 0;
    int sockAddrLen = sizeof(sockaddr_in);
    if (!isPassive) {
        dataServAddr = new sockaddr_in();
        dataFd = MSocket::maccept(dataBaseFd, (void*)dataServAddr, &sockAddrLen);
        isListenOn = true;
    }

    while ((recvByte = MSocket::mrecv(dataFd, recvBuf, BUFFER_SIZE, 0)) >= 0) {
        if (recvByte == 0) break;
        totSize += recvByte;
        fwrite(recvBuf, 1, recvByte, tmpF);
    }
    if (recvByte < 0)
        printf("recv error: %d %d\n", recvByte, errno);

    fclose(tmpF);
    return totSize;
}

int InstSocket::dataSend() {
    if (!isOpen) return -1;
    FILE *tmpF = fopen(TMP_FILE_NAME, "rb");
    int nowRead = 0, totSend = 0;
    int sockAddrLen = sizeof(sockaddr_in);
    if (!isPassive) {
        dataServAddr = new sockaddr_in();
        dataFd = MSocket::maccept(dataBaseFd, (void*)dataServAddr, &sockAddrLen);
        isListenOn = true;
    }

    while ((nowRead = fread(recvBuf, 1, BUFFER_SIZE, tmpF)) > 0) {
        MSocket::msend(dataFd, (void*)recvBuf, nowRead, 0);
        totSend += nowRead;
    }
    MSocket::msend(dataFd, (void*)recvBuf, 0, 0);

    if (nowRead < 0)
        printf("send error: %d %d\n", nowRead, errno);

    fclose(tmpF);
    return totSend;
}
