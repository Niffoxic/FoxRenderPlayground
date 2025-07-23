//
// Created by niffo on 7/16/2025.
//

#ifndef DEFINEWINDOWS_H
#define DEFINEWINDOWS_H

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>

typedef struct WINDOW_SIZE_DESC
{
    UINT Width;
    UINT Height;

    RECT GetRect() const
    {
        return {0, 0, static_cast<LONG>(Width), static_cast<LONG>(Height) };
    }

    void FromRect(const RECT& rect)
    {
        Width = rect.right - rect.left;
        Height = rect.bottom - rect.top;
    }

}WINDOW_SIZE_DESC;

#endif //DEFINEWINDOWS_H
