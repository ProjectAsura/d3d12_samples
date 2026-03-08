//-----------------------------------------------------------------------------
// File : asdxBit.cpp
// Desc : Bit Operations.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <fnd/asdxBit.h>

#if _HAS_CXX20
  #include <bit>      // for std::countl_zero, std::countr_zero
#elif defined(__clang__) || defined(__GNUC__)
   
#elif defined(_MSC_VER)
  #include <intrin.h> // for _BitScanReverse, _BitScanForward
#endif


namespace asdx {
namespace impl {

static int CountBit8(uint8_t v)
{
    uint8_t count = v;
    count = (count & 0x55) + ((count >> 1) & 0x55);
    count = (count & 0x33) + ((count >> 2) & 0x33);
    count = (count & 0x0f) + ((count >> 4) & 0x0f);
    return int(count);
}

static int CountBit16(uint16_t v)
{
    uint16_t count = v;
    count = (count & 0x5555) + ((count >> 1) & 0x5555);
    count = (count & 0x3333) + ((count >> 2) & 0x3333);
    count = (count & 0x0f0f) + ((count >> 4) & 0x0f0f);
    count = (count & 0x00ff) + ((count >> 8) & 0x00ff);
    return int(count);
}

static int CountBit32(uint32_t v)
{
    uint32_t count = v;
    count = (count & 0x55555555) + ((count >>  1) & 0x55555555);
    count = (count & 0x33333333) + ((count >>  2) & 0x33333333);
    count = (count & 0x0f0f0f0f) + ((count >>  4) & 0x0f0f0f0f);
    count = (count & 0x00ff00ff) + ((count >>  8) & 0x00ff00ff);
    count = (count & 0x0000ffff) + ((count >> 16) & 0x0000ffff);
    return int(count);
}

static int CountBit64(uint64_t v)
{
    uint64_t count = v;
    count = (count & 0x5555555555555555) + ((count >>  1) & 0x5555555555555555);
    count = (count & 0x3333333333333333) + ((count >>  2) & 0x3333333333333333);
    count = (count & 0x0f0f0f0f0f0f0f0f) + ((count >>  4) & 0x0f0f0f0f0f0f0f0f);
    count = (count & 0x00ff00ff00ff00ff) + ((count >>  8) & 0x00ff00ff00ff00ff);
    count = (count & 0x0000ffff0000ffff) + ((count >> 16) & 0x0000ffff0000ffff);
    count = (count & 0x00000000ffffffff) + ((count >> 32) & 0x00000000ffffffff);
    return int(count);
}

static int CountZeroL(uint8_t value)
{
    value |= (value >> 1);
    value |= (value >> 2);
    value |= (value >> 4);
    return CountBit8(~value); 
}

static int CountZeroL(uint16_t value)
{
    value |= (value >> 1);
    value |= (value >> 2);
    value |= (value >> 4);
    value |= (value >> 8);
    return CountBit16(~value);
}

static int CountZeroL(uint32_t value)
{
    value |= (value >> 1);
    value |= (value >> 2);
    value |= (value >> 4);
    value |= (value >> 8);
    value |= (value >> 16);
    return CountBit32(~value);
}

static int CountZeroL(uint64_t value)
{
    value |= (value >> 1);
    value |= (value >> 2);
    value |= (value >> 4);
    value |= (value >> 8);
    value |= (value >> 16);
    value |= (value >> 32);
    return CountBit64(~value);
}

static int CountZeroR(uint8_t  value) { return CountBit8 ((~value) & (value - 1)); }
static int CountZeroR(uint16_t value) { return CountBit16((~value) & (value - 1)); }
static int CountZeroR(uint32_t value) { return CountBit32((~value) & (value - 1)); }
static int CountZeroR(uint64_t value) { return CountBit64((~value) & (value - 1)); }

} // namespace impl

#if !_HAS_CXX20

int CountBit  (uint8_t value) { return impl::CountBit8 (value); }
int CountZeroL(uint8_t value) { return impl::CountZeroL(value); }
int CountZeroR(uint8_t value) { return impl::CountZeroR(value); }

#endif

#if _HAS_CXX20
// C++ 20.

int CountBit(uint8_t  value) { return std::popcount(value); }
int CountBit(uint16_t value) { return std::popcount(value); }
int CountBit(uint32_t value) { return std::popcount(value); }
int CountBit(uint64_t value) { return std::popcount(value); }

int CountZeroL(uint8_t  value) { return std::countl_zero(value); }
int CountZeroL(uint16_t value) { return std::countl_zero(value); }
int CountZeroL(uint32_t value) { return std::countl_zero(value); }
int CountZeroL(uint64_t value) { return std::countl_zero(value); }

int CountZeroR(uint8_t  value) { return std::countr_zero(value); }
int CountZeroR(uint16_t value) { return std::countr_zero(value); }
int CountZeroR(uint32_t value) { return std::countr_zero(value); }
int CountZeroR(uint64_t value) { return std::countr_zero(value); }

#elif defined(__clang__) || defined(__GNUC__)
// GCC or clang.

int CountBit(uint8_t  value) { return __builtin_popcount(value); }
int CountBit(uint16_t value) { return __builtin_popcount(value); }
int CountBit(uint32_t value) { return __builtin_popcount(value); }
int CountBit(uint64_t value) { return __builtin_popcountll(value); }

int CountZeroL(uint16_t value) { return __builtin_clzs(value); }
int CountZeroL(uint32_t value) { return __builtin_clz(value); }
int CountZeroL(uint64_t value) { return __builtin_clzll(value); }

int CountZeroR(uint16_t value) { return __builtin_ctzs(value); }
int CountZeroR(uint32_t value) { return __builtin_ctz(value); }
int CountZeroR(uint64_t value) { return __builtin_ctzll(value); }

#elif defined(_MSC_VER)
// Microsoft Visual Studio.

int CountBit(uint16_t value) { return int(__popcnt16(value)); }
int CountBit(uint32_t value) { return int(__popcnt(value)); }
int CountBit(uint64_t value) { return int(__popcnt64(value)); }

int CountZeroL(uint16_t value) { return int(__lzcnt16(value)); }
int CountZeroL(uint32_t value) { return int(__lzcnt(value)); }
int CountZeroL(uint64_t value) { return int(__lzcnt64(value)); }

int CountZeroR(uint16_t value) { return int(_tzcnt_u16(value)); }
int CountZeroR(uint32_t value) { return int(_tzcnt_u32(value)); }
int CountZeroR(uint64_t value) { return int(_tzcnt_u64(value)); }

#else
// Fallback.

int CountBit(uint16_t value) { return impl::CountBit16(value); }
int CountBit(uint32_t value) { return impl::CountBit32(value); }
int CountBit(uint64_t value) { return impl::CountBit64(value); }

int CountZeroL(uint16_t value) { return impl::CountZeroL(value); }
int CountZeroL(uint32_t value) { return impl::CountZeroL(value); }
int CountZeroL(uint64_t value) { return impl::CountZeroL(value); }

int CountZeroR(uint16_t value) { return impl::CountZeroR(value); }
int CountZeroR(uint32_t value) { return impl::CountZeroR(value); }
int CountZeroR(uint64_t value) { return impl::CountZeroR(value); }

#endif

static_assert(sizeof(BitFlag8)  == sizeof(uint8_t) , "BitFlag8  Size Not Match");
static_assert(sizeof(BitFlag16) == sizeof(uint16_t), "BitFlag16 Size Not Match");
static_assert(sizeof(BitFlag32) == sizeof(uint32_t), "BitFlag32 Size Not Match");
static_assert(sizeof(BitFlag64) == sizeof(uint64_t), "BitFlag64 Size Not Match");


} // namespace asdx
