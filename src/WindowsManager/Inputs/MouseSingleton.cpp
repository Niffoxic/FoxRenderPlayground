//
// Created by niffo on 7/23/2025.
//

#include "Common/DefineWindows.h"
#include "MouseSingleton.h"

MouseSingleton::MouseSingleton()
{
    //~ Raw Mouse Inputs
    RAWINPUTDEVICE rid{};
    rid.usUsagePage = 0x01;
    rid.usUsage     = 0x02;
    rid.dwFlags     = 0;
    rid.hwndTarget  = nullptr;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
    {
#if defined(DEBUG) || defined(_DEBUG)
        __debugbreak();
#else
        THROW_EXCEPTION_MSG("Failed to register rawinput device");
#endif
    }
}

void MouseSingleton::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INPUT:
    {
        UINT dwSize = 0;
        GetRawInputData(
            reinterpret_cast<HRAWINPUT>(lParam),
            RID_INPUT,
            nullptr, &dwSize,
            sizeof(RAWINPUTHEADER)
        );

        if (dwSize == 0) break;

        std::vector<BYTE> rawBuffer(dwSize);

        if (GetRawInputData(
                reinterpret_cast<HRAWINPUT>(lParam),
                RID_INPUT,
                rawBuffer.data(),
                &dwSize,
                sizeof(RAWINPUTHEADER)) != dwSize
            )
            break;

        if (const RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(rawBuffer.data()); raw->header.dwType == RIM_TYPEMOUSE)
        {
            const LONG dx = raw->data.mouse.lLastX;
            const LONG dy = raw->data.mouse.lLastY;

            m_descMouseState.Delta.x += dx;
            m_descMouseState.Delta.y += dy;
        }
        break;
    }
    case WM_MOUSEMOVE:
    {
        const LONG x = GET_X_LPARAM(lParam);
        const LONG y = GET_Y_LPARAM(lParam);

        m_descMouseState.Delta.x = x - m_descMouseState.Position.x;
        m_descMouseState.Delta.y = y - m_descMouseState.Position.y;

        m_descMouseState.Position.x = x;
        m_descMouseState.Position.y = y;
        break;
    }

    case WM_MOUSEWHEEL:
    {
        const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        m_descMouseState.WheelDelta += delta / WHEEL_DELTA;
        break;
    }

    case WM_LBUTTONDOWN: m_btButtonStates.set(static_cast<size_t>(EMouseButtons::MOUSE_LEFT  )); break;
    case WM_RBUTTONDOWN: m_btButtonStates.set(static_cast<size_t>(EMouseButtons::MOUSE_RIGHT )); break;
    case WM_MBUTTONDOWN: m_btButtonStates.set(static_cast<size_t>(EMouseButtons::MOUSE_MIDDLE)); break;

    case WM_LBUTTONUP: m_btButtonStates.reset(static_cast<size_t>(EMouseButtons::MOUSE_LEFT  )); break;
    case WM_RBUTTONUP: m_btButtonStates.reset(static_cast<size_t>(EMouseButtons::MOUSE_RIGHT )); break;
    case WM_MBUTTONUP: m_btButtonStates.reset(static_cast<size_t>(EMouseButtons::MOUSE_MIDDLE)); break;

    case WM_XBUTTONDOWN:
    {
        const WORD btn = GET_XBUTTON_WPARAM(wParam);
        if (btn & XBUTTON1)
            m_btButtonStates.set(static_cast<size_t>(EMouseButtons::MOUSE_X1));
        if (btn & XBUTTON2)
            m_btButtonStates.set(static_cast<size_t>(EMouseButtons::MOUSE_X2));
        break;
    }

    case WM_XBUTTONUP:
    {
        const WORD btn = GET_XBUTTON_WPARAM(wParam);
        if (btn & XBUTTON1)
            m_btButtonStates.reset(static_cast<size_t>(EMouseButtons::MOUSE_X1));
        if (btn & XBUTTON2)
            m_btButtonStates.reset(static_cast<size_t>(EMouseButtons::MOUSE_X2));
        break;
    }

    default:
        break;
    }
}

void MouseSingleton::Reset()
{
    m_descMouseState.Delta = {0l, 0l};
    m_descMouseState.WheelDelta = 0;
    m_btButtonStates.reset();
}

LONG MouseSingleton::GetX        () const noexcept { return m_descMouseState.Position.x; }
LONG MouseSingleton::GetY        () const noexcept { return m_descMouseState.Position.y; }
LONG MouseSingleton::GetDeltaX   () const noexcept { return m_descMouseState.Delta.x;    }
LONG MouseSingleton::GetDeltaY   () const noexcept { return m_descMouseState.Delta.y;    }
int MouseSingleton::GetWheelDelta() const noexcept { return m_descMouseState.WheelDelta; }

const MOUSE_STATE_DESC & MouseSingleton::GetState() const noexcept { return m_descMouseState; }

bool MouseSingleton::IsMoved() const noexcept
{
    return m_descMouseState.GetDeltaX() != 0l || m_descMouseState.GetDeltaY() != 0l;
}

bool MouseSingleton::IsScrolled() const noexcept
{
    return m_descMouseState.WheelDelta != 0;
}

bool MouseSingleton::IsButtonDown(EMouseButtons btn) const
{
    const auto index = static_cast<size_t>(btn);

    if (index >= BUTTON_BOUND)
    {
#if defined(DEBUG) || defined(_DEBUG)
        __debugbreak();
#else
        return false;
#endif
    }

    return m_btButtonStates.test(index);
}

void MouseSingleton::DebugKeysPressed() const
{
    for (size_t i = 0; i < BUTTON_BOUND; ++i)
    {
        if (!m_btButtonStates.test(i))
            continue;

        switch (static_cast<EMouseButtons>(i))
        {
        case EMouseButtons::MOUSE_LEFT:     LOG_INFO("[MOUSE]: Left Button Down");   break;
        case EMouseButtons::MOUSE_RIGHT:    LOG_INFO("[MOUSE]: Right Button Down");  break;
        case EMouseButtons::MOUSE_MIDDLE:   LOG_INFO("[MOUSE]: Middle Button Down"); break;
        case EMouseButtons::MOUSE_WHEEL:    LOG_INFO("[MOUSE]: Wheel Pressed");      break;
        case EMouseButtons::MOUSE_X1:       LOG_INFO("[MOUSE]: X1 Button Down");     break;
        case EMouseButtons::MOUSE_X2:       LOG_INFO("[MOUSE]: X2 Button Down");     break;
        default:
            LOG_INFO("[MOUSE]: Unknown Button Index {}", i);
            break;
        }
    }

    if (IsMoved())
        LOG_INFO("[MOUSE]: Position ({}, {})", m_descMouseState.GetX(), m_descMouseState.GetY());

    if (IsScrolled())
        LOG_INFO("[MOUSE]: Scrolled Wheel {}", m_descMouseState.WheelDelta);
}
