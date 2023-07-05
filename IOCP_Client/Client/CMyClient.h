#pragma once
#include "common.h"

typedef struct UserInfo
{
	char szName[20];
	char szBuf[1024];
} USERINFO;

typedef struct Packet
{
	SOCKET sock;
	char szName[20];
	char szBuf[128];
} PACKET;

class CMyClient
{
public:
	CMyClient();
	~CMyClient();

public:
	HRESULT NativeConstruct();
	HRESULT SetSwapChain(HWND hWnd, UINT WinX, UINT WinY);
	HRESULT SetBackBufferRTV();
	HRESULT SetDepthStencil(UINT WinX, UINT WinY);
	HRESULT ConnectServer(const char* szName);

public:
	void Init_Imgui();
	void Render();
	void ShowChat();
	void Release();
	void ThreadRecv(void* pData);
	void TextCenter(const char* str);
	void WindowCenter(const char* str);
	void ConnectFail();
	void Menu();
	bool Alive();

private:
	SOCKET m_sock;
	thread m_thread;
	bool m_bConnectEnable;
	char m_szBuf[1024];
	char m_szInputBuf[256];
	vector<USERINFO*> m_vecInfo;
	USERINFO m_Info;
	bool m_bConnectFail = false;
	bool m_bAlive = true;

private:
	shared_ptr<ID3D11Device*> m_pDevice;
	shared_ptr<ID3D11DeviceContext*> m_pDeviceContext;
	shared_ptr<IDXGISwapChain*> m_pSwapChain;
	shared_ptr<ID3D11RenderTargetView*> m_pBackBufferRTV;
	shared_ptr<ID3D11DepthStencilView*> m_pDepthStencilView;
};

