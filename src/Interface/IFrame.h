//
// Created by niffo on 7/16/2025.
//

#ifndef IFRAME_H
#define IFRAME_H

#include "Common/ObjectID.h"

class IFrame: public ObjectID
{
public:
    virtual ~IFrame() = default;

    virtual void OnFrameBegin() = 0;
    virtual void OnFramePresent() = 0;
    virtual void OnFrameEnd() = 0;
};

#endif //IFRAME_H
