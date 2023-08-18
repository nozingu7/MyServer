#include "CMyServer.h"
#include "CMyDB.h"

CMyServer::CMyServer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:m_pDevice(pDevice), m_pDeviceContext(pContext)
{
	m_pDevice->AddRef();
	m_pDeviceContext->AddRef();
	memset(m_szIP, 0, sizeof(m_szIP));
	m_iPort = 15000;
	m_vecClient.reserve(100);

	m_pDB = new CMyDB;
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
		bResult = GetQueuedCompletionStatus(m_iocp, &dwSize,
			(PULONG_PTR)&userInfo, &overlap, INFINITE);

		if (bResult)
		{
			if (0 < dwSize)
			{
				MyOverlapped* myOverlap = (MyOverlapped*)overlap;

				if (RECV == myOverlap->state)
				{
					char* pBuf = userInfo->szRecvBuf;
					MsgType* pType = (MsgType*)pBuf;
					pBuf += sizeof(MsgType);

					if (MsgType::LOGIN == *pType)
					{
						#pragma region 로그인 처리
						LOGIN_HEADER_TO_SERVER* pHeader = (LOGIN_HEADER_TO_SERVER*)pBuf;
						int iIDLen = pHeader->iIDLen;
						int iPasswordLen = pHeader->iPasswordLen;
						char szID[20];
						char szPassword[20];

						pBuf += sizeof(LOGIN_HEADER_TO_SERVER);
						memcpy(szID, pBuf, iIDLen);
						pBuf += iIDLen;
						memcpy(szPassword, pBuf, iPasswordLen);

						// 로그인 정보 확인
						if (m_pDB->LoginDataCheck(szID, szPassword))
							LoginDataSend(szID, userInfo, true);
						else
							LoginDataSend(szID, userInfo, false);
						#pragma endregion
					}
					else if (MsgType::CHATTING == *pType)
					{
						#pragma region 메세지 전송
						// 모든 클라이언트에게 메세지 전송
						SendAll(*userInfo, (int)dwSize);
						#pragma endregion
					}
					else if (MsgType::SIGNUP == *pType)
					{
						#pragma region 회원가입 처리
						// 회원가입 처리
						bool bSuccess = false;

						if (SignupCheck(pBuf, *userInfo))
						{
							cout << "아이디 생성 성공!\n";
							bSuccess = true;
						}
						else
							cout << "아이디 생성 실패!\n";

						SignupDataSend(*userInfo, bSuccess);
						#pragma endregion
					}
					else
					{
						#pragma region 아이디, 닉네임 중복 처리
						// 아이디, 닉네임 중복 확인
						DUPLICATE_HEADER* pHeader = (DUPLICATE_HEADER*)pBuf;
						int iStrLen = pHeader->iNickNameLen;
						char szStr[20] = "";

						pBuf += sizeof(DUPLICATE_HEADER);
						memcpy(szStr, pBuf, pHeader->iNickNameLen);

						DuType eType = DuType::NICKNAME;
						if (DuType::ID == pHeader->eType)
							eType = DuType::ID;
						bool bSuccess = false;

						if (m_pDB->DBDubplicateCheck(szStr, eType))
						{
							cout << "중복 없음!\n";
							bSuccess = true;
						}
						else
							cout << "중복 있음!\n";

						DuplicateDataSend(*userInfo, bSuccess, eType);
						#pragma endregion
					}

					memset(userInfo->szRecvBuf, 0, sizeof(userInfo->szRecvBuf));

					dwSize = 0;
					dwFlag = 0;
					userInfo->RecvOverlap.wsaBuf.buf = userInfo->szRecvBuf;
					userInfo->RecvOverlap.wsaBuf.len = sizeof(userInfo->szRecvBuf);
					userInfo->RecvOverlap.state = RECV;

					if (WSARecv(userInfo->sock, &userInfo->RecvOverlap.wsaBuf, 1, &dwSize,
						&dwFlag, &userInfo->RecvOverlap.overlap, NULL))
					{
						if (WSAGetLastError() != WSA_IO_PENDING)
							cout << "Recv PENDING FAILED\n";
					}
				}
				else if (SEND == myOverlap->state)
				{
					// SEND
				}
			}
			else
			{
				cout << "클라이언트 접속 종료\n";
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

		cout << "Client Accept\n";
		userInfo = SetUpUserData(clientSock, clientAddr);
		if (nullptr == userInfo)
			continue;

		CreateIoCompletionPort((HANDLE)clientSock, m_iocp, (ULONG_PTR)userInfo, 0);

		userInfo->RecvOverlap.wsaBuf.buf = userInfo->szRecvBuf;
		userInfo->RecvOverlap.wsaBuf.len = sizeof(userInfo->szRecvBuf);
		userInfo->RecvOverlap.state = RECV;
		dwFlag = 0;
		dwSize = 0;

		WSARecv(clientSock, &userInfo->RecvOverlap.wsaBuf, 1, &dwSize, &dwFlag, &userInfo->RecvOverlap.overlap, NULL);
		if (WSAGetLastError() != WSA_IO_PENDING)
			cout << "Recv PENDING FAILED\n";
	}
}

HRESULT CMyServer::CreateServer(int iPort)
{
	if (0 != WSAStartup(MAKEWORD(2, 2), &m_wsaData))
	{
		cout << "WinSock Init Failed\n";
		return E_FAIL;
	}

	InitializeCriticalSection(&m_cs);

	// IOCP Create
	m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (0 == m_iocp)
	{
		cout << "IOCP Create Failed\n";
		return E_FAIL;
	}

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	m_bWorkerThread = true;

	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
		m_vecThreadWorker.push_back(thread(&CMyServer::ThreadWork, this, nullptr));

	// listen Socket Init Failed
	m_listenSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_listenSock)
	{
		cout << "LISTEN SOCKET INIT FAILED!\n";
		return E_FAIL;
	}

	// 소켓 구조체 초기화
	SOCKADDR_IN addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(iPort);

	// listen 소켓에 소켓 정보 바인드
	if (SOCKET_ERROR == bind(m_listenSock, (SOCKADDR*)&addr, sizeof(addr)))
	{
		cout << "BIND Failed!\n";
		return E_FAIL;
	}

	// 연결 대기 상태 준비
	if (SOCKET_ERROR == listen(m_listenSock, 5))
	{
		cout << "Listen Failed!\n";
		return E_FAIL;
	}

	// Accept Thread Create
	m_bAcceptThread = true;
	m_threadApt = thread(&CMyServer::ThreadAccept, this, nullptr);

	// Servers IP 기록
	GetIP();

	return S_OK;
}

