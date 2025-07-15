//
// Created by niffo on 7/16/2025.
//

#ifndef OBJECTID_H
#define OBJECTID_H

using ID = unsigned int;

class ObjectID
{
public:
    ObjectID(): m_id(++ID_ALLOCATOR) {}
    ID GetID() const { return m_id; }

protected:
    ~ObjectID() = default;

private:
    inline static ID ID_ALLOCATOR{ 0u };
    ID m_id;
};

#endif //OBJECTID_H
