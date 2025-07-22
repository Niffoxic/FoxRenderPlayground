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
class WindowsManager final: public ISystem, public IFrame
{
public:
     WindowsManager() = default;
    ~WindowsManager()   override;

    FOX_CHECK_RETURN static std::optional<int> ProcessMessages();

    //~ Frame Interface Impl
    void OnFrameBegin()   override;
    void OnFramePresent() override;
    void OnFrameEnd()     override;

    //~ System Interface Impl
    FOX_CHECK_RETURN bool OnInit() _Success_(return != false) override;
    bool OnRelease() _Success_(return != false) override;

    //~ Getters
    FOX_CHECK_RETURN _Ret_maybenull_ HWND      GetWinHandle    () const { return m_hWnd;      }
    FOX_CHECK_RETURN _Ret_maybenull_ HINSTANCE GetWinHInstance () const { return m_hInstance; }

    FOX_CHECK_RETURN const std::string& GetWindowsTitle () const { return m_szWindowsTitle; }
    FOX_CHECK_RETURN bool               IsFullScreen    () const { return m_bFullScreen;    }
    FOX_CHECK_RETURN WINDOW_SIZE_DESC   GetWindowSize   () const { return m_descWindowSize; }

    //~ Setters
    void SetWindowSize    (FOX_IN const WINDOW_SIZE_DESC& desc) { m_descWindowSize = desc;   }
    void SetWindowsTitle  (FOX_IN const FString& title)         { m_szWindowsTitle = title;  }
    void AddOnWindowsTitle(FOX_IN const FString& addOn) const;
    void SetFullScreen    (FOX_IN bool fullScreen);

private:
    FOX_CHECK_RETURN bool InitWindow();

    //~ Handle Windows Message
    FOX_CHECK_RETURN LRESULT MessageHandler(
        FOX_IN HWND hwnd,
        FOX_IN UINT msg,
        FOX_IN WPARAM wParam,
        FOX_IN LPARAM lParam);

    FOX_CHECK_RETURN static LRESULT CALLBACK WindowProcSetup(
        FOX_IN HWND hwnd,
        FOX_IN UINT msg,
        FOX_IN WPARAM wParam,
        FOX_IN LPARAM lParam);

    FOX_CHECK_RETURN static LRESULT CALLBACK WindowProcThunk(
        FOX_IN HWND hwnd,
        FOX_IN UINT msg,
        FOX_IN WPARAM wParam,
        FOX_IN LPARAM lParam);

private:
    bool             m_bFullScreen{ false   };
    HINSTANCE        m_hInstance  { nullptr };
    HWND             m_hWnd       { nullptr };

    FString          m_szWindowsTitle { F_TEXT("Fox Render Playground")   };
    WINDOW_SIZE_DESC m_descWindowSize { .Width = 1280u, .Height = 720u        };
};

#endif //WINDOWSMANAGER_H
