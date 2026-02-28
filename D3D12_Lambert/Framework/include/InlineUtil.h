//-----------------------------------------------------------------------------
// File : InlineUtil.h
// Desc : Inline Utility.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------
#pragma once


//-----------------------------------------------------------------------------
//      nullptr を考慮して delete 処理を行います.
//-----------------------------------------------------------------------------
template<typename T>
inline void SafeDelete(T*& ptr)
{
    if (ptr != nullptr)
    {
        delete ptr;
        ptr = nullptr;
    }
}

//-----------------------------------------------------------------------------
//      nullptr を考慮して delete[] 処理を行います.
//-----------------------------------------------------------------------------
template<typename T>
inline void SafeDeleteArray(T*& ptr)
{
    if (ptr != nullptr)
    {
        delete[] ptr;
        ptr = nullptr;
    }
}

//-----------------------------------------------------------------------------
//      nullptr を考慮して Release() メソッドを呼び出します.
//-----------------------------------------------------------------------------
template<typename T>
inline void SafeRelease(T*& ptr)
{
    if (ptr != nullptr)
    {
        ptr->Release();
        ptr = nullptr;
    }
}

//-----------------------------------------------------------------------------
//      nullptr を考慮して Term() メソッドを呼び出し, delete 処理を行います.
//-----------------------------------------------------------------------------
template<typename T>
inline void SafeTerm(T*& ptr)
{
    if (ptr != nullptr)
    {
        ptr->Term();
        delete ptr;
        ptr = nullptr;
    }
}