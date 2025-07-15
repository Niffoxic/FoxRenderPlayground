//
// Created by niffo on 7/16/2025.
//

#ifndef WINDOWSMANAGER_H
#define WINDOWSMANAGER_H

#include "Interface/IFrame.h"
#include "Interface/ISystem.h"

class WindowsManager final: public ISystem, public IFrame
{
public:
    WindowsManager() = default;
    ~WindowsManager() override;

    //~ Frame Interface Impl
    void OnFrameBegin() override;
    void OnFramePresent() override;
    void OnFrameEnd() override;

    //~ System Interface Impl
    bool OnInit() override;
    bool OnDelete() override;
};

#endif //WINDOWSMANAGER_H
