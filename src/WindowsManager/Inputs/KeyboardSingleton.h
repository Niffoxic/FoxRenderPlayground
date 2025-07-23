//
// Created by niffo on 7/23/2025.
//

#ifndef INPUTKEYBOARDSINGLETON_H
#define INPUTKEYBOARDSINGLETON_H

#include "Interface/ISingleton.h"

#include <bitset>


using KeyStroke = uint16_t;

class KeyboardSingleton final: public ISingleton<KeyboardSingleton>
{
    friend class ISingleton<KeyboardSingleton>;
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
    bool IsKeyDown (_fox_In_ KeyStroke key) _fox_Pre_satisfies_(KeyStroke < KEY_BOUND)  const;
    _fox_Success_(return == true)
    bool operator[](_fox_In_ KeyStroke key) _fox_Pre_satisfies_(KeyStroke < KEY_BOUND)  const;

    void DebugKeysPressed() const;

private:
    KeyboardSingleton() = default;

private:
    static constexpr KeyStroke KEY_BOUND{ 256 };
    std::bitset<KEY_BOUND> m_btKeyStates{};
};

#endif //INPUTKEYBOARDSINGLETON_H
