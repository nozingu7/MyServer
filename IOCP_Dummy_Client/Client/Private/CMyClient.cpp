#include "CMyClient.h"

CMyClient::CMyClient(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:m_pDevice(pDevice), m_pDeviceContext(pContext)
{
	m_pDevice->AddRef();
	m_pDeviceContext->AddRef();
	memset(m_szName, 0, sizeof(m_szName));
	memset(m_szInputBuf, 0, sizeof(m_szInputBuf));
	m_sock = 0;
	m_bConnectEnable = false;
	m_vecDummy.reserve(100);
	m_iDummySize = 100;

	/*m_vecChat.push_back("안녕하세요");
	m_vecChat.push_back("식사하셨나요?");
	m_vecChat.push_back("반갑습니다!");
	m_vecChat.push_back("안녕히계세요");
	m_vecChat.push_back("Hello?");
	m_vecChat.push_back("Everybody Jump!!!");
	m_vecChat.push_back("내가 누구게?");
	m_vecChat.push_back("저는 더미 클라이언트입니다^^");
	m_vecChat.push_back("박성호 파이팅~");
	m_vecChat.push_back("Shit the Fuck!!!!!!");*/


	m_vecChat.push_back("HI");
	m_vecChat.push_back("HELLO");
	m_vecChat.push_back("DEATH");
	m_vecChat.push_back("IVE");
	m_vecChat.push_back("KINGKONG");
	m_vecChat.push_back("nozingu");
	m_vecChat.push_back("ParkSungHo");
	m_vecChat.push_back("Show Me The Money");
	m_vecChat.push_back("TEKKEN8");
	m_vecChat.push_back("STREET FIGHTER6");

	srand((unsigned int)time(NULL));
}

CMyClient::~CMyClient()
{
	Release();
}

HRESULT CMyClient::ConnectServer(const char* szName)
{
	WSAData wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata))
	{
		cout << "윈속 초기화 실패!\n";
		return E_FAIL;
	}

	for (int i = 0; i < m_iDummySize; ++i)
	{
		Dummy* pDummy = new Dummy;
		pDummy->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		SOCKADDR_IN addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
		addr.sin_port = htons(15000);

		if (SOCKET_ERROR == connect(pDummy->sock, (SOCKADDR*)&addr, sizeof(addr)))
		{
			cout << "서버에 연결할 수 없습니다!\n";
			return E_FAIL;
		}
		
		char Name[30] = { 0 };
		strcpy(Name, szName);
		strcat(Name, to_string(i + 1).c_str());

		send(pDummy->sock, Name, (int)strlen(Name) + 1, 0);

		strcpy(pDummy->szName, Name);
		m_vecDummy.push_back(pDummy);

		m_bConnectEnable = true;

		m_vecThread.push_back(thread(&CMyClient::ThreadRecv, this, &pDummy->sock));
	}

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

	// 나중에 버튼 비활성화 되더라도 투명도 그대로 유지하려고 설정
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

void CMyClient::Render(double TimeDelta)
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (ImGui::Begin("MENU"))
	{
		// 현재 서버에 접속한 상태가 아닐때만 활성화
		ImGui::BeginDisabled(m_bConnectEnable);
		JoinServer();
		ImGui::EndDisabled();

		ImGui::SameLine();

		if (ImGui::Button(u8"서버 종료", ImVec2(150.f, 50.f)))
		{
			cout << "서버 종료 실행\n";
			m_bAlive = false;
			m_bConnectEnable = false;
		}
	}
	ImGui::End();

	// 채팅 GUI Render
	if (m_bConnectEnable)
		ShowChat(TimeDelta);

	ImGui::Render();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO(); (void)io;

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void CMyClient::ShowChat(double TimeDelta)
{
	if (ImGui::Begin(u8"채팅"))
	{
		ImGui::BeginChild("Chatting", ImVec2(0, -25), true, 0);

		int iSize = (int)m_vecInfo.size();
		for (int i = 0; i < iSize; i++)
			ImGui::Text("%s : %s", m_vecInfo[i]->szName, m_vecInfo[i]->szBuf);

		ImGui::EndChild();

		m_ChatTime += TimeDelta;

		if (3.0 < m_ChatTime)
		{
			m_ChatTime = 0.0;
			AutoChat();
		}
	}
	ImGui::End();
}

