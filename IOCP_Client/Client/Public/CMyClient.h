#pragma once
#include "common.h"
#include "Define.h"

class CMyClient
{
public:
	CMyClient(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	~CMyClient();

private:
	HRESULT ConnectServer();

public:
	bool Alive();

public:
	void Init_Imgui();
	void Render();
	void ShowChat();
	void Release();
	void ThreadRecv(void* pData);
	void TextCenter(const char* str);
	void WindowCenter(const char* str);
	void ConnectFail();
	void JoinServer();
	void LoginWindow();

public:
	void LoginProgress(char* pBuffer);
	void Disconnect();


private:
	SOCKET m_sock;
	thread m_thread;
	bool m_bConnectEnable;
	char m_szName[30];
	char m_szInputBuf[256];
	vector<USERINFO*> m_vecInfo;
	bool m_bConnectFail = false;
	bool m_bAlive = true;
	bool m_bCheck = false;
	bool m_bChatting = false;

private:
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pDeviceContext;
};

