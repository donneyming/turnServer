#include ".\inc\udp.h"

int  getErrno(void)
{
	return WSAGetLastError();
}

Socket openPort( unsigned short port)
{
	 Socket fd;
	 struct sockaddr_in addr;

	 fd = socket(AF_INET,SOCK_DGRAM,0);
	 if (fd == INVALID_SOCKET)
	 {
		printf("open port error in socket");
		return TURNFAIL;
	 }
		
	 memset(&addr,0,sizeof(addr));
	 addr.sin_addr.s_addr = htonl(INADDR_ANY); //htonl(INADDR_ANY); yfs
	 addr.sin_family = AF_INET;
	 addr.sin_port = htons(port);

	 if (bind(fd,(struct sockaddr*)(&addr),sizeof(addr)) != 0)
	 {
		printf("bind port error in bind");
		return TURNFAIL;
	 }
	 return fd;
}


int getMessage( Socket fd, char *buf, int len, unsigned int *srcIp, unsigned short *srcPort,int outlen)
{
	 int originalSize =0;
	 struct sockaddr_in from;

	 int fromlen = sizeof(from);

	 memset(&from,0,fromlen);
	 originalSize = recvfrom(fd,
							 buf,
							 len,
							 0,
							 (struct sockaddr*) (&from),
							 &fromlen);
	 if (originalSize <= 0)
	 {
		 printf("getmessage error%d\n",getErrno());
		 return TURNFAIL;
	 }

	 *srcIp = from.sin_addr.s_addr;
	 *srcPort = htons(from.sin_port);

	 return TURNSUCCEED;
}


int  sendMessage( Socket fd, char* buf, int len, unsigned int dstIp, unsigned short dstPort )
{
	int sendlen;
	struct sockaddr_in  addr;

	int tolen = sizeof(addr);

	memset(&addr,0,tolen);
	addr.sin_addr.s_addr = dstIp;  //yfs
	addr.sin_port = ntohs(dstPort);
	addr.sin_family = AF_INET;

	printf("sending to IP=%s,PORT=%d",inet_ntoa(addr.sin_addr),dstPort);

	sendlen = sendto(fd,buf,len,0,(struct sockaddr*)(&addr),tolen);
	if (sendlen == SOCKET_ERROR)
	{
		printf("Error:%d",getErrno());
		return TURNFAIL;
	}

	return TURNSUCCEED;
}


int initNetwork(void)
{
	 WSADATA wsaData;

	 int error = WSAStartup(MAKEWORD(2,2),&wsaData);
	 if (error != 0)
	 {
		 printf("windows tcp/ip init fail!!\n");
		 return TURNFAIL;
	 }	

	 if ( LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 ) 
	 {
		printf("windows tcp/ip version error!!\n");
		WSACleanup();
		 return TURNFAIL;
	 }
	  return TURNSUCCEED;
}

void exitNetwork(void)
{
	WSACleanup();
}

U32 GetLocalIp(void)
{
	char localname[80];
	struct hostent *htnt;
	
	U32 localIp = 0;

	memset(localname,80,0);
	if (gethostname(localname,80) == SOCKET_ERROR)
	{
		return TURNFAIL;
	}

	if (!(htnt = gethostbyname(localname)))
	{
		return TURNFAIL;
	}
	
	localIp = (*(U32 *)(htnt->h_addr_list[0]));//获取本机地址 yfs

	return localIp;
}