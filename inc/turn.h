#ifndef _TURN_H
#define _TURN_H

#include "types.h"
#include "udp.h"

#define RTPPORT		11111
#define RTCPPORT	11112

#define MAPPEDADDRESS 0x0001
#define USERNAME 0x0006
#define MESSAGEINTEGRITY 0x0008
#define ERRORCODE 0x0009
#define UNKNOWNATTRIBUTES 0x000a
#define TRANSPORTPREFERENCES 0x000c
#define LIFETIME 0x000d
#define BANDWIDTH 0x0010
#define ALTERNATESERVER 0x000e
#define MAGICCOOKIE 0x000f

#define RTPTYPE						0x1000
#define BINDREQMSG					 0x0101
#define BINDRSPMSG					 0x0102
/*
#define BindErrorResponseMsg         0x0111
#define SharedSecretRequestMsg       0x0002
#define SharedSecretResponseMsg      0x0102
#define SharedSecretErrorResponseMsg 0x0112
*/
#define  IPV4FAMILY  0x01
#define  IPV6FAMILY  0x02


#define  ALLOCREQ         0x0003
#define  ALLOCRSP		  0x0103
#define  ALLOERRRSP       0x0113

//--------自定义类型-----------//
#define   TURNRELAYREQMSG	0x0211
#define   TURNRELAYRSPMSG	0x0212




#define  TURN_MAX_STRING  256
#define  TURN_MAX_UNKNOWN_ATTRIBUTES 4
#define  TURN_MAX_MESSAGE_SIZE 512

#define  MAX_TIMEOUT_NUM	30   //yfs
#define	 MIN_RELAY_PORT		12345


#define  NODECALLER			1
#define  NODEWORK			2

#define  RTPDATA 0
#define  RTCPDATA 1

typedef enum 
{
	noExist =1,
	SourceExist,
	DestinateExist,
	BothExist,
} AddressExist;

typedef struct //20字节
{
	S16 msgType;
	S16 msgLength;
	U128 id;
} TurnMsgHdr;

typedef struct //属性头
{
	S16 type;
	S16 lenth;
}TurnAtrHdr;


typedef struct//地址和端口
{
	S16 port;
	U32 addr;
} TurnAddr;

typedef struct
{
	U8 pad;
	U8 family;
	TurnAddr ipv4;
}TurnAtrAddr;

typedef struct  
{
	U8 pad;
	U8 type;
	TurnAtrAddr  turnAtrAddr;
}TurnAtrPrefference;

typedef struct
{
	U32 value;
}TurnAtrInt32;

typedef struct
{
	char value[TURN_MAX_STRING];      
	S16 sizeValue;
} TurnAtrString;

typedef struct
{
	S16 pad; // all 0
	U8 errorClass;
	U8 number;
	char reason[TURN_MAX_STRING];
	S16 sizeReason;
} TurnAtrError;

typedef struct
{
	char hash[20];
} TurnAtrIntegrity;

typedef struct
{
	S16 attrType[TURN_MAX_UNKNOWN_ATTRIBUTES];
	S16 numAttributes;
} TurnAtrUnknown;

typedef struct
{
	TurnMsgHdr msgHdr;
	
	bool hasMappedAddress;
	TurnAtrAddr  mappedAddress;

	bool hasTurnedAddress;
	TurnAtrAddr  turnnedAddress;

// 	bool hasPrefference;
// 	TurnAtrPrefference transportPrefference;
	/*
	bool hasUsername;
	TurnAtrString username;
	
	bool hasPassword;
	TurnAtrString password;
	
	bool hasMessageIntegrity;
	TurnAtrIntegrity messageIntegrity;
	
	bool hasErrorCode;
	TurnAtrError errorCode;
	
	bool hasUnknownAttributes;
	TurnAtrUnknown unknownAttributes;
	
	
	bool hasLifeTime;
	TurnAtrInt32 lifeTime;
	
	bool hasMagicCookie;
	TurnAtrInt32 magiccookie;

	bool hasBandWidth;
	TurnAtrInt32 bandwidth;

	bool hasAlternateSrv;
	TurnAtrAddr  alternateSrv;

	bool hasmoreAliavable;
	TurnAtrAddr  moreAliavable;
	*/
} TurnMessage; 


typedef struct sNode
{
	TurnAddr callerAddr;
	TurnAddr calleeAddr;

	Socket   Sockfd;
	unsigned short Relayport;
	
	char	lastTime;
	char	status;

	struct sNode *next;
}SessionNode;

typedef struct sList
{
	SessionNode *head;
	SessionNode *tail;
	fd_set		fds;
	Socket		skfd;
	int			count;
	short		type;
	CRITICAL_SECTION gcs;

}SessionList;


struct Serverinf
{
	SessionList rtplist;
	SessionList rtcplist;
	SessionList baklist;
	int curNodeNum;
	unsigned short turnPort;

	char isquit;
}; 

int CompareAddr(TurnAddr *sour,TurnAddr *dest);
void ListAddTail(SessionList *plist,SessionNode *pnode);
void ListAddHead(SessionList *plist,SessionNode *pnode);
SessionNode *ListGetHead(SessionList *plist);
SessionNode *ListSearch(SessionList *pcur, TurnAddr *addr);
void ListCheckTimeout(SessionList *plist);
void ListShow(SessionList *plist);
void ListDelete(SessionList *plist);
SessionNode *OpenNewNode(void);
int  turnInitServer(void);
void TurnExitServer(void);
void stunBuildReqHead(TurnMsgHdr* header,S16 type);
void stunBuildRspHead(TurnMsgHdr *header,TurnMsgHdr recvd,S16 type);
int TLVDecode(TurnMessage *message,char *buff);
int TLVEncode(TurnMessage *message,char *buff);
void stunSendSimpleRsp(TurnMessage recvmessage,TurnAddr dest,Socket fd,int type);
void turnBuildRsp(TurnMessage recved,TurnAddr *dest,SessionNode *node,Socket sock);
void turnRelay(char *buff,TurnAddr *source,SessionNode *node);
void ProcessMainReq(SessionList *plist);
void SetNode(SessionList *plist, TurnAddr *client, SessionNode *node);
void ProcessTurnReq(SessionList *plist);

 extern struct Serverinf server;

#endif

