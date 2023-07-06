#include "CMyServer.h"

CMyServer::CMyServer()
{
	m_iPort = 15000;
	m_vecClient.reserve(100);
	m_pDevice = make_shared<ID3D11Device*>(nullptr);
	m_pDeviceContext = make_shared<ID3D11DeviceContext*>(nullptr);
	m_pSwapChain = make_shared<IDXGISwapChain*>(nullptr);
	m_pBackBufferRTV = make_shared<ID3D11RenderTargetView*>(nullptr);
	m_pDepthStencilView = make_shared<ID3D11DepthStencilView*>(nullptr);
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

	while (true)
	{
		bResult = GetQueuedCompletionStatus(m_iocp, &dwSize, (PULONG_PTR)&userInfo, &overlap, INFINITE);

		if (bResult)
		{
			if (0 < dwSize)
			{
				SendAll(*userInfo);
				memset(userInfo->szBuf, 0, sizeof(userInfo->szBuf));

				dwSize = 0;
				dwFlag = 0;
				ZeroMemory(overlap, sizeof(OVERLAPPED));
				WSABUF wsaBuf = { 0 };
				wsaBuf.buf = userInfo->szBuf;
				wsaBuf.len = sizeof(userInfo->szBuf);

				WSARecv(userInfo->sock, &wsaBuf, 1, &dwSize, &dwFlag, overlap, NULL);
				if (WSAGetLastError() != WSA_IO_PENDING)
					cout << "PENDING 실패\n";
			}
			else
			{
				cout << "클라이언트 연결 해제\n";
				RemoveClient(userInfo->sock);
			}
		}
		else
		{
			//cout << "비정상 종료\n";
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

	while (INVALID_SOCKET != (clientSock = accept(m_listenSock, (SOCKADDR*)&clientAddr, &addrlen)))
	{
		cout << "클라이언트 접속\n";

		char msg[20] = { 0 };
		recv(clientSock, msg, sizeof(msg), 0);

		userInfo = new USERINFO;
		ZeroMemory(userInfo, sizeof(USERINFO));
		userInfo->sock = clientSock;
		strcpy(userInfo->szIP, inet_ntoa(clientAddr.sin_addr));
		userInfo->iPort = ntohs(clientAddr.sin_port);
		strcpy(userInfo->szName, msg);
		local_time<system_clock::duration> timer = current_zone()->to_local(system_clock::now());
		string str = format("{:%Y-%m-%d %X}", timer);
		strcpy(userInfo->szTime, str.c_str());

		EnterCriticalSection(&m_cs);
		m_vecClient.push_back(userInfo);
		LeaveCriticalSection(&m_cs);

		OVERLAPPED overlap;
		ZeroMemory(&overlap, sizeof(OVERLAPPED));

		CreateIoCompletionPort((HANDLE)clientSock, m_iocp, (ULONG_PTR)userInfo, 0);

		ZeroMemory(&wsaBuf, sizeof(WSABUF));
		wsaBuf.buf = userInfo->szBuf;
		wsaBuf.len = sizeof(userInfo->szBuf);
		dwFlag = 0;
		dwSize = 0;

		WSARecv(clientSock, &wsaBuf, 1, &dwSize, &dwFlag, &overlap, NULL);
		if (WSAGetLastError() != WSA_IO_PENDING)
			cout << "PENDING 실패\n";
	}

	cout << "Accept 실패\n";
}

HRESULT CMyServer::NativeConstruct()
{
	UINT iFlag = 0;

#ifdef _DEBUG
	iFlag = D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL			FreatureLV;

	if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, iFlag, nullptr, 0, D3D11_SDK_VERSION, m_pDevice.get(), &FreatureLV, m_pDeviceContext.get())))
		return E_FAIL;

	if (FAILED(SetSwapChain(g_hWnd, 1280, 720)))
		return E_FAIL;

	if (FAILED(SetBackBufferRTV()))
		return E_FAIL;

	if (FAILED(SetDepthStencil(1280, 720)))
		return E_FAIL;

	(*m_pDeviceContext.get())->OMSetRenderTargets(1, m_pBackBufferRTV.get(), *m_pDepthStencilView);

	return S_OK;
}

HRESULT CMyServer::SetSwapChain(HWND hWnd, UINT WinX, UINT WinY)
{
	unique_ptr<IDXGIDevice*> pDevice = make_unique<IDXGIDevice*>(nullptr);
	(*m_pDevice.get())->QueryInterface(__uuidof(IDXGIDevice), (void**)pDevice.get());

	unique_ptr<IDXGIAdapter*> pAdapter = make_unique<IDXGIAdapter*>(nullptr);
	(*pDevice.get())->GetParent(__uuidof(IDXGIAdapter), (void**)pAdapter.get());

	unique_ptr<IDXGIFactory*> pFactory = make_unique<IDXGIFactory*>(nullptr);
	(*pAdapter.get())->GetParent(__uuidof(IDXGIFactory), (void**)pFactory.get());

	DXGI_SWAP_CHAIN_DESC		SwapChain;
	ZeroMemory(&SwapChain, sizeof(DXGI_SWAP_CHAIN_DESC));

	SwapChain.BufferDesc.Width = WinX;
	SwapChain.BufferDesc.Height = WinY;
	SwapChain.BufferDesc.RefreshRate.Numerator = 60;
	SwapChain.BufferDesc.RefreshRate.Denominator = 1;
	SwapChain.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	SwapChain.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	SwapChain.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	SwapChain.SampleDesc.Quality = 0;
	SwapChain.SampleDesc.Count = 1;
	SwapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChain.BufferCount = 1;
	SwapChain.OutputWindow = hWnd;
	SwapChain.Windowed = TRUE;
	SwapChain.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	if (FAILED((*pFactory.get())->CreateSwapChain(*m_pDevice.get(), &SwapChain, m_pSwapChain.get())))
		return E_FAIL;

	return S_OK;
}

