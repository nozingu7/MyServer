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

	send(m_sock, szName, (int)strlen(szName) + 1, 0);

	m_bConnectEnable = true;

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
		JoinServer();
		ImGui::EndDisabled();

		ImGui::SameLine();

		if (ImGui::Button(u8"서버 종료", ImVec2(150.f, 50.f)))
		{
			cout << "접속 종료\n";
			m_bAlive = false;
			m_bConnectEnable = false;
		}
	}
	ImGui::End();

	// Client GUI Render
	if (m_bConnectEnable)
		ShowChat();

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
				BYTE* pBuf = new BYTE[sizeof(PACKET)];
				memset(pBuf, 0, sizeof(PACKET));

				PACKETHEADER* pHeader = (PACKETHEADER*)pBuf;
				pHeader->iNameLen = (int)strlen(m_szName) + 1;
				pHeader->iMsgLen = (int)strlen(m_szInputBuf) + 1;

				char* pMsg = (char*)(pBuf + sizeof(PACKETHEADER));
				memcpy(pMsg, m_szName, pHeader->iNameLen);
				pMsg += pHeader->iNameLen;
				memcpy(pMsg, m_szInputBuf, pHeader->iMsgLen);

				int len = sizeof(PACKETHEADER) + pHeader->iNameLen + pHeader->iMsgLen;
				send(m_sock, (char*)pBuf, len, 0);
				delete[] pBuf;
				memset(m_szInputBuf, 0, sizeof(m_szInputBuf));
			}
		}

		ImGui::SameLine();

		if (ImGui::Button(u8"보내기", ImVec2(50, 25)))
		{
			if (strcmp("", m_szInputBuf))
			{
				BYTE* pBuf = new BYTE[sizeof(PACKET)];
				memset(pBuf, 0, sizeof(PACKET));

				PACKETHEADER* pHeader = (PACKETHEADER*)pBuf;
				pHeader->iNameLen = (int)strlen(m_szName) + 1;
				pHeader->iMsgLen = (int)strlen(m_szInputBuf) + 1;

				char* pMsg = (char*)(pBuf + sizeof(PACKETHEADER));
				memcpy(pMsg, m_szName, pHeader->iNameLen);
				pMsg += pHeader->iNameLen;
				memcpy(pMsg, m_szInputBuf, pHeader->iMsgLen);

				int len = sizeof(PACKETHEADER) + pHeader->iNameLen + pHeader->iMsgLen;
				send(m_sock, (char*)pBuf, len, 0);
				delete[] pBuf;
				memset(m_szInputBuf, 0, sizeof(m_szInputBuf));
			}
		}
	}
	ImGui::End();
}

void CMyClient::Release()
{
	shutdown(m_sock, SD_BOTH);
	closesocket(m_sock);
	if (m_thread.joinable())
		m_thread.join();

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
	char msg[1024] = { 0 };
	int len = sizeof(msg);

	while (0 < recv(m_sock, msg, len, 0))
	{
		PACKETHEADER* pHeader = (PACKETHEADER*)msg;
		int iNameLen = pHeader->iNameLen;
		int iMsgLen = pHeader->iMsgLen;
		char* pBuf = msg + sizeof(PACKETHEADER);

		UserInfo* pInfo = new USERINFO;
		memcpy(pInfo->szName, pBuf, iNameLen);
		memcpy(pInfo->szBuf, pBuf + iNameLen, iMsgLen);
		m_vecInfo.push_back(pInfo);
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
	if (ImGui::Button(u8"서버 접속", ImVec2(150.f, 50.f)))
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
			// Connect Server
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
