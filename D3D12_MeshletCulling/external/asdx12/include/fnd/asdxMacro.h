//-----------------------------------------------------------------------------
// File : asdxMacro.h
// Desc : Macro
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

#if defined(DEBUG) || defined(_DEBUG)
    #define ASDX_DEBUG      (1)
#else
    #define ASDX_RELEASE    (1)
#endif

#ifndef ASDX_UNUSED
#define ASDX_UNUSED(x)  (void)x
#endif//ASDX_UNUSED

#ifndef ASDX_DEBUG_CODE
    #ifdef ASDX_DEBUG
        #define ASDX_DEBUG_CODE(x)  x
    #else
        #define ASDX_DEBUG_CODE(x)
    #endif
#endif

#ifndef ASDX_COUNT_OF
#define ASDX_COUNT_OF(arr) (sizeof(arr) / sizeof(arr[0]))
#endif//ASDX_COUNT_OF

#ifndef ASDX_DEV_VAR
    #ifndef ASDX_RELEASE
        #define ASDX_DEV_VAR(develop, release) develop
    #else
        #define ASDX_DEV_VAR(develop, release) release
    #endif
#endif

