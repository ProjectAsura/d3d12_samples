//-----------------------------------------------------------------------------
// File : asdxBinary.h
// Desc : Binary Utility.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cstdint>
#include <cassert>


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// OffsetPtr class
///////////////////////////////////////////////////////////////////////////////
template<typename T>
class OffsetPtr
{
public:
    OffsetPtr() 
    : m_Offset(0)
    { /* DO_NOTHING */ }

    Offset(int offset)
    : m_Offset(offset)
    { /* DO_NOTHING */ }

    void setOffset(int offset)
    { m_Offset = offset; }

    T* get()
    { return (m_Offset == 0) ? nullptr : reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(this) + m_Offset); }

    const T* get() const
    { return (m_Offset == 0) ? nullptr : reinterpret_cast<const T*>(reinterpret_cast<const uint8_t*>(this) + m_Offset); }

    const T* operator -> () const { return get(); }
    const T* operator * () const { return get(); }

private:
    int m_Offset = 0;

    OffsetPtr             (const OffsetPtr&) = delete;
    OffsetPtr& operator = (const OffsetPtr&) = delete;
};

///////////////////////////////////////////////////////////////////////////////
// BinaryHeader structure
///////////////////////////////////////////////////////////////////////////////
struct BinaryHeader
{
    uint8_t     Magic[4]    = {};
    uint32_t    Version     = {};

    bool CheckMagic(uint8_t a, uint8_t b, uint8_t c, uint8_t d) const
    {
        return Magic[0] == a
            && Magic[1] == b
            && Magic[2] == c
            && Magic[3] == d;
    }

    bool CheckVersion(uint32_t version) const 
    { return Version == version; }
};

///////////////////////////////////////////////////////////////////////////////
// BinaryFile structure
///////////////////////////////////////////////////////////////////////////////
struct BinayFile : public BinaryHeader
{
    void*   Contents = nullptr;

    template<typename T>
    T* GetContentsAs() const
    { return reinterpret_cast<T*>(Contents); }
};

///////////////////////////////////////////////////////////////////////////////
// BinaryArray class
///////////////////////////////////////////////////////////////////////////////
template<typename T>
class BinaryArrary
{
public:
    bool empty() const
    { return m_Size == 0; }

    int size() const
    { return m_Size; }

    T* data()
    { return m_Data.get(); }

    const T* data() const
    { return m_Data.get(); }

    T& at(int index)
    {
        assert(0 <= index && index < m_Size);
        return m_Data.get()[index];
    }

    const T& at(int index) const
    {
        assert(0 <= index && index < m_Size);
        return m_Data.get()[index];
    }

    T& operator[](int index)
    {
        assert(0 <= index && index < m_Size);
        return m_Data.get()[index];
    }

    const T& operator[](int index) const
    {
        assert(0 <= index && index < m_Size);
        return m_Data.get()[index];
    }

    T* begin()
    { return m_Data.get(); }

    T* end()
    { return &m_Data.get()[m_Size]; }

    const T* begin() const
    { return m_Data.get(); }

    const T* end() const
    { return &m_Data.get()[m_Size]; }

    void SetSize(int size)
    { m_Size = size; }

    void SetOffset(int offset)
    { m_Data.SetOffset(offset); }

protected:
    int             m_Size = 0;
    OffsetPtr<T>    m_Data;
};

} // namespace asdx
