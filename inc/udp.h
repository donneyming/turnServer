#ifndef _UDP_H
#define _UDP_H
#pragma comment(lib,"ws2_32.lib")
#include "winsock2.h"
#include "types.h"

typedef SOCKET Socket;

U32 GetLocalIp(void);
void exitNetwork(void);
int initNetwork(void);
int  sendMessage( Socket fd, char* buf, int len, unsigned int dstIp, unsigned short dstPort );
int getMessage( Socket fd, char *buf, int len, unsigned int *srcIp, unsigned short *srcPort,int outlen);
Socket openPort( unsigned short port);
int  getErrno(void);


#endif
