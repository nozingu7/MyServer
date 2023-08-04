#include "CMyClient.h"

CMyClient::CMyClient(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:m_pDevice(pDevice), m_pDeviceContext(pContext)
{
	m_pDevice->AddRef();
	m_pDeviceContext->AddRef();
	memset(m_szName, 0, sizeof(m_szName));
	memset(m_szInputBuf, 0, sizeof(m_szInputBuf));
	memset(m_szSignupID, 0, sizeof(m_szSignupID));
	memset(m_szSignupPassword, 0, sizeof(m_szSignupPassword));
	memset(m_szSignupNickName, 0, sizeof(m_szSignupNickName));
	m_sock = 0;
	m_bConnectEnable = false;
}

CMyClient::~CMyClient()
{
	Release();
}

HRESULT CMyClient::ConnectServer()
{
	WSAData wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata))
	{
		cout << "WINSOCK INIT FAILED!\n";
		return E_FAIL;
	}

	m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(15000);

	if (SOCKET_ERROR == connect(m_sock, (SOCKADDR*)&addr, sizeof(addr)))
	{
		cout << "Connnect Server Failed!\n";
		return E_FAIL;
	}

	m_bConnectEnable = true;
	m_bLogin = true;

	m_thread = thread(&CMyClient::ThreadRecv, this, nullptr);

	return S_OK;
}

void CMyClient::Init_Imgui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.Fonts->AddFontFromFileTTF("..\\..\\Fonts\\SCDream5.otf", 18.f, NULL, io.Fonts->GetGlyphRangesKorean());

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();

	// 서버 접속 버튼 비활성화 상태일때 알파값 그대로 유지
	style.DisabledAlpha = 1.f;

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(g_hWnd);
	ImGui_ImplDX11_Init(m_pDevice, m_pDeviceContext);
}

void CMyClient::Render()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (ImGui::Begin("MENU"))
	{
		// 서버에 접속 중이라면 버튼 비활성화
		ImGui::BeginDisabled(m_bConnectEnable);
		if (ImGui::Button(u8"서버 접속", ImVec2(150.f, 50.f)))
		{
			if (!m_bConnectEnable)
				JoinServer();
		}
		ImGui::EndDisabled();

		ImGui::SameLine();

		if (ImGui::Button(u8"서버 종료", ImVec2(150.f, 50.f)))
		{
			m_bAlive = false;
			m_bConnectEnable = false;
		}
	}
	ImGui::End();

	// Client GUI Render
	if (m_bLogin)
		LoginWindow();

	if (m_bSignup)
		SignupProgress();

	// 로그인 성공했을 경우 채팅 접속
	if (m_bChatting)
		ShowChat();

	if (!m_bConnectEnable)
		Disconnect();

	ImGui::Render();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO(); (void)io;

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void CMyClient::ShowChat()
{
	if (ImGui::Begin(u8"채팅"))
	{
		ImGui::BeginChild("Chatting", ImVec2(0, -25), true, 0);

		int iSize = (int)m_vecInfo.size();
		for (int i = 0; i < iSize; i++)
			ImGui::Text("%s : %s", m_vecInfo[i]->szName, m_vecInfo[i]->szBuf);

		ImGui::EndChild();

		if (ImGui::InputText("##", m_szInputBuf, IM_ARRAYSIZE(m_szInputBuf), ImGuiInputTextFlags_EnterReturnsTrue))
		{
			if (strcmp("", m_szInputBuf))
			{
				SendMsg(m_szInputBuf);
				memset(m_szInputBuf, 0, sizeof(m_szInputBuf));
			}
		}

		ImGui::SameLine();

		if (ImGui::Button(u8"보내기", ImVec2(50, 25)))
		{
			if (strcmp("", m_szInputBuf))
			{
				SendMsg(m_szInputBuf);
				memset(m_szInputBuf, 0, sizeof(m_szInputBuf));
			}
		}
	}
	ImGui::End();
}

void CMyClient::Release()
{
	Disconnect();

	WSACleanup();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	m_pDeviceContext->Release();
	m_pDevice->Release();
}

