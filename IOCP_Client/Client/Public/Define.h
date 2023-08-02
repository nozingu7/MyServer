#pragma once

enum MsgType
{
	CHATTING,
	LOGIN,
	SIGNUP
};

typedef struct UserInfo
{
	char szName[20];
	char szBuf[1024];
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

typedef struct Packet
{
	char szBuf[1024];
} PACKET;