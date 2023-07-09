#pragma once

enum MsgState
{
	RECV,
	SEND
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

typedef struct Packet
{
	char szBuf[1024];
} PACKET;