void CMyClient::ThreadRecv(void* pData)
{
	char msg[1024] = { 0 };
	int len = sizeof(msg);

	while (0 < recv(m_sock, msg, len, 0))
	{
		// MsgType에 맞춰서 로직 진행
		char* pBuf = msg;
		MsgType* pType = (MsgType*)pBuf;
		pBuf += sizeof(MsgType);

		if (MsgType::LOGIN == *pType)
		{
			LoginProgress(pBuf);
		}
		else if (MsgType::CHATTING == *pType)
		{
			PACKETHEADER* pHeader = (PACKETHEADER*)pBuf;
			int iNameLen = pHeader->iNameLen;
			int iMsgLen = pHeader->iMsgLen;
			pBuf += sizeof(PACKETHEADER);

			UserInfo* pInfo = new USERINFO;
			memcpy(pInfo->szName, pBuf, iNameLen);
			memcpy(pInfo->szBuf, pBuf + iNameLen, iMsgLen);
			m_vecInfo.push_back(pInfo);
		}
		else if (MsgType::SIGNUP == *pType)
		{
			LOGIN_HEADER_TO_CLIENT* pHeader = (LOGIN_HEADER_TO_CLIENT*)pBuf;
			if (pHeader->bSuccess)
				cout << "가입 성공!\n";
			else
				cout << "가입 실패!\n";
		}
		else
		{
			DUPLICATE_HEADER* pHeader = (DUPLICATE_HEADER*)pBuf;
			if (pHeader->bSuccess)
			{
				if (DuType::ID == pHeader->eType)
					m_bIDCheck = true;
				else
					m_bNickCheck = true;
			}
			else
			{
				if (DuType::ID == pHeader->eType)
					m_bIDCheck = false;
				else
					m_bNickCheck = false;
			}
		}

		memset(msg, 0, len);
	}
}

void CMyClient::TextCenter(const char* str)
{
	auto windowWidth = ImGui::GetWindowSize().x;
	auto textWidth = ImGui::CalcTextSize(str).x;
	ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
	ImGui::Text(str);
}

void CMyClient::WindowCenter(const char* str)
{
	float size = ImGui::CalcTextSize(str).x;
	float avail = ImGui::GetContentRegionAvail().x;
	float off = (avail - size) * 0.5f;
	if (off > 0.0f)
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
}

