#pragma once
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "dxguid")

#pragma warning(disable:4996)

#include <WinSock2.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_impl_win32.h"
#include "../../ImGui/imgui_impl_dx11.h"
#include <crtdbg.h>
#include <iostream>
#include <thread>
#include <memory>
#include <vector>
#include <string>
#include <ctime>

#define _CRTDBG_MAP_ALLOC

#ifdef _DEBUG
#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
#define DBG_NEW new
#endif

extern HINSTANCE g_hInst;
extern HWND g_hWnd;

using namespace DirectX;
using namespace std;
