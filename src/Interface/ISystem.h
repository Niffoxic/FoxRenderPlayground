//
// Created by niffo on 7/16/2025.
//

#ifndef ISYSTEM_H
#define ISYSTEM_H

#include "Common/ObjectID.h"

class ISystem: public ObjectID
{
public:
    virtual ~ISystem() = default;

    virtual bool OnInit() = 0;
    virtual bool OnRelease() = 0;
};

#endif //ISYSTEM_H
