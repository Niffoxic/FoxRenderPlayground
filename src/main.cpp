#include <windows.h>

// Window procedure callback
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);  // Request to exit message loop
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd); // Optional cleanup before exit
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    auto CLASS_NAME = TEXT("MySimpleWindowClass");

    // Register the window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    // Create the window
    HWND hwnd = CreateWindowEx(
        0,                          // Optional styles
        CLASS_NAME,                 // Window class name
        TEXT("My First Win32 Window"),   // Title text
        WS_OVERLAPPEDWINDOW,        // Window style

        // Size & position
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,

        nullptr,       // Parent window
        nullptr,       // Menu
        hInstance,     // App instance
        nullptr        // Additional application data
    );

    if (!hwnd)
    {
        MessageBox(nullptr, TEXT("Window creation failed!"), TEXT("Error"), MB_OK | MB_ICONERROR);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Run the message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
