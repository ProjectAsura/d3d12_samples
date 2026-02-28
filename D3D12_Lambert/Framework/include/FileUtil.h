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

//-----------------------------------------------------------------------------
//! @brief      ディレクトリパスを削除し，ファイル名を返却します.
//!
//! @param[in]      path        ディスレクトリパスを取り除くファイルパス.
//! @return     ファイル名を返却します.
//-----------------------------------------------------------------------------
std::string RemoveDirectoryPathA(const std::string& path);

//-----------------------------------------------------------------------------
//! @brief      ディレクトリパスを削除し，ファイル名を返却します.
//!
//! @param[in]      path        ディスレクトリパスを取り除くファイルパス.
//! @return     ファイル名を返却します.
//-----------------------------------------------------------------------------
std::wstring RemoveDirectoryPathW(const std::wstring& path);

//-----------------------------------------------------------------------------
//! @brief      ディレクトリ名を取得します.
//!
//! @param[in]      filePath        ファイルパス.
//! @return     指定されたファイルパスからディレクトリ名を抜き出します.
//-----------------------------------------------------------------------------
std::string GetDirectoryPathA(const char* path);

//-----------------------------------------------------------------------------
//! @brief      ディレクトリ名を取得します.
//!
//! @param[in]      filePath        ファイルパス.
//! @return     指定されたファイルパスからディレクトリ名を抜き出します.
//-----------------------------------------------------------------------------
std::wstring GetDirectoryPathW(const wchar_t* path);


#if defined(UNICODE) || defined(_UNICODE)
    inline bool SearchFilePath(const wchar_t* filename, std::wstring& result)
    { return SearchFilePathW(filename, result); }

    inline std::wstring RemoveDirectoryPath(const std::wstring& path)
    { return RemoveDirectoryPathW(path); }

    inline std::wstring GetDirectoryPath(const wchar_t* path)
    { return GetDirectoryPathW(path); }
#else
    inline bool SearchFilePath(const char* filename, std::string& result)
    { return SearchFilePathA(filename, result); }

    inline std::string RemoveDirectoryPath(const std::string& path)
    { return RemoveDirectoryPathA(path); }

    inline std::string GetDirectoryPath(const char* path)
    { return GetDirectoryPathA(path); }
#endif//defined(UNICODE) || defined(_UNICODE)
