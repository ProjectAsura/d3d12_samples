//-----------------------------------------------------------------------------
// File : asdxLogger.h
// Desc : Logger Module.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cstdio>
#include <cstdarg>
#include <Windows.h>
#include <fnd/asdxLogger.h>


namespace /* anonymous */ {

///////////////////////////////////////////////////////////////////////////////
// ConsoleColor class
///////////////////////////////////////////////////////////////////////////////
class ConsoleColor
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    /* NOTHING */

public:
    //=========================================================================
    // public variables.
    //=========================================================================
    /* NOTHING */

    //=========================================================================
    // public methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //!
    //! @param[in]      level       ログレベルです.
    //-------------------------------------------------------------------------
    explicit ConsoleColor(asdx::LOG_LEVEL level)
    {
        const auto handle = GetStdHandle( STD_OUTPUT_HANDLE );
        GetConsoleScreenBufferInfo( handle, &m_Info );

        auto attribute = m_Info.wAttributes;
        switch( level )
        {
        case asdx::LOG_VERBOSE:
            attribute = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
            break;

        case asdx::LOG_INFO:
            attribute = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;

        case asdx::LOG_DEBUG:
            attribute = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            break;

        case asdx::LOG_WARNING:
            attribute = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
            break;

        case asdx::LOG_ERROR:
            attribute = FOREGROUND_RED | FOREGROUND_INTENSITY;
            break;
        }

        SetConsoleTextAttribute( handle, attribute );
    }

    //-------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //-------------------------------------------------------------------------
    ~ConsoleColor()
    {
        const auto handle = GetStdHandle( STD_OUTPUT_HANDLE );
        SetConsoleTextAttribute( handle, m_Info.wAttributes );
    }

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    CONSOLE_SCREEN_BUFFER_INFO  m_Info = {};

    //=========================================================================
    // private methods.
    //=========================================================================
    /* NOTHING */
};

}// namespace /* anonymous */


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// SystemLogger class
///////////////////////////////////////////////////////////////////////////////
SystemLogger SystemLogger::s_Instance;

//-----------------------------------------------------------------------------
//      コンストラクタです
//-----------------------------------------------------------------------------
SystemLogger::SystemLogger()
: m_Filter(LOG_VERBOSE)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      インスタンスを取得します.
//-----------------------------------------------------------------------------
SystemLogger& SystemLogger::Instance()
{ return s_Instance; }

//-----------------------------------------------------------------------------
//      ログを出力します.
//-----------------------------------------------------------------------------
void SystemLogger::WriteA(LOG_LEVEL level, const char* format, ... )
{
    if ( level >= m_Filter )
    {
        ConsoleColor color(level);
        // ログ出力.
        {
            char msg[ 2048 ] = "\0";
            va_list arg;

            va_start( arg, format );
            vsprintf_s( msg, format, arg );
            va_end( arg );

            fprintf((level == LOG_ERROR ? stderr : stdout), "%s", msg );

            OutputDebugStringA( msg );
        }
    }
}

//-----------------------------------------------------------------------------
//      ログを出力します.
//-----------------------------------------------------------------------------
void SystemLogger::WriteW(LOG_LEVEL level, const wchar_t* format, ... )
{
    if ( level >= m_Filter )
    {
        ConsoleColor color(level);
        // ログ出力.
        {
            wchar_t msg[ 2048 ] = L"\0";
            va_list arg;

            va_start( arg, format );
            vswprintf_s( msg, format, arg );
            va_end( arg );

            fwprintf_s((level == LOG_ERROR ? stderr : stdout), L"%s", msg );

            OutputDebugStringW( msg );
        }
    }
}

//-----------------------------------------------------------------------------
//      フィルタを設定します.
//-----------------------------------------------------------------------------
void SystemLogger::SetFilter( const LOG_LEVEL filter )
{ m_Filter = filter; }

//-----------------------------------------------------------------------------
//      設定されているフィルタを取得します.
//-----------------------------------------------------------------------------
LOG_LEVEL SystemLogger::GetFilter()
{ return m_Filter; }

} // namespace asdx
