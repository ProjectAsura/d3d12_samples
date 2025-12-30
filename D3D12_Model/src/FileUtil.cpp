//-----------------------------------------------------------------------------
// File : FileUtil.cpp
// Desc : File Utility.
// Copyright(c) Pocol. All right reserved
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "FileUtil.h"


//-----------------------------------------------------------------------------
//      ファイルパスを検索します.
//-----------------------------------------------------------------------------
bool SearchFilePathW(const wchar_t* filename, std::wstring& result)
{
    if (filename == nullptr)
    {
        return false;
    }

    if (wcscmp(filename, L" ") == 0 || wcscmp(filename, L"") == 0)
    {
        return false;
    }

    wchar_t exePath[520] = { 0 };
    GetModuleFileNameW(nullptr, exePath, 520);
    exePath[519] = 0; // null終端化.
    PathRemoveFileSpecW(exePath);

    wchar_t dstPath[520] = { 0 };

    wcscpy_s(dstPath, filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    swprintf_s(dstPath, L"..\\%s", filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    swprintf_s(dstPath, L"..\\..\\%s", filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    swprintf_s(dstPath, L"\\res\\%s", filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    swprintf_s(dstPath, L"%s\\%s", exePath, filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    swprintf_s(dstPath, L"%s\\..\\%s", exePath, filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    swprintf_s(dstPath, L"%s\\..\\..\\%s", exePath, filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    swprintf_s(dstPath, L"%s\\res\\%s", exePath, filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
//      ファイルパスを検索します.
//-----------------------------------------------------------------------------
bool SearchFilePathA(const char* filename, std::string& result)
{
    if (filename == nullptr)
    {
        return false;
    }

    if (strcmp(filename, " ") == 0 || strcmp(filename, "") == 0)
    {
        return false;
    }

    char exePath[520] = { 0 };
    GetModuleFileNameA(nullptr, exePath, 520);
    exePath[519] = 0; // null終端化.
    PathRemoveFileSpecA(exePath);

    char dstPath[520] = { 0 };

    strcpy_s(dstPath, filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    sprintf_s(dstPath, "..\\%s", filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    sprintf_s(dstPath, "..\\..\\%s", filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    sprintf_s(dstPath, "\\res\\%s", filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    sprintf_s(dstPath, "%s\\%s", exePath, filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    sprintf_s(dstPath, "%s\\..\\%s", exePath, filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    sprintf_s(dstPath, "%s\\..\\..\\%s", exePath, filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    sprintf_s(dstPath, "%s\\res\\%s", exePath, filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
//      ディレクトリパスを削除し，ファイル名を返却します.
//-----------------------------------------------------------------------------
std::string RemoveDirectoryPathA(const std::string& path)
{
    auto pos = path.rfind('/');

    if (pos != std::string::npos)
    { return path.substr(pos + 1); }

    pos = path.rfind('\\');
    if (pos != std::string::npos)
    { return path.substr(pos + 1); }

    return path;
}

//-----------------------------------------------------------------------------
//      ディレクトリパスを削除し，ファイル名を返却します.
//-----------------------------------------------------------------------------
std::wstring RemoveDirectoryPathW(const std::wstring& path)
{
    auto pos = path.rfind(L'/');

    if (pos != std::wstring::npos)
    { return path.substr(pos + 1); }

    pos = path.rfind(L'\\');
    if (pos != std::wstring::npos)
    { return path.substr(pos + 1); }

    return path;
}
