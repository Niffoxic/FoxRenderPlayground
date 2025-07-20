//
// Created by niffo on 7/16/2025.
//

#ifndef ISYSTEM_H
#define ISYSTEM_H

#include "Common/Core.h"
#include "Common/ObjectID.h"


class __declspec(novtable) ISystem: public ObjectID
{
public:
    virtual ~ISystem() = default;

    virtual bool OnInit()    = 0;
    virtual bool OnRelease() = 0;
};

#endif //ISYSTEM_H