HRESULT CMyServer::SetBackBufferRTV()
{
	if (nullptr == m_pDevice)
		return E_FAIL;

	ID3D11Texture2D* pBackBufferTexture = nullptr;

	if (FAILED((*m_pSwapChain.get())->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBufferTexture)))
		return E_FAIL;

	if (FAILED((*m_pDevice.get())->CreateRenderTargetView(pBackBufferTexture, nullptr, m_pBackBufferRTV.get())))
		return E_FAIL;

	return S_OK;
}

HRESULT CMyServer::SetDepthStencil(UINT WinX, UINT WinY)
{
	if (nullptr == m_pDevice)
		return E_FAIL;

	ID3D11Texture2D* pDepthStencilTexture = nullptr;

	D3D11_TEXTURE2D_DESC	TextureDesc;
	ZeroMemory(&TextureDesc, sizeof(D3D11_TEXTURE2D_DESC));

	TextureDesc.Width = WinX;
	TextureDesc.Height = WinY;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.SampleDesc.Count = 1;

	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.MiscFlags = 0;

	if (FAILED((*m_pDevice.get())->CreateTexture2D(&TextureDesc, nullptr, &pDepthStencilTexture)))
		return E_FAIL;

	if (FAILED((*m_pDevice.get())->CreateDepthStencilView(pDepthStencilTexture, nullptr, m_pDepthStencilView.get())))
		return E_FAIL;

	return S_OK;
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
	m_threadApt = thread(&CMyServer::ThreadAccept, this, nullptr);

	return S_OK;
}

HRESULT CMyServer::ReleaseServer()
{
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

	for (int i = 0; i < m_vecThreadWorker.size(); ++i)
		m_vecThreadWorker[i].join();
	m_threadApt.join();

	m_vecThreadWorker.clear();
	vector<thread>().swap(m_vecThreadWorker);

	CloseHandle(m_iocp);
	shutdown(m_listenSock, SD_BOTH);
	closesocket(m_listenSock);
	DeleteCriticalSection(&m_cs);
	WSACleanup();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	m_pDevice.reset();
	m_pDeviceContext.reset();
	m_pSwapChain.reset();
	m_pBackBufferRTV.reset();
	m_pDepthStencilView.reset();

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
	io.Fonts->AddFontFromFileTTF("..\\Fonts\\SCDream5.otf", 18.f, NULL, io.Fonts->GetGlyphRangesKorean());

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
	ImGui_ImplDX11_Init((*m_pDevice.get()), (*m_pDeviceContext.get()));
}

bool CMyServer::SendAll(USERINFO& userInfo)
{
	cout << "Message : " << userInfo.szBuf << '\n';

	WSABUF wsaBuf = { 0 };
	wsaBuf.buf = userInfo.szBuf;
	wsaBuf.len = strlen(userInfo.szBuf);
	DWORD dwByte = 0;
	DWORD dwFlag = 0;

	auto iter = m_vecClient.begin();

	EnterCriticalSection(&m_cs);
	for (; iter != m_vecClient.end(); ++iter)
		WSASend((*iter)->sock, &wsaBuf, 1, &dwByte, dwFlag, NULL, NULL);
	LeaveCriticalSection(&m_cs);

	return true;
}

void CMyServer::MainWindow()
{
	if (ImGui::Begin("Main"))
	{
		if (ImGui::BeginTable("split1", 4, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders))
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
				ImGui::Text(m_vecClient[i]->szTime);
				ImGui::TableNextColumn();
				ImGui::Text(m_vecClient[i]->szIP);
				ImGui::TableNextColumn();
				ImGui::Text("%d", m_vecClient[i]->iPort);
				ImGui::TableNextColumn();
				ImGui::Text(m_vecClient[i]->szName);
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

void CMyServer::Render()
{
	XMFLOAT4 vColor(0.45f, 0.55f, 0.6f, 1.f);
	(*m_pDeviceContext.get())->ClearRenderTargetView(*m_pBackBufferRTV, (float*)&vColor);

	(*m_pDeviceContext.get())->ClearDepthStencilView(*m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (ImGui::Begin("MENU"))
	{
		ImGui::BeginChild("##", ImVec2(500, 300));
		ImGui::PushItemWidth(100);
		ImGui::Text("IP : ");
		ImGui::Text("Port : ");
		ImGui::SameLine();
		ImGui::DragInt("##", &m_iPort);
		ImGui::PopItemWidth();

		if (ImGui::Button("Server Open", ImVec2(150.f, 50.f)))
		{
			if (S_OK == CreateServer(m_iPort))
			{
				m_bRun = true;
				cout << "서버 가동 실행\n";
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Server Close", ImVec2(150.f, 50.f)))
		{
			cout << "서버 종료 실행\n";
		}

		if (m_bRun)
		{
			MainWindow();
		}

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

	(*m_pDeviceContext.get())->OMSetRenderTargets(1, m_pBackBufferRTV.get(), *m_pDepthStencilView);

	(*m_pSwapChain.get())->Present(0, 0);
}

