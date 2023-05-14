#include <Windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

// Required for accessing the dxgi and d3d11 C API
#define COBJMACROS
#define CINTERFACE
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dxgiformat.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

struct gfx_system {
	IDXGIFactory2* dxgi_factory;
	IDXGIAdapter1* dxgi_adapter;
	IDXGIDevice1* dxgi_device;
	IDXGISwapChain2* dxgi_swap_chain;
	ID3D11Device1* device;
	ID3D11DeviceContext1* ctx;
};

#define ADAPTER_INDEX 0

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

static const GUID IID_ID3D11Device1 = {0xa04bfb29,
					  0x08ef,
					  0x43d6,
					  {0xa4, 0x9c, 0xa9, 0xbd, 0xbd, 0xcb,
					   0xe6, 0x86}};
static const GUID IID_ID3D11DeviceContext1 = {0xbb2c6faa,
						 0xb5fb,
						 0x4082,
						 {0x8e, 0x6b, 0x38, 0x8b, 0x8c,
						  0xfa, 0x90, 0xe1}};

static const GUID IID_IDXGIFactory2 = {0x50c83a1c,
					  0xe072,
					  0x4c48,
					  {0x87, 0xb0, 0x36, 0x30, 0xfa, 0x36,
					   0xa6, 0xd0}};
static const GUID IID_IDXGIDevice1 = {0x77db970f,
					 0x6276,
					 0x48ba,
					 {0xba, 0x28, 0x07, 0x01, 0x43, 0xb4,
					  0x39, 0x2c}};

typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID riid,
						  void** ppFactory);
static PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2Func = NULL;
static PFN_D3D11_CREATE_DEVICE D3D11CreateDeviceFunc = NULL;

void *lib_open(const char* path)
{
    if (!path)
        return NULL;

    size_t len = strlen(path) + 1;
    wchar_t* path_wide = (wchar_t*)malloc((sizeof(wchar_t) * len));
    size_t conv_len = 0;
    size_t wlen = MultiByteToWideChar(CP_ACP, 0, path, (int)len, 0, 0);
    MultiByteToWideChar(CP_ACP, 0, path, (int)len, path_wide, (int)wlen);

    HMODULE module = LoadLibrary(path_wide);
    if (!module)
        return NULL;

    free((void*)path_wide);
    path_wide = NULL;

    return (void*)module;
}

void lib_close(void* module)
{
    FreeLibrary(module);
}

void* lib_get_sym(void* module, const char* func)
{
    return GetProcAddress(module, func);
}

void close_libs(HMODULE d3d11, HMODULE dxgi)
{
    if (d3d11)
        lib_close(d3d11);
    if (dxgi)
        lib_close(dxgi);
}

bool gfx_create_device(uint32_t adapter, struct gfx_system* gfx)
{
	UINT dxgi_flags = 0;
#if defined(BM_DEBUG)
	dxgi_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    HRESULT hr = CreateDXGIFactory2Func(dxgi_flags, &IID_IDXGIFactory2, (void**)&gfx->dxgi_factory);
    if (FAILED(hr)) {
        return false;
    }

    hr = IDXGIFactory2_EnumAdapters1(gfx->dxgi_factory, adapter, &gfx->dxgi_adapter);
    if (FAILED(hr)) {
        return false;
    }

	UINT create_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(BM_DEBUG)
	create_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	const D3D_FEATURE_LEVEL feature_levels[] = {
		D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,  D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1};

	UINT num_feature_levels = ARRAY_SIZE(feature_levels);

    D3D_FEATURE_LEVEL feature_level;
	hr = D3D11CreateDeviceFunc((IDXGIAdapter*)gfx->dxgi_adapter,
				   D3D_DRIVER_TYPE_UNKNOWN, NULL, create_flags,
				   feature_levels, num_feature_levels,
				   D3D11_SDK_VERSION,
				   (ID3D11Device**)&gfx->device,
				   &feature_level,
				   (ID3D11DeviceContext**)&gfx->ctx);
    if (FAILED(hr)) {
        return false;
    }

	hr = ID3D11Device_QueryInterface(gfx->device, &IID_ID3D11Device1, (void**)&gfx->device);
    if (FAILED(hr)) {
        return false;
    }

	hr = ID3D11DeviceContext_QueryInterface(gfx->ctx,
						&IID_ID3D11DeviceContext1,
						(void**)&gfx->ctx);
    if (FAILED(hr)) {
        return false;
    }

	hr = ID3D11Device_QueryInterface(gfx->device,
					 &IID_IDXGIDevice1,
					 (void**)&gfx->dxgi_device);
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE instance_prev, PWSTR cmd_line, int cmd_show)
{
    HMODULE d3d11_dll = (HMODULE)lib_open("d3d11.dll");
    if (!d3d11_dll) {
        return 1;
    }
    HMODULE dxgi_dll = (HMODULE)lib_open("dxgi.dll");
    if (!dxgi_dll) {
        return 1;
    }

    CreateDXGIFactory2Func = lib_get_sym(dxgi_dll, "CreateDXGIFactory2");
    D3D11CreateDeviceFunc = lib_get_sym(d3d11_dll, "D3D11CreateDevice");
    if (!CreateDXGIFactory2Func || !D3D11CreateDeviceFunc) {
        close_libs(d3d11_dll, dxgi_dll);
        return 1;
    }

    // Register the window class.
    const wchar_t class_name[]  = L"D3D11 in C: Lesson 1";

    WNDCLASS wc = { 0 };

    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = instance;
    wc.lpszClassName = class_name;

    RegisterClass(&wc);

    // Create the window.

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        class_name,                     // Window class
        L"Learn to Program Windows",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window
        NULL,       // Menu
        instance,  // Instance handle
        NULL        // Additional application data
        );

    if (hwnd == NULL)
    {
        return 0;
    }

    struct gfx_system gfx = { 0 };
    gfx_create_device(ADAPTER_INDEX, &gfx);

    ShowWindow(hwnd, cmd_show);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}