#include "CMyServer.h"

CMyServer::CMyServer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:m_pDevice(pDevice), m_pDeviceContext(pContext)
{
	m_pDevice->AddRef();
	m_pDeviceContext->AddRef();
	memset(m_szIP, 0, sizeof(m_szIP));
	m_iPort = 15000;
	m_vecClient.reserve(100);
}

CMyServer::~CMyServer()
{
	ReleaseServer();
}

void CMyServer::ThreadWork(void* pData)
{
	DWORD dwSize = 0;
	DWORD dwFlag = 0;
	USERINFO* userInfo = nullptr;
	OVERLAPPED* overlap = nullptr;
	bool bResult = false;

	while (m_bWorkerThread)
	{
		bResult = GetQueuedCompletionStatus(m_iocp, &dwSize, (PULONG_PTR)&userInfo, &overlap, INFINITE);

		if (bResult)
		{
			if (0 < dwSize)
			{
				MyOverlapped* myOverlap = (MyOverlapped*)overlap;

				if (RECV == myOverlap->state)
				{
					SendAll(*userInfo, (int)dwSize);
					memset(userInfo->szRecvBuf, 0, sizeof(userInfo->szRecvBuf));

					dwSize = 0;
					dwFlag = 0;
					userInfo->RecvOverlap.wsaBuf.buf = userInfo->szRecvBuf;
					userInfo->RecvOverlap.wsaBuf.len = sizeof(userInfo->szRecvBuf);
					userInfo->RecvOverlap.state = RECV;

					if (WSARecv(userInfo->sock, &userInfo->RecvOverlap.wsaBuf, 1, &dwSize, &dwFlag, &userInfo->RecvOverlap.overlap, NULL))
					{
						if (WSAGetLastError() != WSA_IO_PENDING)
							cout << "Recv PENDING 실패\n";
					}
				}
				else if (SEND == myOverlap->state)
				{
					char* pBuf = userInfo->szSendBuf;
					PACKETHEADER* pHeader = (PACKETHEADER*)pBuf;
					char szName[20] = { 0 };
					char szMsg[256] = { 0 };
					memcpy(szName, pBuf + sizeof(PACKETHEADER), pHeader->iNameLen);
					pBuf += sizeof(PACKETHEADER) + pHeader->iNameLen;
					memcpy(szMsg, pBuf, pHeader->iMsgLen);

					strcpy(szName, UTF8ToMultiByte(szName));
					strcpy(szMsg, UTF8ToMultiByte(szMsg));

					cout << "Message - " << szName << " : " << szMsg << '\n';
				}
			}
			else
			{
				cout << "클라이언트 연결 해제\n";
				RemoveClient(userInfo->sock);
			}
		}
		else
		{
			cout << "비정상 종료\n";
			break;
		}
	}
}

void CMyServer::ThreadAccept(void* pData)
{
	SOCKET clientSock = 0;
	SOCKADDR_IN clientAddr = { 0 };
	int addrlen = sizeof(clientAddr);
	USERINFO* userInfo = nullptr;
	WSABUF wsaBuf = { 0 };
	DWORD dwSize = 0;
	DWORD dwFlag = 0;
	bool bResult = false;

	while (m_bAcceptThread)
	{
		if (INVALID_SOCKET == (clientSock = accept(m_listenSock, (SOCKADDR*)&clientAddr, &addrlen)))
			continue;

		cout << "클라이언트 접속\n";

		char msg[20] = { 0 };
		recv(clientSock, msg, sizeof(msg), 0);

		userInfo = new USERINFO;
		ZeroMemory(userInfo, sizeof(USERINFO));
		userInfo->sock = clientSock;
		strcpy(userInfo->szIP, inet_ntoa(clientAddr.sin_addr));
		userInfo->iPort = ntohs(clientAddr.sin_port);
		strcpy(userInfo->szName, UTF8ToMultiByte(msg));

		system_clock::time_point now = system_clock::now();
		time_point<system_clock, days> dp = floor<days>(now);
		userInfo->date = year_month_day(dp);
		userInfo->time = hh_mm_ss<milliseconds>{ floor<milliseconds>(now - dp) };

		EnterCriticalSection(&m_cs);
		m_vecClient.push_back(userInfo);
		LeaveCriticalSection(&m_cs);

		CreateIoCompletionPort((HANDLE)clientSock, m_iocp, (ULONG_PTR)userInfo, 0);

		userInfo->RecvOverlap.wsaBuf.buf = userInfo->szRecvBuf;
		userInfo->RecvOverlap.wsaBuf.len = sizeof(userInfo->szRecvBuf);
		userInfo->RecvOverlap.state = RECV;
		dwFlag = 0;
		dwSize = 0;

		WSARecv(clientSock, &userInfo->RecvOverlap.wsaBuf, 1, &dwSize, &dwFlag, &userInfo->RecvOverlap.overlap, NULL);
		if (WSAGetLastError() != WSA_IO_PENDING)
			cout << "Recv PENDING 실패\n";
	}
}

