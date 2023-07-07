#pragma once
#include "common.h"
#include "Define.h"

class CMyServer
{
public:
	CMyServer();
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
	void GetIP();

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

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pDeviceContext = nullptr;
	IDXGISwapChain* m_pSwapChain = nullptr;
	ID3D11RenderTargetView* m_pBackBufferRTV = nullptr;
	ID3D11DepthStencilView* m_pDepthStencilView = nullptr;
};

