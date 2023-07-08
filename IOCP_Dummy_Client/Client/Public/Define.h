#pragma once

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

typedef struct Packet
{
	char szBuf[1024];
} PACKET;