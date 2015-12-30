#include ".\inc\turn.h"
#include ".\inc\udp.h"


//�Ƚ�Դ�˺�Ŀ�Ķ˿��Ƿ�һ��
int CompareAddr(TurnAddr *sour,TurnAddr *dest)
{
	if (sour != NULL && dest != NULL)
	{
		if (sour->addr == dest->addr && sour->port == dest->port)
		{
			return TURNSUCCEED;
		}
	}

	return TURNFAIL;
}

//���ڵ���ӵ����һ��
void ListAddTail(SessionList *plist,SessionNode *pnode)
{
	if(plist->head == NULL)
	{
		plist->head = pnode;
		plist->tail = pnode;
	}
	else
	{
		plist->tail->next = pnode;
		plist->tail=pnode;
	}
	
	plist->count++;
}

//���ڵ���ӵ���ǰһ��
void ListAddHead(SessionList *plist,SessionNode *pnode)
{
	SessionNode *temp;

	if(plist->head == NULL)
	{
		plist->head = pnode;
		plist->tail = pnode;
	}
	else
	{
		temp = plist->head;
		plist->head = pnode;
		plist->head->next = temp;
	}

	plist->count++;
}

//remove a node from list's head
SessionNode *ListGetHead(SessionList *plist)
{
	SessionNode *temp=NULL;

	if(plist->count>0)
	{
		temp = plist->head;
		plist->head = plist->head->next;

		if(plist->count == 1)
			plist->tail = plist->head;
	}

	plist->count--;

	return temp;

}


//remove the pcur node which time is out to pbak,when finding address
SessionNode *ListSearch(SessionList *pcur, TurnAddr *addr)
{
	SessionNode *temp,*last,*cur, *ret=NULL;

	last = pcur->head;
	cur = temp = last;

	while (pcur->count > 0 && cur != NULL)
	{
		if (CompareAddr(&(cur->callerAddr),addr) == TURNFAIL && CompareAddr(&(cur->calleeAddr),addr) == TURNFAIL )
		{
			if(temp->lastTime >= MAX_TIMEOUT_NUM) //find a node timeout
			{
				if(pcur->head != temp)
					last->next = temp->next;
				else
				{
					pcur->head = temp->next;
					last = temp->next;
				}
				cur = temp->next;
				pcur->count--;

				temp->next = NULL;
				temp->calleeAddr.addr=0;
				temp->callerAddr.addr=0;
				temp->calleeAddr.port=0;
				temp->callerAddr.port=0;
				temp->lastTime = 0;
				temp->status = 0;
				ListAddTail(&server.baklist,temp);
//				FD_CLR(temp->Sockfd, &(pcur->fds));
			}
			else
			{
				last = cur;
				cur = cur->next;
			}
		}
		else
		{
			cur->lastTime = 0;
			ret = cur;
			break;
		}

	}
	return ret;
}


void ListCheckTimeout(SessionList *plist)
{

	SessionNode *cur = plist->head;

	EnterCriticalSection(&(plist->gcs));

	while(cur)
	{
		cur->lastTime++;
		cur = cur->next;
	}

	LeaveCriticalSection(&(plist->gcs));
}


//��ʾ�����е�����
void ListShow(SessionList *plist)
{
	SessionNode *cur = plist->head;

	while(cur)
	{
		printf("caller IP:%d,",	cur->callerAddr.addr);
		printf("caller PORT:%d\n",	cur->callerAddr.port);
		printf("callee IP%d,",	cur->calleeAddr.addr);
		printf("callee PORT:%d\n",	cur->calleeAddr.port);
		printf("last:%d\n", cur->lastTime);
		
		cur = cur->next;
	}
}

//ɾ������
void ListDelete(SessionList *plist)
{
	SessionNode *temp,*cur = plist->head;

	while(cur)
	{
		temp = cur;
		cur=cur->next;
		closesocket(cur->Sockfd);

		free(temp);
	}
}


void TurnExitServer(void)
{
	closesocket(server.rtplist.skfd); 
	closesocket(server.rtcplist.skfd);

	ListDelete(&server.rtplist);
	ListDelete(&server.rtcplist);
	ListDelete(&server.baklist);

	DeleteCriticalSection(&(server.rtplist.gcs));
	DeleteCriticalSection(&(server.rtcplist.gcs));
	DeleteCriticalSection(&(server.baklist.gcs));
}

