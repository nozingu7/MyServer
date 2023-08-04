#pragma once

enum MsgState
{
	RECV,
	SEND
};

enum MsgType
{
	CHATTING,
	LOGIN,
	SIGNUP,
	DUPLICATECHECK
};

enum DuType
{
	ID,
	NICKNAME
};

struct MyOverlapped
{
	OVERLAPPED overlap;
	WSABUF wsaBuf;
	MsgState state;
};

typedef struct UserInfo
{
	SOCKET sock;
	char szRecvBuf[1024];
	char szSendBuf[1024];
	char szIP[64];
	int iPort;
	char szName[30];
	year_month_day date;
	hh_mm_ss<milliseconds> time;
	MyOverlapped RecvOverlap;
	MyOverlapped SendOverlap;
} USERINFO;

typedef struct PacketHeader
{
	int iNameLen;
	int iMsgLen;
} PACKETHEADER;

typedef struct LoginHeaderToServer
{
	int iIDLen;
	int iPasswordLen;
} LOGIN_HEADER_TO_SERVER;

typedef struct LoginHeaderToClient
{
	bool bSuccess;
	int iNickNameLen;
} LOGIN_HEADER_TO_CLIENT;

typedef struct DuplicateHeader
{
	DuType eType;
	bool bSuccess;
	int iNickNameLen;
} DUPLICATE_HEADER;

typedef struct SignupHeader
{
	int iIDLen;
	int iPasswordLen;
	int iNickNameLen;
} SIGNUP_HEADER;

typedef struct Packet
{
	char szBuf[1024];
} PACKET;