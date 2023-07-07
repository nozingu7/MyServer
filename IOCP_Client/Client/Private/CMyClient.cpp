#include "CMyClient.h"

CMyClient::CMyClient()
{
	m_sock = 0;
	m_bConnectEnable = false;
}

CMyClient::~CMyClient()
{
	Release();
}

HRESULT CMyClient::NativeConstruct()
{
	UINT iFlag = 0;

#ifdef _DEBUG
	iFlag = D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL			FreatureLV;

	if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, iFlag, nullptr, 0, D3D11_SDK_VERSION, &m_pDevice, &FreatureLV, &m_pDeviceContext)))
		return E_FAIL;

	if (FAILED(SetSwapChain(g_hWnd, 1280, 720)))
		return E_FAIL;

	if (FAILED(SetBackBufferRTV()))
		return E_FAIL;

	if (FAILED(SetDepthStencil(1280, 720)))
		return E_FAIL;

	m_pDeviceContext->OMSetRenderTargets(1, &m_pBackBufferRTV, m_pDepthStencilView);

	return S_OK;
}

HRESULT CMyClient::SetSwapChain(HWND hWnd, UINT WinX, UINT WinY)
{
	IDXGIDevice* pDevice = nullptr;
	m_pDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDevice);

	IDXGIAdapter* pAdapter = nullptr;
	pDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&pAdapter);

	IDXGIFactory* pFactory = nullptr;
	pAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&pFactory);

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

	if (FAILED(pFactory->CreateSwapChain(m_pDevice, &SwapChain, &m_pSwapChain)))
		return E_FAIL;

	pDevice->Release();
	pAdapter->Release();
	pFactory->Release();

	return S_OK;
}

HRESULT CMyClient::SetBackBufferRTV()
{
	if (nullptr == m_pDevice)
		return E_FAIL;

	ID3D11Texture2D* pBackBufferTexture = nullptr;

	if (FAILED(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBufferTexture)))
		return E_FAIL;

	if (FAILED(m_pDevice->CreateRenderTargetView(pBackBufferTexture, nullptr, &m_pBackBufferRTV)))
		return E_FAIL;

	pBackBufferTexture->Release();

	return S_OK;
}

HRESULT CMyClient::SetDepthStencil(UINT WinX, UINT WinY)
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

	if (FAILED(m_pDevice->CreateTexture2D(&TextureDesc, nullptr, &pDepthStencilTexture)))
		return E_FAIL;

	if (FAILED(m_pDevice->CreateDepthStencilView(pDepthStencilTexture, nullptr, &m_pDepthStencilView)))
		return E_FAIL;

	pDepthStencilTexture->Release();

	return S_OK;
}

HRESULT CMyClient::ConnectServer(const char* szName)
{
	WSAData wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata))
	{
		cout << "윈속 초기화 실패!\n";
		return E_FAIL;
	}

	m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(15000);

	if (SOCKET_ERROR == connect(m_sock, (SOCKADDR*)&addr, sizeof(addr)))
	{
		cout << "서버에 연결할 수 없습니다!\n";
		return E_FAIL;
	}

	send(m_sock, szName, sizeof(szName), 0);

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
	XMFLOAT4 vColor(0.45f, 0.55f, 0.6f, 1.f);
	m_pDeviceContext->ClearRenderTargetView(m_pBackBufferRTV, (float*)&vColor);

	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (ImGui::Begin("MENU"))
	{
		Menu();

		ImGui::SameLine();

		if (ImGui::Button(u8"서버 종료", ImVec2(150.f, 50.f)))
		{
			cout << "서버 종료 실행\n";
			m_bAlive = false;
			m_bConnectEnable = false;
		}
	}
	ImGui::End();

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

	m_pDeviceContext->OMSetRenderTargets(1, &m_pBackBufferRTV, m_pDepthStencilView);

	m_pSwapChain->Present(0, 0);
}

void CMyClient::ShowChat()
{
	if (ImGui::Begin(u8"채팅"))
	{
		ImGui::BeginChild("Chatting", ImVec2(0, -25), true, 0);

		int iSize = (int)m_vecInfo.size();
		for (int i = 0; i < iSize; i++)
			ImGui::Text("%s", m_vecInfo[i]->szBuf);

		ImGui::EndChild();

		ImGui::InputText("##", m_szInputBuf, IM_ARRAYSIZE(m_szInputBuf));
		if (GetKeyState(VK_RETURN) & 0x8000)
		{
			if (strcmp("", m_szInputBuf))
			{
				strcpy(m_szBuf, m_Info.szName);
				strcat(m_szBuf, " : ");
				strcat(m_szBuf, m_szInputBuf);
				int len = (int)strlen(m_szBuf);
				send(m_sock, m_szBuf, len + 1, 0);
				memset(m_szInputBuf, 0, sizeof(m_szInputBuf));
			}
		}

		ImGui::SameLine();

		if (ImGui::Button(u8"보내기", ImVec2(50, 25)))
		{
			if (strcmp("", m_szInputBuf))
			{
				strcpy(m_szBuf, m_Info.szName);
				strcat(m_szBuf, " : ");
				strcat(m_szBuf, m_szInputBuf);
				int len = (int)strlen(m_szBuf);
				send(m_sock, m_szBuf, len + 1, 0);
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

	m_pSwapChain->Release();
	m_pBackBufferRTV->Release();
	m_pDepthStencilView->Release();
	m_pDeviceContext->Release();
	m_pDevice->Release();
}

void CMyClient::ThreadRecv(void* pData)
{
	char msg[1024] = { 0 };
	int len = sizeof(msg);

	while (0 < recv(m_sock, msg, len, 0))
	{
		USERINFO* pInfo = new USERINFO;
		strcpy(pInfo->szBuf, msg);
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

void CMyClient::Menu()
{
	if (ImGui::Button(u8"채팅서버 입장", ImVec2(150.f, 50.f)))
		ImGui::OpenPopup("NickName");

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("NickName", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		TextCenter(u8"닉네임을 설정해주세요!");
		ImGui::InputText("##", m_Info.szName, IM_ARRAYSIZE(m_Info.szName));
		WindowCenter(u8"확인##1");
		if (ImGui::Button(u8"확인##1", ImVec2(50.f, 25.f)))
		{
			// Connect 함수 호출
			if (S_OK == ConnectServer(m_Info.szName))
			{
				ImGui::CloseCurrentPopup();
			}
			else
			{
				ImGui::CloseCurrentPopup();
				m_bConnectFail = true;
			}
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