int  turnInitServer(void)
{
	server.rtplist.skfd  = openPort(RTPPORT); 
	server.rtcplist.skfd = openPort(RTCPPORT);


	if (server.rtplist.skfd == INVALID_SOCKET || server.rtcplist.skfd == INVALID_SOCKET)
	{
		if (server.rtplist.skfd > 0)
		{
			closesocket(server.rtplist.skfd);
		}

		if (server.rtcplist.skfd > 0)
		{
			closesocket(server.rtcplist.skfd);
		}

		printf("Can't open server socket!!\n");

		return TURNFAIL;
	}

	server.rtplist.type = RTPDATA;
	server.rtcplist.type = RTCPDATA;

	InitializeCriticalSection(&(server.rtplist.gcs));
	InitializeCriticalSection(&(server.rtcplist.gcs));
	InitializeCriticalSection(&(server.baklist.gcs));

	server.turnPort = MIN_RELAY_PORT;
}


//��������ͷ����Ϣ
void stunBuildReqHead(TurnMsgHdr* header,S16 type)
{
	int i,r, r1, r2;

	memset(header,0,sizeof(TurnMsgHdr));

 	for (i=0;i < 16;i+=4)
 	{
		srand((unsigned)GetTickCount());
		r1 =rand();
		srand((unsigned)GetTickCount()+1);
		r2 =rand();
		r = (r1<<16) + r2;
 		header->id.octet[i+0] = r>>0;
 		header->id.octet[i+1] = r>>8;
 		header->id.octet[i+2] = r>>16;
 		header->id.octet[i+3] = r>>24;
 	}
	header->msgType = htons(type);
}

//������Ӧͷ����Ϣ
void stunBuildRspHead(TurnMsgHdr *header,TurnMsgHdr recvd,S16 type)
{
	memcpy(header,&recvd,sizeof(TurnMsgHdr));
	header->msgType = htons(type);
}


int TLVDecode(TurnMessage *message,char *buff)//����          ��
{
	int size;
	int type;

	if((*buff) & 0x80) //���λΪ1
	{
		type = RTPTYPE;
		return type;
	}

	memcpy(&message->msgHdr,buff,sizeof(TurnMsgHdr));

	message->msgHdr.msgLength = ntohs(message->msgHdr.msgLength);
	message->msgHdr.msgType =   ntohs(message->msgHdr.msgType);

	size = message->msgHdr.msgLength;
	
	//debug information--------------------------------------------
	switch(message->msgHdr.msgType)
	{
			case ALLOCREQ:
				{
					printf("AllocateRequest:");
				}
				break;
			case ALLOCRSP:
				{
					printf("AllocateResponse:");
				}
				break;
			case BINDREQMSG:
				{
					printf("BindRequestMsg:");
				}
				break;
			case BINDRSPMSG:
				{
					printf("BindResponseMsg:");
				}
				break;
			case TURNRELAYREQMSG:
				{
					printf("TurnRelayRequestMsg:");
				}
				break;
			case TURNRELAYRSPMSG:
				{
					printf("TurnRelayResponseMsg:");
				}
				break;

			default:
				{
					printf("unkown");
				}
				break;
	}
	//debug information over----------------------------------------------

	buff += sizeof(TurnMsgHdr);
	while(size >0)
	{
		S16 attrLen;
		S16 attrType;

		memcpy(&attrType,buff,2);
		buff += 2;
		attrLen = (*buff);
		buff += 2;
		size -= 4;
		attrType = ntohs(attrType);
		switch (attrType)
		{
			case MAPPEDADDRESS:
				{
					message->hasMappedAddress = true;
					memcpy(&message->mappedAddress.pad,buff,1);
					buff ++;
					memcpy(&message->mappedAddress.family,buff,1);
					buff ++;
					memcpy(&message->mappedAddress.ipv4.port,buff,2);
					buff+= 2;
					memcpy(&message->mappedAddress.ipv4.addr,buff,4);
					buff += 4;
					printf("MAPPEDADDRESS");
				}
			break;
			case TRANSPORTPREFERENCES:
				{
					message->hasTurnedAddress = true;
					memcpy(&message->turnnedAddress.pad,buff,1);
					buff ++;
					memcpy(&message->turnnedAddress.family,buff,1);
					buff ++;
					memcpy(&message->turnnedAddress.ipv4.port,buff,2);
					buff += 2;
					memcpy(&message->turnnedAddress.ipv4.addr,buff,4);
					buff += 4;
					printf("TRANSPORTPREFERENCES");

				}
			break;
			default:
				break;
		}
		size -= attrLen; 
	}

	return message->msgHdr.msgType;
}