HRESULT CMyServer::CreateServer(int iPort)
{
	if (0 != WSAStartup(MAKEWORD(2, 2), &m_wsaData))
	{
		cout << "서버 생성 실패\n";
		return E_FAIL;
	}

	InitializeCriticalSection(&m_cs);

	// IOCP 생성
	m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (0 == m_iocp)
	{
		cout << "IOCP 생성 실패\n";
		return E_FAIL;
	}

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	m_bWorkerThread = true;

	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
		m_vecThreadWorker.push_back(thread(&CMyServer::ThreadWork, this, nullptr));

	// listen 소켓 생성
	m_listenSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_listenSock)
	{
		cout << "LISTEN SOCKET 생성 실패!\n";
		return E_FAIL;
	}

	// 소켓 구조체 설정
	SOCKADDR_IN addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(iPort);

	// 바인드
	if (SOCKET_ERROR == bind(m_listenSock, (SOCKADDR*)&addr, sizeof(addr)))
	{
		cout << "BIND 실패!\n";
		return E_FAIL;
	}

	// 연결 대기 상태
	if (SOCKET_ERROR == listen(m_listenSock, 5))
	{
		cout << "연결 대기 실패!\n";
		return E_FAIL;
	}

	// 연결 요청 쓰레드 가동
	m_bAcceptThread = true;
	m_threadApt = thread(&CMyServer::ThreadAccept, this, nullptr);

	// IP 얻어오기
	GetIP();

	return S_OK;
}

HRESULT CMyServer::ReleaseServer()
{
	// 클라이언트 접속 해제
	for (int i = 0; i < m_vecClient.size(); ++i)
	{
		EnterCriticalSection(&m_cs);
		shutdown(m_vecClient[i]->sock, SD_BOTH);
		closesocket(m_vecClient[i]->sock);
		LeaveCriticalSection(&m_cs);
		delete m_vecClient[i];
	}
	m_vecClient.clear();
	vector<USERINFO*>().swap(m_vecClient);

	// WorkerThread 종료
	m_bWorkerThread = false;
	CloseHandle(m_iocp);

	for (int i = 0; i < m_vecThreadWorker.size(); ++i)
		m_vecThreadWorker[i].join();
	m_vecThreadWorker.clear();
	vector<thread>().swap(m_vecThreadWorker);

	// AcceptThread 종료
	m_bAcceptThread = false;
	shutdown(m_listenSock, SD_BOTH);

	closesocket(m_listenSock);
	if (m_threadApt.joinable())
		m_threadApt.join();

	// Critical_Section 해제
	DeleteCriticalSection(&m_cs);
	WSACleanup();

	// ImGui 해제
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// D3D 해제
	m_pDeviceContext->Release();
	m_pDevice->Release();

	return S_OK;
}

HRESULT CMyServer::RemoveClient(SOCKET sock)
{
	EnterCriticalSection(&m_cs);
	for (auto iter = m_vecClient.begin(); iter != m_vecClient.end(); ++iter)
	{
		if (sock == (*iter)->sock)
		{
			shutdown((*iter)->sock, SD_BOTH);
			closesocket((*iter)->sock);
			delete (*iter);
			m_vecClient.erase(iter);
			break;
		}
	}
	LeaveCriticalSection(&m_cs);

	return S_OK;
}

void CMyServer::Init_Imgui()
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
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(g_hWnd);
	ImGui_ImplDX11_Init(m_pDevice, m_pDeviceContext);
}

bool CMyServer::SendAll(USERINFO& userInfo, int iSize)
{
	memcpy(userInfo.szSendBuf, userInfo.szRecvBuf, sizeof(userInfo.szRecvBuf));
	userInfo.SendOverlap.wsaBuf.buf = userInfo.szSendBuf;
	userInfo.SendOverlap.wsaBuf.len = iSize;
	userInfo.SendOverlap.state = SEND;

	DWORD dwByte = 0;
	DWORD dwFlag = 0;

	auto iter = m_vecClient.begin();

	EnterCriticalSection(&m_cs);
	for (int i = 0; i < (int)m_vecClient.size(); ++i)
	{
		if (WSASend(m_vecClient[i]->sock, &userInfo.SendOverlap.wsaBuf, 1, &dwByte, dwFlag, &userInfo.SendOverlap.overlap, NULL))
		{
			if (WSA_IO_PENDING != WSAGetLastError())
				cout << "SEND PENDING 실패\n";
		}
	}
	LeaveCriticalSection(&m_cs);

	return true;
}