HRESULT CMyServer::ReleaseServer()
{
	// Client Socket Release
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

	// WorkerThread Release
	m_bWorkerThread = false;
	CloseHandle(m_iocp);

	for (int i = 0; i < m_vecThreadWorker.size(); ++i)
	{
		if (m_vecThreadWorker[i].joinable())
			m_vecThreadWorker[i].join();
	}
	m_vecThreadWorker.clear();
	vector<thread>().swap(m_vecThreadWorker);

	// AcceptThread Release
	m_bAcceptThread = false;
	shutdown(m_listenSock, SD_BOTH);
	closesocket(m_listenSock);

	if (m_threadApt.joinable())
		m_threadApt.join();

	delete m_pDB;

	// Critical_Section Delete
	DeleteCriticalSection(&m_cs);
	WSACleanup();

	// ImGui Release
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// D3D Release
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

USERINFO* CMyServer::SetUpUserData(SOCKET sock, SOCKADDR_IN addr)
{
	char msg[20] = { 0 };
	//recv(sock, msg, sizeof(msg), 0);

	USERINFO* userInfo = new USERINFO;
	ZeroMemory(userInfo, sizeof(USERINFO));
	userInfo->sock = sock;
	strcpy(userInfo->szIP, inet_ntoa(addr.sin_addr));
	userInfo->iPort = ntohs(addr.sin_port);
	strcpy(userInfo->szName, UTF8ToMultiByte(msg));

	system_clock::time_point now = system_clock::now();
	time_point<system_clock, days> dp = floor<days>(now);
	userInfo->date = year_month_day(dp);
	userInfo->time = hh_mm_ss<milliseconds>{ floor<milliseconds>(now - dp) };

	return userInfo;
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
	char* pBuf = userInfo.szRecvBuf;
	int len = 0;
	MsgType* pType = (MsgType*)pBuf;
	len += sizeof(MsgType);

	PACKETHEADER* pHeader = (PACKETHEADER*)(pBuf + len);
	char szName[20] = { 0 };
	char szMsg[256] = { 0 };
	len += sizeof(PACKETHEADER);
	memcpy(szName, pBuf + len, pHeader->iNameLen);
	len += pHeader->iNameLen;
	memcpy(szMsg, pBuf + len, pHeader->iMsgLen);

	strcpy(szName, UTF8ToMultiByte(szName));
	strcpy(szMsg, UTF8ToMultiByte(szMsg));

	cout << "Message - " << szName << " : " << szMsg << '\n';

	memcpy(userInfo.szSendBuf, userInfo.szRecvBuf, sizeof(userInfo.szRecvBuf));
	userInfo.SendOverlap.wsaBuf.buf = userInfo.szSendBuf;
	userInfo.SendOverlap.wsaBuf.len = iSize;
	userInfo.SendOverlap.state = SEND;

	DWORD dwByte = 0;
	DWORD dwFlag = 0;

	EnterCriticalSection(&m_cs);
	for (int i = 0; i < (int)m_vecClient.size(); ++i)
	{
		if (WSASend(m_vecClient[i]->sock, &userInfo.SendOverlap.wsaBuf, 1,
			&dwByte, dwFlag, &userInfo.SendOverlap.overlap, NULL))
		{
			if (WSA_IO_PENDING != WSAGetLastError())
				cout << "SEND PENDING FAILED\n";
		}
	}
	LeaveCriticalSection(&m_cs);

	return true;
}

bool CMyServer::SignupCheck(char* szMsg, const USERINFO& info)
{
	char* pBuf = szMsg;
	SIGNUP_HEADER* pHeader = (SIGNUP_HEADER*)pBuf;
	int iIDLen = pHeader->iIDLen;
	int iPassLen = pHeader->iPasswordLen;
	int iNickLen = pHeader->iNickNameLen;
	char szID[20] = "";
	char szPass[20] = "";
	char szNick[20] = "";

	pBuf += sizeof(SIGNUP_HEADER);
	memcpy(szID, pBuf, pHeader->iIDLen);
	pBuf += pHeader->iIDLen;
	memcpy(szPass, pBuf, pHeader->iPasswordLen);
	pBuf += pHeader->iPasswordLen;
	memcpy(szNick, pBuf, pHeader->iNickNameLen);

	return m_pDB->SignUp(szID, szPass, szNick, info.szIP, info.iPort);
}

bool CMyServer::DuplicateCheack(char* szMsg)
{
	char* pBuf = szMsg;
	DUPLICATE_HEADER* pHeader = (DUPLICATE_HEADER*)pBuf;
	DuType eType = pHeader->eType;
	int iStrLen = pHeader->iNickNameLen;
	char szStr[20];

	pBuf += sizeof(DUPLICATE_HEADER);
	memcpy(szStr, pBuf, pHeader->iNickNameLen);

	//return m_pDB->DBDubplicateCheck(szStr);
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

void CMyServer::LoginDataSend(const char* szID, USERINFO* userInfo, bool bState)
{
	BYTE* pLoginBuf = new BYTE[sizeof(PACKET)];
	memset(pLoginBuf, 0, sizeof(PACKET));
	int len = 0;

	MsgType* pType = (MsgType*)pLoginBuf;
	*pType = MsgType::LOGIN;
	len += sizeof(MsgType);

	LOGIN_HEADER_TO_CLIENT* pHeaderToClient = (LoginHeaderToClient*)(pLoginBuf + len);
	pHeaderToClient->bSuccess = bState;

	if (bState)
	{
		char szNickName[20] = "";
		strcpy(szNickName, m_pDB->GetNickName(szID));
		pHeaderToClient->iNickNameLen = strlen(szNickName) + 1;
		len += sizeof(LOGIN_HEADER_TO_CLIENT);

		char* pMsg = (char*)(pLoginBuf + len);
		memcpy(pMsg, szNickName, pHeaderToClient->iNickNameLen);
		len += pHeaderToClient->iNickNameLen;

		EnterCriticalSection(&m_cs);
		strcpy(userInfo->szName, szNickName);
		m_vecClient.push_back(userInfo);
		LeaveCriticalSection(&m_cs);
	}

	userInfo->SendOverlap.wsaBuf.buf = (char*)pLoginBuf;
	userInfo->SendOverlap.wsaBuf.len = len;
	userInfo->SendOverlap.state = SEND;

	DWORD dwByte = 0;
	DWORD dwFlag = 0;

	if (WSASend(userInfo->sock, &userInfo->SendOverlap.wsaBuf, 1,
		&dwByte, dwFlag, &userInfo->SendOverlap.overlap, NULL))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
			cout << "SEND PENDING FAILED\n";
	}

	delete[] pLoginBuf;
}

void CMyServer::SignupDataSend(USERINFO& userInfo, bool bState)
{
	BYTE* pSignBuf = new BYTE[sizeof(PACKET)];
	memset(pSignBuf, 0, sizeof(PACKET));
	int len = 0;

	MsgType* pType = (MsgType*)pSignBuf;
	*pType = MsgType::SIGNUP;
	len += sizeof(MsgType);

	LOGIN_HEADER_TO_CLIENT* pHeaderToClient = (LOGIN_HEADER_TO_CLIENT*)(pSignBuf + len);
	pHeaderToClient->bSuccess = bState;
	len += sizeof(LOGIN_HEADER_TO_CLIENT);

	userInfo.SendOverlap.wsaBuf.buf = (char*)pSignBuf;
	userInfo.SendOverlap.wsaBuf.len = len;
	userInfo.SendOverlap.state = SEND;

	DWORD dwByte = 0;
	DWORD dwFlag = 0;

	if (WSASend(userInfo.sock, &userInfo.SendOverlap.wsaBuf, 1,
		&dwByte, dwFlag, &userInfo.SendOverlap.overlap, NULL))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
			cout << "SEND PENDING FAILED\n";
	}

	delete[] pSignBuf;
}

