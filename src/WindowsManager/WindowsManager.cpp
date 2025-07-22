//
// Created by niffo on 7/16/2025.
//

#include "WindowsManager.h"
#include "ExceptionHandler/WindowException.h"

WindowsManager::~WindowsManager()
{
    OnRelease();
}

std::optional<int> WindowsManager::ProcessMessages()
{
    MSG message;

    while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
    {
        if (message.message == WM_QUIT) return static_cast<int>(message.wParam);
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    return {};
}

void WindowsManager::OnFrameBegin()
{

}

void WindowsManager::OnFramePresent()
{

}

void WindowsManager::OnFrameEnd()
{

}

bool WindowsManager::OnInit()
{
    return InitWindow();
}

bool WindowsManager::OnRelease()
{
    return true;
}

void WindowsManager::AddOnWindowsTitle(const FString &addOn) const
{
    const FString title = m_szWindowsTitle + F_TEXT(" ") + addOn;
    SetWindowText(m_hWnd, title.c_str());
}

void WindowsManager::SetFullScreen(bool fullScreen)
{
    m_bFullScreen = fullScreen;
}

bool WindowsManager::InitWindow()
{
    m_hInstance = GetModuleHandle(nullptr);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WindowProcSetup;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(LONG_PTR);
    wc.hInstance = m_hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = "WINDOW_CLASS_NAME";
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
    {
        return false;
    }

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    RECT rect = m_descWindowSize.GetRect();
    if (!AdjustWindowRect(&rect, style, FALSE))
    {
        return false;
    }

    int adjustedWidth = rect.right - rect.left;
    int adjustedHeight = rect.bottom - rect.top;

    m_hWnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        m_szWindowsTitle.c_str(),
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        adjustedWidth, adjustedHeight,
        nullptr,
        nullptr,
        m_hInstance,
        this);

    if (!m_hWnd)
    {
        THROW_WINDOW_EXCEPTION();
    }

    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);

    return true;
}

LRESULT WindowsManager::MessageHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
    {
        PostQuitMessage(0);
        return 0;
    }
    default: break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT WindowsManager::WindowProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        const auto create = reinterpret_cast<CREATESTRUCT*>(lParam);
        auto that = static_cast<WindowsManager*>(create->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WindowProcThunk));
        return that->MessageHandler(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT WindowsManager::WindowProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (const auto that = reinterpret_cast<WindowsManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))
    {
        return that->MessageHandler(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
