//-----------------------------------------------------------------------------
// File : asdxHash.h
// Desc : Hash Key Module.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cstdint>
#include <cstring>

namespace asdx {

//-----------------------------------------------------------------------------
//! @brief      FNV1によるハッシュ値を計算します.
//! 
//! @param[in]      buffer      バッファ.
//! @param[in]      size        バッファサイズ.
//! @return     ハッシュ値を返却します.
//-----------------------------------------------------------------------------
inline uint32_t CalcHash(const uint8_t* buffer, uint32_t size)
{
    const uint32_t kOffset  = 2166136261;
    const uint32_t kPrime   = 16777619;

    auto hash = kOffset;
    for(auto i=0u; i<size; ++i)
    { hash = (kPrime * hash) ^ buffer[i]; }

    return hash;
}

//-----------------------------------------------------------------------------
//! @brief      FNV1によるハッシュ値を計算します.
//! 
//! @param[in]      buffer      文字列.
//! @return     ハッシュ値を返却します.
//-----------------------------------------------------------------------------
inline uint32_t CalcHash(const char* buffer)
{
    const uint32_t kOffset  = 2166136261;
    const uint32_t kPrime   = 16777619;

    auto hash = kOffset;
    auto size = strlen(buffer);
    for(auto i=0u; i<size; ++i)
    { hash = (kPrime * hash) ^ buffer[i]; }

    return hash;
}

} // namespace asdx
