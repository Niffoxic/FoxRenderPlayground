//
// Created by niffo on 7/16/2025.
//

#ifndef WINDOWSMANAGER_H
#define WINDOWSMANAGER_H

#include "Common/DefineWindows.h"

#include <string>
#include <optional>

#include "Interface/IFrame.h"
#include "Interface/ISystem.h"

class WindowsManager final: public ISystem, public IFrame
{
public:
    WindowsManager() = default;
    ~WindowsManager() override;

    static std::optional<int> ProcessMessages();

    //~ Frame Interface Impl
    void OnFrameBegin() override;
    void OnFramePresent() override;
    void OnFrameEnd() override;

    //~ System Interface Impl
    bool OnInit() override;
    bool OnRelease() override;

    //~ Getters
    [[nodiscard]] HWND GetWinHandle()         const { return m_hWnd;            }
    [[nodiscard]] HINSTANCE GetWinHInstance() const { return m_hInstance;       }
    const std::string& GetWindowsTitle()      const { return m_szWindowsTitle;  }
    bool IsFullScreen()                       const { return m_bFullScreen;     }
    WINDOW_SIZE_DESC GetWindowSize()          const { return m_WindowSizeDesc;  }

    //~ Setters
    void SetWindowSize(const WINDOW_SIZE_DESC& desc) { m_WindowSizeDesc = desc; }
    void SetWindowsTitle(const std::string& title)   { m_szWindowsTitle = title;  }
    void SetFullScreen(bool fullScreen);

private:
    bool InitWindow();

    //~ Handle Windows Message
    LRESULT MessageHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK WindowProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK WindowProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    bool             m_bFullScreen{ false };
    HINSTANCE        m_hInstance;
    HWND             m_hWnd;
    std::string      m_szWindowsTitle{ "Default Window" };
    WINDOW_SIZE_DESC m_WindowSizeDesc { .Width = 1280u, .Height = 720u };
};

#endif //WINDOWSMANAGER_H
