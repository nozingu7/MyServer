#pragma once
#include "common.h"

class CMainApp
{
public:
	CMainApp();
	~CMainApp();

private:
	HRESULT SetSwapChain(HWND hWnd, UINT WinX, UINT WinY);
	HRESULT SetBackBufferRTV();
	HRESULT SetDepthStencil(UINT WinX, UINT WinY);
	HRESULT InitClient();

public:
	HRESULT NativeConstruct();
	void Render();
	bool isAlive();

private:
	class CMyClient* m_pClient = nullptr;

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pDeviceContext = nullptr;
	IDXGISwapChain* m_pSwapChain = nullptr;
	ID3D11RenderTargetView* m_pBackBufferRTV = nullptr;
	ID3D11DepthStencilView* m_pDepthStencilView = nullptr;
};

