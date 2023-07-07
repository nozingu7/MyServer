#include "CMainApp.h"
#include "CMyClient.h"

CMainApp::CMainApp()
{
}

CMainApp::~CMainApp()
{
	delete m_pClient;

	m_pSwapChain->Release();
	m_pBackBufferRTV->Release();
	m_pDepthStencilView->Release();
	m_pDeviceContext->Release();
	m_pDevice->Release();
}

HRESULT CMainApp::NativeConstruct()
{
	UINT iFlag = 0;

#ifdef _DEBUG
	iFlag = D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL			FreatureLV;

	if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, iFlag, nullptr, 0, D3D11_SDK_VERSION, &m_pDevice, &FreatureLV, &m_pDeviceContext)))
		return E_FAIL;

	if (FAILED(SetSwapChain(g_hWnd, 800, 600)))
		return E_FAIL;

	if (FAILED(SetBackBufferRTV()))
		return E_FAIL;

	if (FAILED(SetDepthStencil(800, 600)))
		return E_FAIL;

	m_pDeviceContext->OMSetRenderTargets(1, &m_pBackBufferRTV, m_pDepthStencilView);

	if (FAILED(InitClient()))
		return E_FAIL;

	return S_OK;
}

HRESULT CMainApp::SetSwapChain(HWND hWnd, UINT WinX, UINT WinY)
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

HRESULT CMainApp::SetBackBufferRTV()
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

HRESULT CMainApp::SetDepthStencil(UINT WinX, UINT WinY)
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

HRESULT CMainApp::InitClient()
{
	m_pClient = new CMyClient(m_pDevice, m_pDeviceContext);
	if (nullptr == m_pClient)
		return E_FAIL;

	m_pClient->Init_Imgui();

	return S_OK;
}

void CMainApp::Render()
{
	XMFLOAT4 vColor(0.45f, 0.55f, 0.6f, 1.f);
	m_pDeviceContext->ClearRenderTargetView(m_pBackBufferRTV, (float*)&vColor);
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	// ¼­¹ö GUI Render
	if (nullptr != m_pClient)
		m_pClient->Render();

	m_pDeviceContext->OMSetRenderTargets(1, &m_pBackBufferRTV, m_pDepthStencilView);
	m_pSwapChain->Present(0, 0);
}

bool CMainApp::isAlive()
{
	if (nullptr != m_pClient)
		return m_pClient->Alive();

	return false;
}
