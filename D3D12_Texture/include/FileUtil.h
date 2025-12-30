//-----------------------------------------------------------------------------
// File : FileUtil.h
// Desc : File Utility.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <string>
#include <Shlwapi.h>

//-----------------------------------------------------------------------------
// Linker
//-----------------------------------------------------------------------------
#pragma comment( lib, "shlwapi.lib ")


//-----------------------------------------------------------------------------
//! @brief      ファイルパスを検索します.
//!
//! @param[in]      filePath        検索するファイスパス.
//! @param[out]     result          検索結果の格納先.
//! @retval true    ファイルを発見.
//! @retval false   ファイルが見つからなかった.
//! @memo 検索ルールは以下の通り.
//!      .\
//!      ..\
//!      ..\..\
//!      .\res\
//!      %EXE_DIR%\
//!      %EXE_DIR%\..\
//!      %EXE_DIR%\..\..\
//!      %EXE_DIR%\res\
//-----------------------------------------------------------------------------
bool SearchFilePathA(const char* filename, std::string& result);

//-----------------------------------------------------------------------------
//! @brief      ファイルパスを検索します.
//!
//! @param[in]      filePath        検索するファイスパス.
//! @param[out]     result          検索結果の格納先.
//! @retval true    ファイルを発見.
//! @retval false   ファイルが見つからなかった.
//! @memo 検索ルールは以下の通り.
//!      .\
//!      ..\
//!      ..\..\
//!      .\res\
//!      %EXE_DIR%\
//!      %EXE_DIR%\..\
//!      %EXE_DIR%\..\..\
//!      %EXE_DIR%\res\
//-----------------------------------------------------------------------------
bool SearchFilePathW(const wchar_t* filename, std::wstring& result);


#if defined(UNICODE) || defined(_UNICODE)
    inline bool SearchFilePath(const wchar_t* filename, std::wstring& result)
    { return SearchFilePathW(filename, result); }
#else
    inline bool SearchFilePath(const char* filename, std::string& result)
    { return SearchFilePathA(filename, result); }
#endif//defined(UNICODE) || defined(_UNICODE)
