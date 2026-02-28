//-------------------------------------------------------------------------------------------------
// File : asdxLogger.h
// Desc : Logger Module.
// Copyright(c) Project Asura. All right reserved.
//-------------------------------------------------------------------------------------------------
#pragma once

//-------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------
#include <cstdint>


namespace asdx {

#ifndef __ASDX_WIDE
#define __ASDX_WIDE( _string )      L ## _string
#endif//__ASDX_WIDE


#ifndef ASDX_WIDE
#define ASDX_WIDE( _string )        __ASDX_WIDE( _string )
#endif//ASDX_WIDE

///////////////////////////////////////////////////////////////////////////////////////////////////
// LOG_LEVEL enum
///////////////////////////////////////////////////////////////////////////////////////////////////
enum LOG_LEVEL : uint32_t
{
    LOG_VERBOSE = 0,          //!< VERBOSEレベル (白).
    LOG_INFO,                 //!< INFOレベル    (緑).
    LOG_DEBUG,                //!< DEBUGレベル   (青).
    LOG_WARNING,              //!< WARNINGレベル (黄).
    LOG_ERROR,                //!< ERRORレベル   (赤).
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// ILogger interface
///////////////////////////////////////////////////////////////////////////////////////////////////
struct ILogger
{
    //---------------------------------------------------------------------------------------------
    //! @brief      ログを出力します.
    //!
    //! @param[in]      level       ログレベルです.
    //! @param[in]      format      フォーマットです.
    //---------------------------------------------------------------------------------------------
    virtual void WriteA(LOG_LEVEL level, const char* format, ... ) = 0;

    //---------------------------------------------------------------------------------------------
    //! @brief      ログを出力します.
    //!
    //! @param[in]      level       ログレベルです.
    //! @param[in]      format      フォーマットです.
    //---------------------------------------------------------------------------------------------
    virtual void WriteW(LOG_LEVEL level, const wchar_t* format, ... ) = 0;

    //---------------------------------------------------------------------------------------------
    //! @brief      フィルタを設定します.
    //!
    //! @param[in]      filter      設定するフィルタ.
    //---------------------------------------------------------------------------------------------
    virtual void SetFilter(LOG_LEVEL filter ) = 0;

    //---------------------------------------------------------------------------------------------
    //! @brief      設定されているフィルタを取得します.
    //!
    //! @return     設定されているフィルタを取得します.
    //---------------------------------------------------------------------------------------------
    virtual LOG_LEVEL GetFilter() = 0;
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// SystemLogger class
///////////////////////////////////////////////////////////////////////////////////////////////////
class SystemLogger : public ILogger
{
    //=============================================================================================
    // list of friend classes and methods.
    //=============================================================================================
    /* NOTHING */

public:
    //=============================================================================================
    // public variables.
    //=============================================================================================
    /* NOTHING */

    //=============================================================================================
    // public methods.
    //=============================================================================================

    //---------------------------------------------------------------------------------------------
    //! @brief      唯一のインスタンスを取得します.
    //!
    //! @return     シングルトンインスタンスを返却します.
    //---------------------------------------------------------------------------------------------
    static SystemLogger& Instance();

    //---------------------------------------------------------------------------------------------
    //! @brief      ログを出力します.
    //!
    //! @param[in]      level       ログレベルです.
    //! @param[in]      format      フォーマットです.
    //---------------------------------------------------------------------------------------------
    void WriteA(LOG_LEVEL level, const char* format, ... ) override;

    //---------------------------------------------------------------------------------------------
    //! @brief      ログを出力します.
    //!
    //! @param[in]      level       ログレベルです.
    //! @param[in]      format      フォーマットです.
    //---------------------------------------------------------------------------------------------
    void WriteW(LOG_LEVEL level, const wchar_t* format, ... ) override;

    //---------------------------------------------------------------------------------------------
    //! @brief      フィルタを設定します.
    //!             フィルタを設定したもののみが，ログに出力されるようになります.
    //!
    //! @param[in]      filter      設定するフィルタ.
    //---------------------------------------------------------------------------------------------
    void SetFilter(LOG_LEVEL filter ) override;

    //---------------------------------------------------------------------------------------------
    //! @brief      設定されているフィルタを取得します.
    //!
    //! @return     設定されているフィルタを取得します.
    //---------------------------------------------------------------------------------------------
    LOG_LEVEL GetFilter() override;

protected:
    //=============================================================================================
    // protected variables.
    //=============================================================================================
    /* NOTHING */