void CMyClient::Release()
{
	for (int i = 0; i < (int)m_vecDummy.size(); ++i)
	{
		shutdown(m_vecDummy[i]->sock, SD_BOTH);
		closesocket(m_vecDummy[i]->sock);
		delete m_vecDummy[i];
	}

	for (int i = 0; i < (int)m_vecThread.size(); ++i)
	{
		if (m_vecThread[i].joinable())
			m_vecThread[i].join();
	}

	for (int i = 0; i < m_vecInfo.size(); ++i)
		delete m_vecInfo[i];
	m_vecInfo.clear();
	vector<USERINFO*>().swap(m_vecInfo);

	WSACleanup();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	m_pDeviceContext->Release();
	m_pDevice->Release();
}

void CMyClient::ThreadRecv(void* pData)
{
	SOCKET* sock = (SOCKET*)pData;
	char msg[1024] = { 0 };
	int len = sizeof(msg);
	int iCode = 0;

	while (0 < (iCode = recv(*sock, msg, len, 0)))
	{
		/*PACKETHEADER* pHeader = (PACKETHEADER*)msg;
		int iNameLen = pHeader->iNameLen;
		int iMsgLen = pHeader->iMsgLen;
		char* pBuf = msg + sizeof(PACKETHEADER);

		UserInfo* pInfo = new USERINFO;
		memcpy(pInfo->szName, pBuf, iNameLen);
		memcpy(pInfo->szBuf, pBuf + iNameLen, iMsgLen);
		m_vecInfo.push_back(pInfo);
		memset(msg, 0, len);*/


		/*char* pBuf = msg;
		PACKETHEADER* pHeader = (PACKETHEADER*)pBuf;
		char szName[20] = { 0 };
		char szMsg[256] = { 0 };
		memcpy(szName, pBuf + sizeof(PACKETHEADER), pHeader->iNameLen);
		pBuf += sizeof(PACKETHEADER) + pHeader->iNameLen;
		memcpy(szMsg, pBuf, pHeader->iMsgLen);

		UserInfo* pInfo = new USERINFO;
		memcpy(pInfo->szName, szName, pHeader->iNameLen);
		memcpy(pInfo->szBuf, pBuf + pHeader->iNameLen, pHeader->iMsgLen);
		m_vecInfo.push_back(pInfo);
		memset(msg, 0, len);*/
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
		TextCenter(u8"채팅서버 입장 실패!");
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
	if (ImGui::Button(u8"채팅서버 입장", ImVec2(150.f, 50.f)))
		ImGui::OpenPopup("NickName");

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("NickName", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		TextCenter(u8"닉네임을 설정해주세요!");
		if (ImGui::InputText("##", m_szName, IM_ARRAYSIZE(m_szName), ImGuiInputTextFlags_EnterReturnsTrue))
			m_bCheck = true;

		WindowCenter(u8"확인##1");
		if (ImGui::Button(u8"확인##1", ImVec2(50.f, 25.f)) || m_bCheck)
		{
			// Connect 함수 호출
			if (S_OK == ConnectServer(m_szName))
			{
				ImGui::CloseCurrentPopup();
			}
			else
			{
				ImGui::CloseCurrentPopup();
				m_bConnectFail = true;
			}
			m_bCheck = false;
		}
		ImGui::EndPopup();
	}

	// 연결 실패 팝업창
	if (m_bConnectFail)
	{
		ConnectFail();
	}
}

bool CMyClient::Alive()
{
	return m_bAlive;
}

void CMyClient::AutoChat()
{
	for (int i = 0; i < (int)m_vecDummy.size(); ++i)
	{
		int num = rand() % 100;
		if (50 <= num)
		{
			int idx = num % 10;

			BYTE* pBuf = new BYTE[sizeof(PACKET)];
			memset(pBuf, 0, sizeof(PACKET));

			PACKETHEADER* pHeader = (PACKETHEADER*)pBuf;
			pHeader->iNameLen = (int)strlen(m_vecDummy[i]->szName) + 1;
			pHeader->iMsgLen = (int)strlen(m_vecChat[idx]) + 1;

			char* pMsg = (char*)(pBuf + sizeof(PACKETHEADER));
			memcpy(pMsg, m_vecDummy[i]->szName, pHeader->iNameLen);
			pMsg += pHeader->iNameLen;
			memcpy(pMsg, m_vecChat[idx], pHeader->iMsgLen);

			int len = sizeof(PACKETHEADER) + pHeader->iNameLen + pHeader->iMsgLen;
			int iResult = send(m_vecDummy[i]->sock, (char*)pBuf, len, 0);
			if (-1 == iResult)
			{
				cout << "전송 실패 -- Error : " << GetLastError() << '\n';
			}
			else
			{
				cout << "전송 성공 -- 전송 바이트 : " << iResult << '\n';
			}

			delete[] pBuf;
		}
	}
}
