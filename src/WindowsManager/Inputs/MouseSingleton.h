//
// Created by niffo on 7/23/2025.
//
#ifndef MOUSESINGLETON_H
#define MOUSESINGLETON_H

#include "Interface/ISingleton.h"

#include <cstdint>
#include <bitset>


using MouseButton = uint8_t;

typedef struct _FX_MOUSE_STATE_DESC
{
    POINT Position{ 0l, 0l };
    POINT Delta   { 0l, 0l };
    int WheelDelta{ 0 };

    _fox_Return_safe
    constexpr LONG GetX() const noexcept { return Position.x; }
    _fox_Return_safe
    constexpr LONG GetY() const noexcept { return Position.y; }
    _fox_Return_safe
    constexpr LONG GetDeltaX() const noexcept { return Delta.x; }
    _fox_Return_safe
    constexpr LONG GetDeltaY() const noexcept { return Delta.y; }
} MOUSE_STATE_DESC;

enum class EMouseButtons: MouseButton
{
    MOUSE_LEFT,
    MOUSE_RIGHT,
    MOUSE_MIDDLE,
    MOUSE_WHEEL,
    MOUSE_X1,
    MOUSE_X2
};

class MouseSingleton final: public ISingleton<MouseSingleton>
{
    friend ISingleton<MouseSingleton>;
public:
    void HandleMessage(
        _fox_In_ UINT message,
        _fox_In_ WPARAM wParam,
        _fox_In_ LPARAM lParam
    );

    void Reset();

    //~ Query functions
    _fox_Return_safe LONG GetX         () const noexcept;
    _fox_Return_safe LONG GetY         () const noexcept;
    _fox_Return_safe LONG GetDeltaX    () const noexcept;
    _fox_Return_safe LONG GetDeltaY    () const noexcept;
    _fox_Return_safe int  GetWheelDelta() const noexcept;

    _fox_Return_safe const MOUSE_STATE_DESC& GetState() const noexcept;

    _fox_Return_safe bool IsMoved     () const noexcept;
    _fox_Return_safe bool IsScrolled  () const noexcept;
    _fox_Return_safe bool IsButtonDown(_fox_In_ EMouseButtons btn) const;

    void DebugKeysPressed() const;

private:
    MouseSingleton();

private:
    static constexpr MouseButton BUTTON_BOUND{ 6 };

    std::bitset<BUTTON_BOUND>  m_btButtonStates{};
    MOUSE_STATE_DESC           m_descMouseState{};
};

#endif //MOUSESINGLETON_H