    //=============================================================================================
    // protected methods.
    //=============================================================================================
    /* NOTHING */

private:
    //=============================================================================================
    // private variables.
    //=============================================================================================
    static SystemLogger     s_Instance;     //!< シングルトンインスタンスです.
    LOG_LEVEL               m_Filter;       //!< フィルターです.

    //=============================================================================================
    // private methods.
    //=============================================================================================
    SystemLogger();
    SystemLogger             (const SystemLogger&) = delete;
    SystemLogger& operator = (const SystemLogger&) = delete;
};

} // namespace asdx


//-------------------------------------------------------------------------------------------------
// Macros
//-------------------------------------------------------------------------------------------------
#ifndef DLOGA
  #if defined(DEBUG) || defined(_DEBUG)
    #define DLOGA( fmt, ... )      asdx::SystemLogger::Instance().WriteA( asdx::LOG_DEBUG, "[File: %s, Line: %d] "fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__ )
  #else
    #define DLOGA( fmt, ... )      ((void)0)
  #endif//defined(DEBUG) || defined(_DEBUG)
#endif//DLOGA

#ifndef DLOGW
  #if defined(DEBUG) || defined(_DEBUG)
    #define DLOGW( fmt, ... )      asdx::SystemLogger::Instance().WriteW( asdx::LOG_DEBUG, ASDX_WIDE("[File: %s, Line: %d] ") ASDX_WIDE(fmt) ASDX_WIDE("\n"), ASDX_WIDE(__FILE__), __LINE__, ##__VA_ARGS__ )
  #else
    #define DLOGW( fmt, ... )      ((void)0)
  #endif//defined(DEBUG) || defined(_DEBUG)
#endif//DLOGW


#ifndef VLOGA
#define VLOGA( fmt, ... )   asdx::SystemLogger::Instance().WriteA( asdx::LOG_VERBOSE, fmt "\n", ##__VA_ARGS__ )
#endif//VLOGA

#ifndef VLOGW
#define VLOGW( fmt, ... )   asdx::SystemLogger::Instance().WriteW( asdx::LOG_VERBOSE, ASDX_WIDE(fmt) ASDX_WIDE("\n"), ##__VA_ARGS__ )
#endif//VLOGW

#ifndef ILOGA
#define ILOGA( fmt, ... )   asdx::SystemLogger::Instance().WriteA( asdx::LOG_INFO, fmt "\n", ##__VA_ARGS__ )
#endif//ILOGA

#ifndef ILOGW
#define ILOGW( fmt, ... )   asdx::SystemLogger::Instance().WriteW( asdx::LOG_INFO, ASDX_WIDE(fmt) ASDX_WIDE("\n"), ##__VA_ARGS__ );
#endif//ILOGW

#ifndef WLOGA
#define WLOGA( fmt, ... )   asdx::SystemLogger::Instance().WriteA( asdx::LOG_WARNING, fmt "\n", ##__VA_ARGS__ )
#endif//WLOGA

#ifndef WLOGW
#define WLOGW( fmt, ... )   asdx::SystemLogger::Instance().WriteW( asdx::LOG_WARNING, ASDX_WIDE(fmt) ASDX_WIDE("\n"), ##__VA_ARGS__ )
#endif//WLOGW

#ifndef ELOGA
#define ELOGA( fmt, ... )   asdx::SystemLogger::Instance().WriteA( asdx::LOG_ERROR, "[File: %s, Line: %d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__ )
#endif//ELOGA

#ifndef ELOGW
#define ELOGW( fmt, ... )   asdx::SystemLogger::Instance().WriteW( asdx::LOG_ERROR, ASDX_WIDE("[File: %s, Line: %d] ") ASDX_WIDE(fmt) ASDX_WIDE("\n"), ASDX_WIDE(__FILE__), __LINE__, ##__VA_ARGS__ )
#endif//ELOGW

#if defined(UNICODE) || defined(_UNICODE)
    #define VLOG        VLOGW
    #define DLOG        DLOGW
    #define ILOG        ILOGW
    #define WLOG        WLOGW
    #define ELOG        ELOGW
#else
    #define VLOG        VLOGA 
    #define DLOG        DLOGA
    #define ILOG        ILOGA
    #define WLOG        WLOGA 
    #define ELOG        ELOGA
#endif

