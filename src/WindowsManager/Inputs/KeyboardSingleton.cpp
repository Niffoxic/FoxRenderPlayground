//
// Created by niffo on 7/23/2025.
//

#include "KeyboardSingleton.h"

void KeyboardSingleton::HandleMessage(
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    if (wParam >= KEY_BOUND)
        return;

    switch (message)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        m_btKeyStates.set(static_cast<KeyStroke>(wParam), true);
        break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        m_btKeyStates.set(static_cast<KeyStroke>(wParam), false);
        break;

    default:
        break;
    }
}

void KeyboardSingleton::Reset()
{
    m_btKeyStates.reset();
}

bool KeyboardSingleton::IsKeyDown(const KeyStroke key) const
{
    if (key >= KEY_BOUND)
    {
#if defined(DEBUG) || defined(_DEBUG)
        __debugbreak();
#else
        THROW_EXCEPTION_MSG("Requested key is out of bounds!");
#endif
    }

    return m_btKeyStates.test(key);
}

bool KeyboardSingleton::operator[](const KeyStroke key) const
{
    return IsKeyDown(key);
}

void KeyboardSingleton::DebugKeysPressed() const
{
    for (KeyStroke key = 0; key < KEY_BOUND; ++key)
    {
        if (!m_btKeyStates.test(key)) continue;

        const UINT scanCode = MapVirtualKeyA(key, MAPVK_VK_TO_VSC);
        LONG lParam = (scanCode << 16);

        switch (key)
        {
        case VK_LEFT:   case VK_UP: case VK_RIGHT: case VK_DOWN:
        case VK_PRIOR:  case VK_NEXT:
        case VK_END:    case VK_HOME:
        case VK_INSERT: case VK_DELETE:
        case VK_DIVIDE: case VK_NUMLOCK:
            lParam |= (1 << 24);
            break;
        default:;
        }

        char name[128]{};
        if (GetKeyNameTextA(lParam, name, static_cast<int>(sizeof(name))) > 0)
            LOG_INFO("[KEY]: {} (VK: {})", name, key);
        else LOG_INFO("[KEY]: Unknown (VK: {})", key);
    }
}
