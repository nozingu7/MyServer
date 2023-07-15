#pragma once
#include "common.h"
#include "Define.h"

class CMyServer
{
public:
	CMyServer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	~CMyServer();

public:
	HRESULT CreateServer(int iPort);
	HRESULT ReleaseServer();
	HRESULT RemoveClient(SOCKET sock);
	USERINFO* SetUpUserData(SOCKET sock, SOCKADDR_IN addr);
	void Init_Imgui();

public:
	bool SendAll(USERINFO& userInfo, int iSize);
	void MainWindow();
	bool isAlive();
	void GetIP();
	char* UTF8ToMultiByte(char* msg);
	char* MultiByteToUTF8(char* msg);

public:
	void Render();

public:
	void ThreadAccept(void* pData);
	void ThreadWork(void* pData);

private:
	char m_szIP[20];
	int m_iPort = 0;
	bool m_bRun = false;
	bool m_bAlive = true;
	bool m_bWorkerThread = false;
	bool m_bAcceptThread = false;

private:
	WSADATA m_wsaData;
	SOCKET m_listenSock;
	vector<USERINFO*> m_vecClient;
	HANDLE m_iocp;
	thread m_threadApt;
	vector<thread> m_vecThreadWorker;
	CRITICAL_SECTION m_cs;
	OVERLAPPED sendOverlap;
	OVERLAPPED recvOverlap;

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pDeviceContext = nullptr;
};

