//-----------------------------------------------------------------------------
// File : Logger.cpp
// Desc : Logger Module.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cstdio>
#include <cstdarg>
#include <Windows.h>


//-----------------------------------------------------------------------------
//      ログ出力します.
//-----------------------------------------------------------------------------
void OutputLog(const char* format, ...)
{
    char msg[2048];
    memset( msg, '\0', sizeof(msg) );
    va_list arg;

    va_start(arg, format);
    vsprintf_s(msg, format, arg);
    va_end(arg);

    // コンソールに出力.
    printf_s("%s", msg);

    // Visual Studioの出力ウィンドウにも表示.
    OutputDebugStringA(msg);
}