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

//--------------------------------------------
// WindowsManager
// Handles Win32 windowing, messaging, and state.
//--------------------------------------------
class WindowsManager final: public ISystem
{
    FOX_SYSTEM_GENERATOR(WindowsManager);
public:
     WindowsManager() = default;
    ~WindowsManager()   override;

    _fox_Return_enforce static std::optional<int> ProcessMessages();

    //~ System Interface Impl
    _fox_Return_enforce bool OnInit() _fox_Success_(return != false) override;
    void OnUpdateStart(float deltaTime) override;
    void OnUpdateEnd() override;
    void OnRelease() override;

    //~ Getters
    _fox_Return_enforce _fox_Ret_maybenull_ HWND      GetWinHandle    () const { return m_hWnd;      }
    _fox_Return_enforce _fox_Ret_maybenull_ HINSTANCE GetWinHInstance () const { return m_hInstance; }

    _fox_Return_enforce const FString&     GetWindowsTitle() const { return m_szWindowsTitle; }
    _fox_Return_enforce bool               IsFullScreen   () const { return m_bFullScreen;    }
    _fox_Return_enforce WINDOW_SIZE_DESC   GetWindowSize  () const { return m_descWindowSize; }

    //~ Setters
    void SetWindowSize    (_fox_In_ const WINDOW_SIZE_DESC& desc) { m_descWindowSize = desc;   }
    void SetWindowsTitle  (_fox_In_ const FString& title)         { m_szWindowsTitle = title;  }
    void SetFullScreen    (_fox_In_ bool fullScreen);
    void AddOnWindowsTitle(_fox_In_ const FString& addOn) const;

private:
    _fox_Return_enforce bool InitWindow();

    //~ Handle Windows Message
    _fox_Return_enforce LRESULT MessageHandler(
        _fox_In_ HWND hwnd,
        _fox_In_ UINT msg,
        _fox_In_ WPARAM wParam,
        _fox_In_ LPARAM lParam);

    _fox_Return_enforce static LRESULT CALLBACK WindowProcSetup(
        _fox_In_ HWND hwnd,
        _fox_In_ UINT msg,
        _fox_In_ WPARAM wParam,
        _fox_In_ LPARAM lParam);

    _fox_Return_enforce static LRESULT CALLBACK WindowProcThunk(
        _fox_In_ HWND hwnd,
        _fox_In_ UINT msg,
        _fox_In_ WPARAM wParam,
        _fox_In_ LPARAM lParam);

private:
    bool             m_bFullScreen{ false   };
    HINSTANCE        m_hInstance  { nullptr };
    HWND             m_hWnd       { nullptr };

    FString          m_szWindowsTitle { F_TEXT("Fox Render Playground")   };
    WINDOW_SIZE_DESC m_descWindowSize { .Width = 1280u, .Height = 720u        };
};

#endif //WINDOWSMANAGER_H
