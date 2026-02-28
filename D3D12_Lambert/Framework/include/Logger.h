//-----------------------------------------------------------------------------
// File : Logger.h
// Desc : Logger Module.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------
#pragma once


//-----------------------------------------------------------------------------
//! @brief      ログを出力します.
//!
//! @param[in]      format      フォーマットです.
//-----------------------------------------------------------------------------
void OutputLog(const char* format, ...);


#ifndef DLOG
    #if defined(DEBUG) || defined(_DEBUG)
        #define DLOG( x, ... ) OutputLog( x "\n", ##__VA_ARGS__ );
    #else
        #define DLOG( x, ... ) 
    #endif
#endif//DLOG

#ifndef ELOG
    #define ELOG( x, ... ) OutputLog( "[File : %s, Line : %d] " x "\n", __FILE__, __LINE__, ##__VA_ARGS__ )
#endif//ELOG
