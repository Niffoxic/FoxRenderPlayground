//
// Created by niffo on 8/4/2025.
//

#ifndef IGFXOBJECT_H
#define IGFXOBJECT_H

#include "Common/Core.h"

class _NOVTABLE IGfxObject
{
public:
    virtual ~IGfxObject() = default;

    virtual bool Init()    = 0;
    virtual void Release() = 0;
};

#endif //IGFXOBJECT_H
