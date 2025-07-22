//
// Created by niffo on 7/16/2025.
//

#ifndef OBJECTID_H
#define OBJECTID_H

#include "Core.h"
using ID = unsigned int;

class FObject
{
public:
    FObject(): m_id(++ID_ALLOCATOR)  {}
    _fox_Return_safe ID GetID() const { return m_id; }

protected:
    ~FObject() = default;

private:
    inline static ID ID_ALLOCATOR{ 0u };
    ID m_id;
};

#endif //OBJECTID_H
