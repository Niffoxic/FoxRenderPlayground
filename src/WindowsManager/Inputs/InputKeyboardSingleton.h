//
// Created by niffo on 7/23/2025.
//

#ifndef INPUTKEYBOARDSINGLETON_H
#define INPUTKEYBOARDSINGLETON_H

#include "Interface/ISingleton.h"

#include <bitset>


using KEY = uint16_t;

class InputKeyboardSingleton final: public ISingleton<InputKeyboardSingleton>
{
    friend class ISingleton<InputKeyboardSingleton>;
public:
    void HandleMessage(
        _fox_In_ UINT message,
        _fox_In_ WPARAM wParam,
        _fox_In_ LPARAM lParam
    );

    //~ Call it after windows frame end so that I can clean stuff from here
    void Reset();

    //~ Query functions
    _fox_Success_(return == true)
    bool IsKeyDown (_fox_In_ KEY key) _fox_Pre_satisfies_(KEY < KEY_BOUND)  const;
    _fox_Success_(return == true)
    bool operator[](_fox_In_ KEY key) _fox_Pre_satisfies_(KEY < KEY_BOUND)  const;

    void DebugKeysPressed() const;

private:
    InputKeyboardSingleton() = default;

private:
    static constexpr KEY KEY_BOUND{ 256 };
    std::bitset<KEY_BOUND> m_btKeyStates{};
};

#endif //INPUTKEYBOARDSINGLETON_H
