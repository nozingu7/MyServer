#pragma once
#include "common.h"

typedef struct UserInfo
{
	SOCKET sock;
	char szBuf[1024];
	char szIP[64];
	int iPort;
	char szName[20];
	year_month_day date;
	hh_mm_ss<milliseconds> time;
} USERINFO;

class CMyServer
{
public:
	explicit CMyServer();
	~CMyServer();

public:
	HRESULT NativeConstruct();
	HRESULT SetSwapChain(HWND hWnd, UINT WinX, UINT WinY);
	HRESULT SetBackBufferRTV();
	HRESULT SetDepthStencil(UINT WinX, UINT WinY);

public:
	HRESULT CreateServer(int iPort);
	HRESULT ReleaseServer();
	HRESULT RemoveClient(SOCKET sock);
	void Init_Imgui();

public:
	bool SendAll(USERINFO& userInfo);
	void MainWindow();
	bool isAlive();

public:
	void Render();

public:
	void ThreadAccept(void* pData);
	void ThreadWork(void* pData);

private:
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

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pDeviceContext = nullptr;
	IDXGISwapChain* m_pSwapChain = nullptr;
	ID3D11RenderTargetView* m_pBackBufferRTV = nullptr;
	ID3D11DepthStencilView* m_pDepthStencilView = nullptr;
};

