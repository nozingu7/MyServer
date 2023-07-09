#pragma once
#include "common.h"
#include "Define.h"

struct Dummy
{
	SOCKET sock;
	char szName[30];
};

class CMyClient
{
public:
	CMyClient(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	~CMyClient();

private:
	HRESULT ConnectServer(const char* szName);

public:
	void Init_Imgui();
	void Render(double TimeDelta);
	void ShowChat(double TimeDelta);
	void Release();
	void ThreadRecv(void* pData);
	void TextCenter(const char* str);
	void WindowCenter(const char* str);
	void ConnectFail();
	void JoinServer();
	bool Alive();
	void AutoChat();

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
	vector<Dummy*> m_vecDummy;
	vector<thread> m_vecThread;
	int m_iDummySize = 0;
	vector<const char*> m_vecChat;
	double m_ChatTime = 0.0;

private:
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pDeviceContext;
};