int TLVEncode(TurnMessage *message,char *buff)//����
{
	int len = sizeof(TurnMsgHdr);

	memset(buff,0,TURN_MAX_MESSAGE_SIZE);
	memcpy(buff,&message->msgHdr,len);
	buff = buff+len;
	
	if (message->hasMappedAddress)
	{
		S16 atrlen  = htons(0x0008);
		S16 type = htons(MAPPEDADDRESS);
		U8 family = IPV4FAMILY;

		memcpy(buff,&type,2);
		buff += 2;
		memcpy(buff,&atrlen,2);
		buff += 2;
		*(unsigned short *)buff++ = htons(message->mappedAddress.pad);
		memcpy(buff,&family,1);
		buff ++;
		message->mappedAddress.ipv4.port = (message->mappedAddress.ipv4.port);
		memcpy(buff,&message->mappedAddress.ipv4.port,2);
		buff += 2;
		message->mappedAddress.ipv4.addr = (message->mappedAddress.ipv4.addr);
		memcpy(buff,&message->mappedAddress.ipv4.addr,4);
		buff += 4;
		len = len +12;
	}

	if (message->hasTurnedAddress)
	{
		S16 atrlen  = htons(0x0008);
		S16 type = htons(TRANSPORTPREFERENCES);
		U8 family =IPV4FAMILY;

		memcpy(buff,&type,2);
		buff += 2;
		memcpy(buff,&atrlen,2);
		buff += 2;
		*(unsigned short *)buff++ = htons(message->turnnedAddress.pad);
		memcpy(buff,&family,1);
		buff ++;
		message->turnnedAddress.ipv4.port = (message->turnnedAddress.ipv4.port);
		memcpy(buff,&message->turnnedAddress.ipv4.port,2);
		buff += 2;
		message->turnnedAddress.ipv4.addr = (message->turnnedAddress.ipv4.addr);
		memcpy(buff,&message->turnnedAddress.ipv4.addr,4);
		buff += 4;
		len =len +12;
	}
	buff = buff-len;
	message->msgHdr.msgLength = htons(len);
	return len;
}


//���ͼ򵥻ظ����·���TURN�Ķ˿�
void stunSendSimpleRsp(TurnMessage recvmessage,TurnAddr dest,Socket fd,int type)
{
	char buff[TURN_MAX_MESSAGE_SIZE];
	int len  = 0;
	TurnMessage message;

	memset(&message,0,sizeof(TURN_MAX_MESSAGE_SIZE));
	stunBuildRspHead(&(message.msgHdr),recvmessage.msgHdr,type);//����stunͷ����Ϣ
	len = TLVEncode(&message,buff);
	sendMessage(fd,buff,len,dest.addr,dest.port);
}

//����������TURN�ظ�
void turnBuildRsp(TurnMessage recved,TurnAddr *dest,SessionNode *node,Socket sock)               
{

	int len;
	char buff[TURN_MAX_MESSAGE_SIZE];
	TurnMessage resp;

	memset(&resp,0,sizeof(TurnMessage));
	memset(buff,0,TURN_MAX_MESSAGE_SIZE);

	resp.msgHdr.msgType = ALLOCRSP;
	stunBuildRspHead(&resp.msgHdr,recved.msgHdr,ALLOCRSP);

	resp.msgHdr.msgLength = htons(24);
	resp.hasMappedAddress = true;
	
	resp.mappedAddress.ipv4.addr = (node->callerAddr.addr);//�������Ŀͻ����߳�
	resp.mappedAddress.ipv4.port = node->callerAddr.port;

	resp.hasTurnedAddress = true;
	resp.turnnedAddress.ipv4.addr = (GetLocalIp());
	resp.turnnedAddress.ipv4.port = node->Relayport;


	len = TLVEncode(&resp,buff);
	sendMessage(sock,buff,len,dest->addr,dest->port); //cms�޸�
	
	return;
}

