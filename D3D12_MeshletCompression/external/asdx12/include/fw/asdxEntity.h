//-----------------------------------------------------------------------------
// File : asdxEntity.h
// Desc : Entity
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <map>


namespace asdx {

struct IComponent
{
    virtual ~IComponent() {}
    virtual void Update() {}
    virtual void Draw  () {}
};

class Entity
{
public:
    bool        Add     (uint32_t id, IComponent* item);
    bool        Remove  (uint32_t id);
    IComponent* Get     (uint32_t id) const;
    void        Update  ();
    void        Draw    ();

private:
    std::map<uint32_t, IComponent*> m_Components;
};

} // namespace asdx