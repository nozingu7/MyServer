#pragma once
#include "common.h"
#include "Define.h"

class CMyClient
{
public:
	CMyClient(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	~CMyClient();

private:
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
	char m_szName[30];
	char m_szInputBuf[256];
	vector<USERINFO*> m_vecInfo;
	bool m_bConnectFail = false;
	bool m_bAlive = true;

private:
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pDeviceContext;
};