void turnRelay(char *buff,TurnAddr *source,SessionNode *node)//������ת������
{
	TurnAddr *dest;
	int len = 20;

	if(CompareAddr(source,&(node->calleeAddr))==TURNSUCCEED) //ȷ��ת������
		dest = &(node->callerAddr);
	else  
		dest = &(node->calleeAddr);
	
	if (node->status == NODECALLER) //ת�����ݵ�Դ�˺�Ŀ����һ�������ڵ�ʱ�򣬼��ϵ�ַ
	{
		if(dest == &(node->calleeAddr))
		{
			printf("Very surprise situation\n");
			node->callerAddr.addr = source->addr;
			node->callerAddr.port = source->port;
		}
		else
		{
			node->calleeAddr.addr = source->addr;
			node->calleeAddr.port = source->port;
		}
		node->status = NODEWORK;
	}
	
	sendMessage(node->Sockfd,buff,len,dest->addr,dest->port);
}


void SetNode(SessionList *plist, TurnAddr *client, SessionNode *node)
{
	ListAddHead(plist,node);
//	FD_SET(node->Sockfd,&(plist->fds));

	node->callerAddr.addr = client->addr;
	node->callerAddr.port = client->port;
	node->status = NODECALLER;
}


SessionNode *OpenNewNode(void)
{
	SessionNode *node;
	Socket		fd;

	node = (SessionNode *)malloc(sizeof(SessionNode));
	if(node == NULL)
	{
		printf("No memory in malloc node\n");
		return NULL;
	}

	memset(node, 0, sizeof(SessionNode));

	do 
	{
		server.turnPort ++;
		if(server.turnPort < MIN_RELAY_PORT)
			server.turnPort = MIN_RELAY_PORT;
		fd = openPort(server.turnPort);
	} 
	while(fd == INVALID_SOCKET);

	node->Relayport = server.turnPort;
	if (node->Relayport == 0)
	{
		printf("\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
	}
	node->Sockfd = fd;

	return node;
}


void ProcessMainReq(SessionList *plist)
{
	TurnAddr	client;
	char		buff[TURN_MAX_MESSAGE_SIZE];
	TurnMessage message;
	int type;
	SessionNode* node;
	struct in_addr in;

	if(TURNFAIL==getMessage(plist->skfd,buff,TURN_MAX_MESSAGE_SIZE,&client.addr,&client.port,0))
	{
		printf("read packet error\n");
		return;
	}

	type = TLVDecode(&message,buff); //����
	if(type != ALLOCREQ) //now, we only support the stun type.
	{
		printf("receive error stun packet, current only support the 0x0001, now is %d:\n",type);
		return;
	}

	node = ListSearch(plist,&client);

	in.s_addr = client.addr;
	printf("%s",inet_ntoa(in));

	if(node) //node exist.
		printf("node exist!!\n");
	else //find new node
	{
		EnterCriticalSection(&(server.baklist.gcs));
		node = ListGetHead(&server.baklist);
		LeaveCriticalSection(&(server.baklist.gcs));

		if(node == NULL) //û�б�������,�����ڴ棬�����׽���...
		{
			node = OpenNewNode();//����һ���½ڵ㣬���׽���
			if(node == NULL)
				return;
		}

		SetNode(plist, &client, node); //���ڵ�����������
	}

	turnBuildRsp(message, &client, node,plist->skfd);//&client,server.myrtcpfd,RTCPDATA);
}


void ProcessTurnReq(SessionList *plist)
{
	int i = 1,ret = 0,j = 0;
	int packettype,len  =0;
	SessionNode *node;
	TurnMessage message;
	TurnAddr client;
	char buff[TURN_MAX_MESSAGE_SIZE];

	char tempbuff[TURN_MAX_MESSAGE_SIZE];
	node = plist->head;
	while(node)
	{
		if(FD_ISSET(node->Sockfd, &plist->fds))
		{
			if(getMessage(node->Sockfd, buff,TURN_MAX_MESSAGE_SIZE, &client.addr,&client.port,len) != TURNSUCCEED)
			{
				printf("error in recvfrom: %d",getErrno());
				break;
			} 
			memcpy(tempbuff,buff,TURN_MAX_MESSAGE_SIZE);//cms
			packettype = TLVDecode(&message,buff);
			node->lastTime = 0;
			printf("received a packet, type is = %d\n",packettype);
			
			if (packettype != RTPTYPE)
			{
				switch(message.msgHdr.msgType)
				{
					case BINDREQMSG: //�ͻ������Է������·����TURN�˿�
					{
						 printf("get bind\n");
						 stunSendSimpleRsp(message,client,node->Sockfd,BINDRSPMSG);
					 }
					break;

					case TURNRELAYREQMSG: //�ͻ���Bͨ��TURN����������
					{
						if(node->status == NODECALLER)
						{
							 node->calleeAddr.port = client.port;
							 node->calleeAddr.addr = client.addr;
							 node->status = NODEWORK;
						}

						printf("relay request\n");
						turnRelay(tempbuff,&client,node,message.msgHdr.msgLength);//buff�ĳ�tempbuff
					}
						 break;

					case TURNRELAYRSPMSG: //�ͻ���Aͨ��TURN���ͻظ�����
					{
						printf("relay response\n");
						turnRelay(tempbuff,&client,node,message.msgHdr.msgLength);//buff�ĳ�tempbuff
					}
					break;

					default:
						 printf("UNKNOWN TYPE;\n");
						 break;
				}
			}
			else
			{
				printf("RTP/RTCP packet��turn relay\n");
				turnRelay(tempbuff,&client,node,len);//ת������
			}
		}
		node = node->next;
	}
}



/*
//�ͻ���callee��TURN�˿ڷ���������Ϣ
void turnRelayRequest(TurnMessage *resp,TurnAddr *dest,Socket fd)
{
	char buff[TURN_MAX_MESSAGE_SIZE];
	int len = 0;

	memset(resp,0,sizeof(TurnMessage));
	memset(buff,0,TURN_MAX_MESSAGE_SIZE);
	stunBuildReqHead(&resp->msgHdr,TURNRELAYREQMSG);//����ͷ����Ϣ
	len = TLVEncode(resp,buff);
	sendMessage( fd,buff,len,dest->addr,dest->port);

}
//�ͻ���caller��TURN�˿ڷ��ͻظ���Ϣ
void turnRelayResponse(TurnMessage *resp,TurnMessage recved,TurnAddr *dest,Socket fd)
{
	char buff[TURN_MAX_MESSAGE_SIZE];
	int len = 0;

	stunBuildRspHead(&resp->msgHdr,recved.msgHdr,TURNRELAYRSPMSG);
	len = TLVEncode(resp,buff);
	sendMessage( fd,buff,len,dest->addr,dest->port);
}

//���ͼ��������ڴ�ͨ�·���TURN�Ķ˿�-------------------------�ͻ�����
void stunSendSimpleReq(TurnMessage *message,TurnAddr dest,Socket fd)
{
	char buff[TURN_MAX_MESSAGE_SIZE];
	int len = 0;

	memset(buff,0,TURN_MAX_MESSAGE_SIZE);
	memset(message,0,sizeof(TurnMessage));
	stunBuildReqHead(&message->msgHdr,BINDREQMSG);//����stunͷ����Ϣ

	len = TLVEncode(message,buff);
	sendMessage(fd,buff,len,dest.addr,dest.port);
}
//�ͻ�������TURN����----------------------------------------�ͻ�����
void turnSendReq(TurnMessage *resp,TurnAddress4 *dest,Socket myfd)
{
	char buff[TURN_MAX_MESSAGE_SIZE];
	int len = 0;

	memset(resp,0,sizeof(TurnMessage));
	memset(buff,0,TURN_MAX_MESSAGE_SIZE);
	stunBuildReqHead(&resp->msgHdr,ALLOCREQ);//����ͷ����Ϣ
	len = TLVEncode(resp,buff);
	sendMessage(myfd,buff,len,dest->addr,dest->port);
}
*/