void CMyServer::MainWindow()
{
	if (ImGui::Begin((const char*)u8"유저 정보창"))
	{
		if (ImGui::BeginTable("split", 4, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders))
		{
			ImGui::TableNextColumn();
			ImGui::Text("Time");
			ImGui::TableNextColumn();
			ImGui::Text("IP");
			ImGui::TableNextColumn();
			ImGui::Text("Port");
			ImGui::TableNextColumn();
			ImGui::Text("Name");
			ImGui::TableNextRow();

			for (int i = 0; i < m_vecClient.size(); ++i)
			{
				ImGui::TableNextColumn();
				ImGui::Text(format("{:%Y-%m-%d}", m_vecClient[i]->date).c_str());
				ImGui::SameLine();
				ImGui::Text("%02d:%02d:%02d",
					m_vecClient[i]->time.hours().count() + 9,
					m_vecClient[i]->time.minutes().count(),
					m_vecClient[i]->time.seconds().count());
				ImGui::TableNextColumn();
				ImGui::Text(m_vecClient[i]->szIP);
				ImGui::TableNextColumn();
				ImGui::Text("%d", m_vecClient[i]->iPort);
				ImGui::TableNextColumn();
				ImGui::Text(MultiByteToUTF8(m_vecClient[i]->szName));
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

bool CMyServer::isAlive()
{
	return m_bAlive;
}

void CMyServer::GetIP()
{
	char szHostName[MAX_PATH] = { 0 };
	if (SOCKET_ERROR != gethostname(szHostName, MAX_PATH))
	{
		HOSTENT* pHostent = gethostbyname(szHostName);
		while (nullptr != pHostent)
		{
			if (pHostent->h_addrtype == AF_INET)
			{
				strcpy(m_szIP, inet_ntoa(*(IN_ADDR*)pHostent->h_addr_list[0]));
				break;
			}
			pHostent++;
		}
	}
}

char* CMyServer::UTF8ToMultiByte(char* msg)
{
	char szMsg[128] = { 0 };

	wchar_t strUnicode[256] = { 0, };
	int nLen = (int)MultiByteToWideChar(CP_UTF8, 0, msg, (int)strlen(msg), NULL, NULL);
	MultiByteToWideChar(CP_UTF8, 0, msg, (int)strlen(msg), strUnicode, nLen);

	int len = WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, szMsg, len, NULL, NULL);

	return szMsg;
}

char* CMyServer::MultiByteToUTF8(char* msg)
{
	wchar_t strUnicode[256] = { 0, };
	int nLen = MultiByteToWideChar(CP_ACP, 0, msg, (int)strlen(msg), NULL, NULL);
	MultiByteToWideChar(CP_ACP, 0, msg, (int)strlen(msg), strUnicode, nLen);

	char strUtf8[256] = { 0, };
	int nLen2 = WideCharToMultiByte(CP_UTF8, 0, strUnicode, lstrlenW(strUnicode), NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, strUnicode, lstrlenW(strUnicode), strUtf8, nLen2, NULL, NULL);

	return strUtf8;
}

void CMyServer::Render()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	for (int i = 0; i < (int)m_vecThreadWorker.size(); ++i)
	{
		if (false == m_vecThreadWorker[i].joinable())
		{
			cout << i << "번 쓰레드 정지..\n";
		}
	}

	if (ImGui::Begin("MyServer"))
	{
		int iFlag = 0;
		float fDragSpeed = 1.f;
		// 서버가 가동시 포트번호 변경 X
		if (m_bRun)
		{
			iFlag = ImGuiSliderFlags_NoInput;
			fDragSpeed = 0.f;
		}
		else
			iFlag = ImGuiSliderFlags_None;
		ImGui::BeginChild("##", ImVec2(500, 300));
		ImGui::PushItemWidth(100);
		ImGui::Text("IP : %s", m_szIP);
		ImGui::Text("Port : ");
		ImGui::SameLine();
		ImGui::DragInt("##", &m_iPort, fDragSpeed, 0, 0, "%d", iFlag);
		ImGui::PopItemWidth();

		if (ImGui::Button((const char*)u8"서버 가동", ImVec2(150.f, 50.f)))
		{
			if (S_OK == CreateServer(m_iPort))
			{
				m_bRun = true;
				cout << "서버 가동 실행\n";
			}
		}
		ImGui::SameLine();
		if (ImGui::Button((const char*)u8"서버 종료", ImVec2(150.f, 50.f)))
		{
			cout << "서버 종료 실행\n";
			m_bRun = false;
			m_bAlive = false;
		}

		// 유저 정보창
		if (m_bRun)
			MainWindow();

		ImGui::EndChild();
	}
	ImGui::End();

	ImGui::Render();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	// Update and Render additional Platform Windows
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