void CMyServer::DuplicateDataSend(USERINFO& userInfo, bool bState, DuType etype)
{
	BYTE* pDuBuf = new BYTE[sizeof(PACKET)];
	memset(pDuBuf, 0, sizeof(PACKET));
	int len = 0;

	MsgType* pType = (MsgType*)pDuBuf;
	*pType = MsgType::DUPLICATECHECK;
	len += sizeof(MsgType);
	DUPLICATE_HEADER* pDuHeader = (DUPLICATE_HEADER*)(pDuBuf + len);
	pDuHeader->eType = etype;
	pDuHeader->bSuccess = bState;
	len += sizeof(DUPLICATE_HEADER);

	userInfo.SendOverlap.wsaBuf.buf = (char*)pDuBuf;
	userInfo.SendOverlap.wsaBuf.len = len;
	userInfo.SendOverlap.state = SEND;

	DWORD dwByte = 0;
	DWORD dwFlag = 0;

	if (WSASend(userInfo.sock, &userInfo.SendOverlap.wsaBuf, 1,
		&dwByte, dwFlag, &userInfo.SendOverlap.overlap, NULL))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
			cout << "SEND PENDING FAILED\n";
	}

	delete[] pDuBuf;
}

void CMyServer::Render()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (ImGui::Begin("MyServer"))
	{
		int iFlag = 0;
		float fDragSpeed = 1.f;
		// 서버 가동시 비활성화
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
				cout << "서버 가동 시작\n";
			}
		}
		ImGui::SameLine();
		if (ImGui::Button((const char*)u8"서버 종료", ImVec2(150.f, 50.f)))
		{
			cout << "서버 가동 종료\n";
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

