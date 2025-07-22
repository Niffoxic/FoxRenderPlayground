//
// Created by niffo on 7/16/2025.
//

#ifndef ISYSTEM_H
#define ISYSTEM_H

#include "Common/Core.h"
#include "Common/FObject.h"

#define FOX_SYSTEM_GENERATOR(CLASS_NAME)     \
public:                                      \
    FString GetSystemName() const override   \
    {                                        \
        return F_TEXT(#CLASS_NAME);          \
    }


class _NOVTABLE ISystem: public FObject
{
public:
    virtual ~ISystem() = default;

    virtual bool OnInit       () = 0;
    virtual void OnUpdateStart() = 0;
    virtual void OnUpdateEnd  () = 0;
    virtual void OnRelease    () = 0;
    virtual FString GetSystemName() const = 0;
};

#endif //ISYSTEM_H
