#pragma once

typedef struct UserInfo
{
	SOCKET sock;
	char szBuf[1024]; // 필요없음
	char szIP[64];
	int iPort;
	char szName[20];
	year_month_day date;
	hh_mm_ss<milliseconds> time;
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