void CMyClient::ConnectFail()
{
	ImGui::OpenPopup("ConnectFail");

	ImVec2 center2 = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center2, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(200.f, 90.f));
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	if (ImGui::BeginPopupModal("ConnectFail", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		TextCenter(u8"서버에 접속할 수 없습니다!");
		WindowCenter(u8"확인##2");

		if (ImGui::Button(u8"확인##2", ImVec2(50.f, 25.f)))
		{
			m_bConnectFail = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void CMyClient::JoinServer()
{
	if (E_FAIL == ConnectServer())
		m_bConnectFail = true;

	// Connect Server Failed
	if (m_bConnectFail)
	{
		ConnectFail();
	}
}

bool CMyClient::Alive()
{
	return m_bAlive;
}

void CMyClient::LoginWindow()
{
	if (ImGui::Begin(u8"로그인"))
	{
		if (ImGui::BeginChild(u8"세부창", ImVec2(400, 200)))
		{
			static char szID[256] = "";
			static char szPW[256] = "";
			ImGui::PushItemWidth(225.f);
			ImGui::Text("아이디 : ");
			ImGui::SameLine();
			ImGui::SetCursorPosX(82.f);
			ImGui::InputText("##1", szID, sizeof(szID));
			ImGui::Text("비밀번호 : ");
			ImGui::SameLine();
			ImGui::InputText("##2", szPW, sizeof(szPW));
			ImGui::PopItemWidth();
			

			if (ImGui::Button(u8"로그인", ImVec2(150.f, 50.f)))
			{
				BYTE* pBuf = new BYTE[sizeof(PACKET)];
				memset(pBuf, 0, sizeof(PACKET));
				int len = 0;

				MsgType* pType = (MsgType*)pBuf;
				*pType = MsgType::LOGIN;
				len += sizeof(MsgType);

				LOGIN_HEADER_TO_SERVER* pLogin = (LOGIN_HEADER_TO_SERVER*)(pBuf + len);
				pLogin->iIDLen = strlen(szID) + 1;
				pLogin->iPasswordLen = strlen(szPW) + 1;
				len += sizeof(LOGIN_HEADER_TO_SERVER);

				char* pMsg = (char*)(pBuf + len);
				memcpy(pMsg, szID, pLogin->iIDLen);
				pMsg += pLogin->iIDLen;
				memcpy(pMsg, szPW, pLogin->iPasswordLen);

				len += pLogin->iIDLen + pLogin->iPasswordLen;

				if (SOCKET_ERROR == send(m_sock, (char*)pBuf, len, 0))
					cout << "SEND FAILED\n";

				delete[] pBuf;
				memset(szID, 0, sizeof(szID));
				memset(szPW, 0, sizeof(szPW));
			}

			ImGui::SameLine();

			if (ImGui::Button(u8"회원가입", ImVec2(150.f, 50.f)))
				m_bSignup = true;

			ImGui::EndChild();
		}
	}
	ImGui::End();
}

void CMyClient::SignupProgress()
{
	ImGui::OpenPopup("Signup");

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Signup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text(u8"아이디 : ");
		ImGui::SameLine();
		ImGui::SetCursorPosX(90.f);
		ImGui::PushItemWidth(150.f);
		ImGui::InputText("##ID", m_szSignupID, IM_ARRAYSIZE(m_szSignupID));
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button(u8"중복확인##1", ImVec2(80.f, 25.f)) && strcmp(m_szSignupID, ""))
		{
			if (!DuplicateCheck(m_szSignupID, DuType::ID))
				cout << "아이디 중복체크 전송실패!\n";
		}
		ImGui::SameLine();
		ImVec4 vIDColor = ImVec4(1.f, 0.f, 0.f, 1.f);
		if(m_bIDCheck)
			vIDColor = ImVec4(0.f, 1.f, 0.f, 1.f);
		ImGui::PushStyleColor(ImGuiCol_Button, vIDColor);
		ImGui::Button("##IDColor", ImVec2(25.f, 25.f));
		ImGui::PopStyleColor();


		ImGui::Text(u8"비밀번호 : ");
		ImGui::SameLine();
		ImGui::PushItemWidth(150.f);
		ImGui::InputText("##PASS", m_szSignupPassword, IM_ARRAYSIZE(m_szSignupPassword));

		ImGui::Text(u8"닉네임 : ");
		ImGui::SameLine();
		ImGui::SetCursorPosX(90.f);
		ImGui::InputText("##NICK", m_szSignupNickName, IM_ARRAYSIZE(m_szSignupNickName));
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button(u8"중복확인##3", ImVec2(80.f, 25.f)) && strcmp(m_szSignupNickName, ""))
		{
			if (!DuplicateCheck(m_szSignupNickName, DuType::NICKNAME))
				cout << "닉네임 중복체크 전송실패!\n";
		}
		ImGui::SameLine();
		ImVec4 vNickColor = ImVec4(1.f, 0.f, 0.f, 1.f);
		if (m_bNickCheck)
			vNickColor = ImVec4(0.f, 1.f, 0.f, 1.f);
		ImGui::PushStyleColor(ImGuiCol_Button, vNickColor);
		ImGui::Button("##NickColor", ImVec2(25.f, 25.f));
		ImGui::PopStyleColor();


		WindowCenter(u8"생성##1");
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 55.f);
		if (ImGui::Button(u8"생성##1", ImVec2(80.f, 25.f)) && m_bIDCheck && m_bNickCheck)
		{
			if (SignupRequest(m_szSignupID, m_szSignupPassword, m_szSignupNickName))
				cout << "아이디 가입요청 전송실패!\n";

			m_bSignup = false;
			ImGui::CloseCurrentPopup();
		}
		
		ImGui::SameLine();

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 15.f);
		if (ImGui::Button(u8"취소##1", ImVec2(80.f, 25.f)))
		{
			m_bSignup = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void CMyClient::SendMsg(const char* szStr)
{
	BYTE* pBuf = new BYTE[sizeof(PACKET)];
	memset(pBuf, 0, sizeof(PACKET));
	int len = 0;

	MsgType* pType = (MsgType*)pBuf;
	*pType = MsgType::CHATTING;
	len += sizeof(MsgType);

	PACKETHEADER* pHeader = (PACKETHEADER*)(pBuf + len);
	pHeader->iNameLen = (int)strlen(m_szName) + 1;
	pHeader->iMsgLen = (int)strlen(szStr) + 1;
	len += sizeof(PACKETHEADER);

	char* pMsg = (char*)(pBuf + len);
	memcpy(pMsg, m_szName, pHeader->iNameLen);
	pMsg += pHeader->iNameLen;
	memcpy(pMsg, szStr, pHeader->iMsgLen);
	len += pHeader->iNameLen + pHeader->iMsgLen;

	send(m_sock, (char*)pBuf, len, 0);
	delete[] pBuf;
}

void CMyClient::LoginProgress(char* pBuffer)
{
	char* pBuf = pBuffer;
	LOGIN_HEADER_TO_CLIENT* pHeader = (LOGIN_HEADER_TO_CLIENT*)pBuf;

	if (pHeader->bSuccess)
	{
		int iNickNameLen = pHeader->iNickNameLen;
		pBuf += sizeof(LOGIN_HEADER_TO_CLIENT);
		memcpy(m_szName, pBuf, iNickNameLen);
		m_bChatting = true;
		m_bLogin = false;
	}
	else
	{
		// 로그인 실패
		cout << "LOGIN FAILED!!!\n";
	}
}

bool CMyClient::SignupRequest(const char* szID, const char* szPass, const char* szNick)
{
	BYTE* pBuf = new BYTE[sizeof(PACKET)];
	memset(pBuf, 0, sizeof(PACKET));
	int len = 0;

	MsgType* pType = (MsgType*)pBuf;
	*pType = MsgType::SIGNUP;
	len += sizeof(MsgType);

	SIGNUP_HEADER* pHeader = (SIGNUP_HEADER*)(pBuf + len);
	pHeader->iIDLen = (int)strlen(szID) + 1;
	pHeader->iPasswordLen = (int)strlen(szPass) + 1;
	pHeader->iNickNameLen = (int)strlen(szNick) + 1;
	len += sizeof(SIGNUP_HEADER);

	char* pMsg = (char*)(pBuf + len);
	memcpy(pMsg, szID, pHeader->iIDLen);
	pMsg += pHeader->iIDLen;
	memcpy(pMsg, szPass, pHeader->iPasswordLen);
	pMsg += pHeader->iPasswordLen;
	memcpy(pMsg, szNick, pHeader->iNickNameLen);
	len += pHeader->iIDLen + pHeader->iPasswordLen + pHeader->iNickNameLen;

	if (SOCKET_ERROR == send(m_sock, (char*)pBuf, len, 0))
	{
		delete[] pBuf;
		return false;
	}

	delete[] pBuf;
	return true;
}

bool CMyClient::DuplicateCheck(const char* str, DuType eType)
{
	BYTE* pBuf = new BYTE[sizeof(PACKET)];
	memset(pBuf, 0, sizeof(PACKET));
	int len = 0;

	MsgType* pType = (MsgType*)pBuf;
	*pType = MsgType::DUPLICATECHECK;
	len += sizeof(MsgType);

	DUPLICATE_HEADER* pHeader = (DUPLICATE_HEADER*)(pBuf + len);
	pHeader->eType = eType;
	pHeader->iNickNameLen = (int)strlen(str) + 1;
	len += sizeof(DUPLICATE_HEADER);

	char* pMsg = (char*)(pBuf + len);
	memcpy(pMsg, str, pHeader->iNickNameLen);
	len += pHeader->iNickNameLen;

	if (SOCKET_ERROR == send(m_sock, (char*)pBuf, len, 0))
	{
		delete[] pBuf;
		return false;
	}

	delete[] pBuf;
	return true;
}

void CMyClient::Disconnect()
{
	shutdown(m_sock, SD_BOTH);
	closesocket(m_sock);
	if (m_thread.joinable())
		m_thread.join();

	if (!m_vecInfo.empty())
	{
		for (int i = 0; i < m_vecInfo.size(); ++i)
			delete m_vecInfo[i];
		m_vecInfo.clear();
		vector<USERINFO*>().swap(m_vecInfo);
	}
}

