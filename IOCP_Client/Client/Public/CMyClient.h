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
	void SendMsg(const char* szStr);

public:
	void LoginProgress(char* pBuffer);
	void SignupProgress();
	void Disconnect();
	bool DuplicateCheck(const char* str, DuType eType);
	bool SignupRequest(const char* szID, const char* szPass, const char* szNick);

private:
	SOCKET m_sock;
	thread m_thread;
	vector<USERINFO*> m_vecInfo;
	char m_szName[30];
	char m_szInputBuf[256];
	char m_szSignupID[20];
	char m_szSignupPassword[20];
	char m_szSignupNickName[20];

private:
	bool m_bConnectEnable;
	bool m_bConnectFail = false;
	bool m_bAlive = true;
	bool m_bCheck = false;
	bool m_bChatting = false;
	bool m_bLogin = false;
	bool m_bSignup = false;
	bool m_bIDCheck = false;
	bool m_bNickCheck = false;

private:
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pDeviceContext;
};

