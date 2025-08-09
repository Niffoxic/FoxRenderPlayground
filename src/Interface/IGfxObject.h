//
// Created by niffo on 8/4/2025.
//

#ifndef IGFXOBJECT_H
#define IGFXOBJECT_H

#include <vulkan/vulkan.h>
#include "Common/Core.h"

class NOVTABLE IGfxObject
{
public:
    virtual ~IGfxObject() = default;

    virtual bool Init()    = 0;
    virtual void Release() = 0;
};

#endif //IGFXOBJECT_H
