#include ".\inc\turn.h"
#include "inc\udp.h"
#define _WIN32_WINDOWS 0x0400
#include "windows.h"

struct Serverinf server;

DWORD WINAPI TimerProcess(LPVOID lpParameter)
{
	while(!server.isquit)
	{
		ListCheckTimeout(&(server.rtplist));
		ListCheckTimeout(&(server.rtcplist));
		Sleep(1000);
	}
	return 0;
}

DWORD WINAPI ListenFromRTP(LPVOID lpParameter)//用来分配RTP、TURN的端口
{
	int			ret =0;
	struct timeval tv;
	SessionNode *node;

	tv.tv_sec  = 2;
	tv.tv_usec = 0;

	while (!server.isquit)
	{
		FD_ZERO(&server.rtplist.fds);
		FD_SET(server.rtplist.skfd,&server.rtplist.fds);

		node = server.rtplist.head;
		while(node)
		{
			FD_SET(node->Sockfd, &server.rtplist.fds);
			node=node->next;
		}

		ret = select(server.rtplist.count +1,&server.rtplist.fds,NULL,NULL,&tv);
		printf("rtp ret=%d\n",ret);
		switch (ret)
		{
		case SOCKET_ERROR: //error
			printf("select return error:ret=%n,no=%d\n",ret, WSAGetLastError());
			break;
		case 0://timeout to check if quit
			printf("select timeout\n");
			break;
		default:
			{
			 if (FD_ISSET(server.rtplist.skfd,&server.rtplist.fds))
				 ProcessMainReq(&server.rtplist); //process a new request of stun packet.
	//		 else 
				 ProcessTurnReq(&server.rtplist); //process a turn request
			}
		}
	}
	return 0;
}

DWORD WINAPI ListenFromRTCP(LPVOID lpParameter)//用来分配RTP、TURN的端口
{
	int			ret =0;
	struct timeval tv;
	SessionNode *node;

	tv.tv_sec  = 2;
	tv.tv_usec = 0;

	while (!server.isquit)
	{
		FD_ZERO(&server.rtcplist.fds);
		FD_SET(server.rtcplist.skfd,&server.rtcplist.fds);

		node = server.rtcplist.head;
		while(node)
		{
			FD_SET(node->Sockfd, &server.rtcplist.fds);
			node=node->next;
		}

		ret = select(server.rtcplist.count +1,&server.rtcplist.fds,NULL,NULL,&tv);
		printf("rtcp ret=%d\n",ret);
		switch (ret)
		{
		case SOCKET_ERROR: //error
			printf("select return error:ret=%d, no=%d\n", ret, WSAGetLastError());
			break;
		case 0://timeout to check if quit
			printf("select timeout\n");
			break;
		default:
			{
			 if (FD_ISSET(server.rtcplist.skfd,&server.rtcplist.fds))
				 ProcessMainReq(&server.rtcplist); //process a new request of stun packet.
		//	 else 
				 ProcessTurnReq(&server.rtcplist); //process a turn request
			}
		}
	}
	return 0;
}


void menu(void)
{
	char cmd[80]; 
	
	printf("menu:\n");
	printf("1.show list:\n");
	printf("2.quit server:\n");
	while (1)
	{
		memset(cmd,0,80);
        gets(cmd);

		if (strnicmp(cmd,"1",1) == 0)
		{
			printf("RTP LIST:\n");
			ListShow(&(server.rtplist));
			printf("RTCP LIST:\n");
			ListShow(&(server.rtcplist));
			printf("BAK LIST:\n");
			ListShow(&(server.baklist));
		}
		if (strnicmp(cmd,"2",1) == 0)
		{
			server.isquit = 1;
			printf("system quit....\n");
			Sleep(4000);
			break;
		}
	}
}



int main()
{
	DWORD thread1,thread2,thread3;

	if(initNetwork()==TURNFAIL)
		return 0;

	if(turnInitServer() == TURNFAIL)
		return 0;

	CreateThread(NULL,0,TimerProcess,0,0,&thread3);
	CreateThread(NULL,0,ListenFromRTP,0,0,&thread1);	
	CreateThread(NULL,0,ListenFromRTCP,0,0,&thread2);

	menu();

	TurnExitServer();
	exitNetwork();

	return 0;
}