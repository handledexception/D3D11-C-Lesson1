#include <Windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <stdbool.h>

void *os_dlopen(const char* path)
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

void* os_dlsym(void* module, const char* func)
{
    return GetProcAddress(module, func);
}

void os_dlclose(void* module)
{
    FreeLibrary(module);
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
    HMODULE d3d11_dll = (HMODULE)os_dlopen("d3d11.dll");
    if (!d3d11_dll) {
        return 1;
    }
    HMODULE dxgi_dll = (HMODULE)os_dlopen("dxgi.dll");
    if (!dxgi_dll) {
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

    ShowWindow(hwnd, cmd_show);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}