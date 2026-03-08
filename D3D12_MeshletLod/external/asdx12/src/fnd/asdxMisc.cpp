//-----------------------------------------------------------------------------
// File : asdxMisc.cpp
// Desc : Utility Module.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <shlwapi.h>
#include <locale>
#include <codecvt>
#include <sstream>
#include <direct.h>
#include <ShObjIdl.h>
#include <dxgiformat.h>
#include <fnd/asdxLogger.h>
#include <fnd/asdxMisc.h>


namespace asdx {

//-----------------------------------------------------------------------------
//      ファイルパスを検索します.
//-----------------------------------------------------------------------------
bool SearchFilePathW( const wchar_t* filePath, std::wstring& result )
{
    if ( filePath == nullptr )
    { return false; }

    if ( wcscmp( filePath, L" " ) == 0 || wcscmp( filePath, L"" ) == 0 )
    { return false; }

    wchar_t exePath[ 520 ] = { 0 };
    GetModuleFileNameW( nullptr, exePath, 520  );
    exePath[ 519 ] = 0; // null終端化.
    PathRemoveFileSpecW( exePath );

    wchar_t dstPath[ 520 ] = { 0 };

    wcscpy_s( dstPath, filePath );
    if ( PathFileExistsW( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    swprintf_s( dstPath, L"..\\%s", filePath );
    if ( PathFileExistsW( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    swprintf_s( dstPath, L"..\\..\\%s", filePath );
    if ( PathFileExistsW( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    swprintf_s( dstPath, L"\\res\\%s", filePath );
    if ( PathFileExistsW( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    swprintf_s( dstPath, L"%s\\%s", exePath, filePath );
    if ( PathFileExistsW( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    swprintf_s( dstPath, L"%s\\..\\%s", exePath, filePath );
    if ( PathFileExistsW( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    swprintf_s( dstPath, L"%s\\..\\..\\%s", exePath, filePath );
    if ( PathFileExistsW( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    swprintf_s( dstPath, L"%s\\res\\%s", exePath, filePath );
    if ( PathFileExistsW( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
//      ファイルパスを検索します.
//-----------------------------------------------------------------------------
bool SearchFilePathA( const char* filePath, std::string& result )
{
    if ( filePath == nullptr )
    { return false; }

    if ( strcmp( filePath, " " ) == 0 || strcmp( filePath, "" ) == 0 )
    { return false; }

    char exePath[ 520 ] = { 0 };
    GetModuleFileNameA( nullptr, exePath, 520  );
    exePath[ 519 ] = 0; // null終端化.
    PathRemoveFileSpecA( exePath );

    char dstPath[ 520 ] = { 0 };

    strcpy_s( dstPath, filePath );
    if ( PathFileExistsA( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    sprintf_s( dstPath, "..\\%s", filePath );
    if ( PathFileExistsA( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    sprintf_s( dstPath, "..\\..\\%s", filePath );
    if ( PathFileExistsA( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    sprintf_s( dstPath, "\\res\\%s", filePath );
    if ( PathFileExistsA( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    sprintf_s( dstPath, "%s\\%s", exePath, filePath );
    if ( PathFileExistsA( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    sprintf_s( dstPath, "%s\\..\\%s", exePath, filePath );
    if ( PathFileExistsA( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    sprintf_s( dstPath, "%s\\..\\..\\%s", exePath, filePath );
    if ( PathFileExistsA( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    sprintf_s( dstPath, "%s\\res\\%s", exePath, filePath );
    if ( PathFileExistsA( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
//      ファイルパスからディレクトリ名を取得します.
//-----------------------------------------------------------------------------
std::wstring GetDirectoryPathW( const wchar_t* filePath )
{
    std::wstring path = filePath;
    auto idx = path.find_last_of( L"/" );
    if ( idx != std::wstring::npos )
    {
        auto result = path.substr( 0, idx + 1 );
        return result;
    }

    idx = path.find_last_of( L"\\" );
    if ( idx != std::wstring::npos )
    {
        auto result = path.substr( 0, idx + 1 );
        return result;
    }

    return std::wstring();
}

//-----------------------------------------------------------------------------
//      ファイルパスからディレクトリ名を取得します.
//-----------------------------------------------------------------------------
std::string GetDirectoryPathA( const char* filePath )
{
    std::string path = filePath;
    auto idx = path.find_last_of( "/" );
    if ( idx != std::string::npos )
    {
        auto result = path.substr( 0, idx + 1 );
        return result;
    }

    idx = path.find_last_of( "\\" );
    if ( idx != std::string::npos )
    {
        auto result = path.substr( 0, idx + 1 );
        return result;
    }

    return std::string();
}

//-----------------------------------------------------------------------------
//      ファイルパスからディレクトリ名を削除します.
//-----------------------------------------------------------------------------
std::wstring RemoveDirectoryPathW( const wchar_t* filePath )
{
    std::wstring path = filePath;
    auto idx = path.find_last_of( L"/" );
    if ( idx != std::wstring::npos )
    {
        auto result = path.substr( idx + 1 );
        return result;
    }

    idx = path.find_last_of( L"\\" );
    if ( idx != std::wstring::npos )
    {
        auto result = path.substr( idx + 1 );
        return result;
    }

    return path;
}

//-----------------------------------------------------------------------------
//      ファイルパスからディレクトリ名を削除します.
//-----------------------------------------------------------------------------
std::string RemoveDirectoryPathA( const char* filePath )
{
    std::string path = filePath;
    auto idx = path.find_last_of( "/" );
    if ( idx != std::string::npos )
    {
        auto result = path.substr( idx + 1 );
        return result;
    }

    idx = path.find_last_of( "\\" );
    if ( idx != std::string::npos )
    {
        auto result = path.substr( idx + 1 );
        return result;
    }

    return path;
}

//-----------------------------------------------------------------------------
//      実行ファイルのファイルパスを取得します.
//-----------------------------------------------------------------------------
std::wstring GetExePathW()
{
    wchar_t exePath[ 520 ] = { 0 };
    GetModuleFileNameW( nullptr, exePath, 520  );
    exePath[ 519 ] = 0; // null終端化.

    return asdx::GetDirectoryPathW( exePath );
}

//-----------------------------------------------------------------------------
//      実行ファイルのファイルパスを取得します.
//-----------------------------------------------------------------------------
std::string GetExePathA()
{
    char exePath[ 520 ] = { 0 };
    GetModuleFileNameA( nullptr, exePath, 520  );
    exePath[ 519 ] = 0; // null終端化.

    return asdx::GetDirectoryPathA( exePath );
}

//-----------------------------------------------------------------------------
//      ファイルパスから拡張子を取得します.
//-----------------------------------------------------------------------------
std::wstring GetExtW( const wchar_t* filePath )
{
    std::wstring path = filePath;
    auto idx = path.find_last_of( L"." );
    if ( idx != std::wstring::npos )
    {
        std::wstring result = path.substr( idx + 1 );

        // 小文字化.
        std::transform( result.begin(), result.end(), result.begin(), tolower );

        return result;
    }

    return std::wstring();
}

//-----------------------------------------------------------------------------
//      ファイルパスから拡張子を取得します.
//-----------------------------------------------------------------------------
std::string GetExtA( const char* filePath )
{
    std::string path = filePath;
    auto idx = path.find_last_of( "." );
    if ( idx != std::string::npos )
    {
        std::string result = path.substr( idx + 1 );

        // 小文字化.
        std::transform( result.begin(), result.end(), result.begin(), tolower );

        return result;
    }

    return std::string();
}

//-----------------------------------------------------------------------------
//      拡張子を取り除いたファイルパスを取得します.
//-----------------------------------------------------------------------------
std::wstring GetPathWithoutExtW( const wchar_t* filePath )
{
    std::wstring path = filePath;
    auto idx = path.find_last_of( L"." );
    if ( idx != std::wstring::npos )
    {
        return path.substr( 0, idx );
    }

    return path;
}

//-----------------------------------------------------------------------------
//      拡張子を取り除いたファイルパスを取得します.
//-----------------------------------------------------------------------------
std::string GetPathWithoutExtA( const char* filePath )
{
    std::string path = filePath;
    auto idx = path.find_last_of( "." );
    if ( idx != std::string::npos )
    {
        return path.substr( 0, idx );
    }

    return path;
}


//-----------------------------------------------------------------------------
//      指定されたファイルパスが存在するかチェックします.
//-----------------------------------------------------------------------------
bool IsExistFilePathW( const wchar_t* filePath )
{
    if ( PathFileExistsW( filePath ) == TRUE )
    { return true; }

    return false;
}

//-----------------------------------------------------------------------------
//      指定されたファイルパスが存在するかチェックします.
//-----------------------------------------------------------------------------
bool IsExistFilePathA( const char* filePath )
{
    if ( PathFileExistsA( filePath ) == TRUE )
    { return true; }

    return false;
}

//-----------------------------------------------------------------------------
//      指定されたフォルダパスが存在するかチェックします.
//-----------------------------------------------------------------------------
bool IsExistFolderPathA( const char* folderPath )
{
    if ( PathFileExistsA ( folderPath ) == TRUE
      && PathIsDirectoryA( folderPath ) != FALSE ) // PathIsDirectoryA() は TRUE を返却しないので注意!!
    { return true; }

    return false;
}

//-----------------------------------------------------------------------------
//      指定されたフォルダパスが存在するかチェックします.
//-----------------------------------------------------------------------------
bool IsExistFolderPathW( const wchar_t* folderPath )
{
    if ( PathFileExistsW ( folderPath ) == TRUE
      && PathIsDirectoryW( folderPath ) != FALSE ) // PathIsDirectoryW() は TRUE を返却しないので注意!!
    { return true; }

    return false;
}

//-----------------------------------------------------------------------------
//      ワイド文字列に変換します.
//-----------------------------------------------------------------------------
std::wstring ToStringW( const std::string& value )
{
    auto length = MultiByteToWideChar(CP_ACP, 0, value.c_str(), int(value.size() + 1), nullptr, 0 );
    auto buffer = new wchar_t[length];

    MultiByteToWideChar(CP_ACP, 0, value.c_str(), int(value.size() + 1),  buffer, length );

    std::wstring result( buffer );
    delete[] buffer;

    return result;
}

//-----------------------------------------------------------------------------
//      マルチバイト文字列に変換します.
//-----------------------------------------------------------------------------
std::string ToStringA( const std::wstring& value )
{
    auto length = WideCharToMultiByte(CP_ACP, 0, value.c_str(), int(value.size() + 1), nullptr, 0, nullptr, nullptr); 
    auto buffer = new char[length];
 
    WideCharToMultiByte(CP_ACP, 0, value.c_str(), int(value.size() + 1), buffer, length, nullptr, nullptr);

    std::string result(buffer);
    delete[] buffer;

    return result;
}

//-----------------------------------------------------------------------------
//      UTF-8文字列に変換します.
//-----------------------------------------------------------------------------
std::string ToStringUTF8(const std::wstring& src)
{
    auto const dest_size = ::WideCharToMultiByte(CP_UTF8, 0U, src.data(), -1, nullptr, 0, nullptr, nullptr);
    std::vector<char> dest(dest_size, '\0');
    if (::WideCharToMultiByte(CP_UTF8, 0U, src.data(), -1, dest.data(), int(dest.size()), nullptr, nullptr) == 0) {
        throw std::system_error{static_cast<int>(::GetLastError()), std::system_category()};
    }
    return std::string(dest.begin(), dest.end());
}

//-----------------------------------------------------------------------------
//      UTF-8文字列に変換します.
//-----------------------------------------------------------------------------
std::string ToStringUTF8(const std::string& value)
{
    auto wide = ToStringW(value);
    return ToStringUTF8(wide);
}

//-----------------------------------------------------------------------------
//      指定文字で文字列を分割します.
//-----------------------------------------------------------------------------
std::vector<std::string> Split(const std::string& input, char delimiter)
{
    std::istringstream stream(input);

    std::string field;
    std::vector<std::string> result;
    while (std::getline(stream, field, delimiter))
    { result.push_back(field); }
    return result;
}

//-----------------------------------------------------------------------------
//      指定文字で文字列を分割します.
//-----------------------------------------------------------------------------
std::vector<std::wstring> Split(const std::wstring& input, wchar_t delimiter)
{
    std::wistringstream stream(input);

    std::wstring field;
    std::vector<std::wstring> result;
    while (std::getline(stream, field, delimiter))
    { result.push_back(field); }
    return result;
}

//-----------------------------------------------------------------------------
//      外部プロセスを実行します.
//-----------------------------------------------------------------------------
bool RunProcess(const char* cmd, bool wait, int* retcode)
{
    STARTUPINFOA        startup_info = {};
    PROCESS_INFORMATION process_info = {};

    DWORD flag = NORMAL_PRIORITY_CLASS;
    startup_info.cb = sizeof(STARTUPINFOA);

    // 成功すると0以外, 失敗すると0が返る.
    auto ret = CreateProcessA(
        nullptr,
        const_cast<char*>(cmd), // 実害はないはず...
        nullptr,
        nullptr,
        FALSE,
        flag,
        nullptr,
        nullptr,
        &startup_info,
        &process_info);

    if (ret == 0)
    {
        //ELOGA("Error : プロセス起動に失敗. コマンド = %s", cmd);
        CloseHandle(process_info.hProcess);
        CloseHandle(process_info.hThread);
        return false;
    }

    if (wait)
    { WaitForSingleObject(process_info.hProcess, INFINITE); }

    DWORD exitCode;
    ret = GetExitCodeProcess(process_info.hProcess, &exitCode);

    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);

    if (ret == 0)
    { return false; }

    if (retcode != nullptr)
    { *retcode = int(exitCode); }

    return true;
}

//-----------------------------------------------------------------------------
//      オープンファイルダイアログです.
//-----------------------------------------------------------------------------
bool OpenFileDlg(const char* fileFilter, std::string& result, const std::string& defaultPath)
{
    OPENFILENAMEA ofn;
    ZeroMemory( &ofn, sizeof(ofn) );

    CHAR inputFile     [ MAX_PATH ] = { 0 };
    CHAR inputFileTitle[ MAX_PATH ] = { 0 };
    CHAR initDir       [ MAX_PATH ] = { 0 };

    // パスが設定されていれば初期ディレク処理を設定.
    if (!defaultPath.empty() && defaultPath != "")
    {
        auto path = defaultPath;
        auto idx = path.find_last_of("\\");
        if (idx != std::string::npos && idx == path.length() - 1)
        { path = path.substr(0, idx); }
        strcpy_s(initDir, path.c_str());
    }

    ofn.lStructSize     = sizeof(OPENFILENAMEA);
    ofn.hwndOwner       = nullptr;
    ofn.lpstrFilter     = fileFilter;
    ofn.nMaxFile        = MAX_PATH;
    ofn.nMaxFileTitle   = MAX_PATH;
    ofn.Flags           = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrTitle      = "Open";
    ofn.lpstrFile       = inputFile;
    ofn.lpstrFileTitle  = inputFileTitle;
    ofn.lpstrInitialDir = initDir;

    if ( GetOpenFileNameA( &ofn ) == TRUE )
    {
        result = inputFile;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
//      セーブファイルダイアログです.
//-----------------------------------------------------------------------------
bool SaveFileDlg(const char* fileFilter, std::string& base, std::string& ext, const std::string& defaultPath)
{
    OPENFILENAMEA ofn;
    ZeroMemory( &ofn, sizeof(ofn) );

    CHAR inputFile     [ MAX_PATH ] = { 0 };
    CHAR inputFileTitle[ MAX_PATH ] = { 0 };
    CHAR templateName  [ MAX_PATH ] = { 0 };
    CHAR initDir       [ MAX_PATH ] = { 0 };

    if (!defaultPath.empty() && defaultPath != "")
    {
        auto path = defaultPath;
        auto idx = path.find_last_of("\\");
        if (idx != std::string::npos && idx == path.length() - 1)
        { path = path.substr(0, idx); }
        strcpy_s(initDir, path.c_str());
    }

    ofn.lStructSize     = sizeof(OPENFILENAMEA);
    ofn.hwndOwner       = nullptr;
    ofn.lpstrFilter     = fileFilter;
    ofn.nMaxFile        = MAX_PATH;
    ofn.nMaxFileTitle   = MAX_PATH;
    ofn.nFilterIndex    = 1;
    ofn.Flags           = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    ofn.lpstrTitle      = "Save As";
    ofn.lpstrFile       = inputFile;
    ofn.lpstrFileTitle  = inputFileTitle;
    ofn.lpTemplateName  = templateName;
    ofn.lpstrInitialDir = initDir;

    if ( GetSaveFileNameA( &ofn ) == TRUE )
    {
        base = std::string( inputFile ).substr( 0, ofn.nFileExtension - 1 );
        if ( ofn.nFileExtension != 0 )
        {
            ext = std::string( inputFile ).substr( ofn.nFileExtension );
        }
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
//      フォルダ選択ダイアログを開きます.
//-----------------------------------------------------------------------------
bool OpenFolderDlg(std::string& result, const std::string& defaultPath)
{
    IFileDialog* pDlg       = nullptr;
    IShellItem*  pShellItem = nullptr;
    IShellItem*  pDefaultItem = nullptr;
    DWORD options = 0;

    auto ret = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pDlg));
    if (FAILED(ret))
    { return false; }

    if (!defaultPath.empty() && defaultPath != "")
    {
        auto folder = asdx::ToStringW(defaultPath);
        auto pos = folder.find_last_of(L"\\");
        if (pos != std::wstring::npos && pos == folder.length() - 1)
        { folder = folder.substr(0, pos); }

        ret = SHCreateItemFromParsingName(folder.c_str(), nullptr, IID_PPV_ARGS(&pDefaultItem));
        if (FAILED(ret))
        {
            WLOG("LOG_LEVEL_WARNING : SHCreateItemFromParsingName() Failed");
            if (pDefaultItem != nullptr)
            {
                pDefaultItem->Release();
                pDefaultItem = nullptr;
            }
        }
    }

    if (pDefaultItem != nullptr)
    {
        pDlg->SetFolder(pDefaultItem);
    }

    pDlg->GetOptions(&options);
    pDlg->SetOptions(options | FOS_PICKFOLDERS);
    ret = pDlg->Show(nullptr);
    if (FAILED(ret))
    {
        if (pShellItem != nullptr)
        {
            pShellItem->Release();
            pShellItem = nullptr;
        }

        if (pDlg != nullptr)
        {
            pDlg->Release();
            pDlg = nullptr;
        }

        if (pDefaultItem != nullptr)
        {
            pDefaultItem->Release();
            pDefaultItem = nullptr;
        }
        return false;
    }

    ret = pDlg->GetResult(&pShellItem);
    if (FAILED(ret))
    {
        if (pShellItem != nullptr)
        {
            pShellItem->Release();
            pShellItem = nullptr;
        }

        if (pDlg != nullptr)
        {
            pDlg->Release();
            pDlg = nullptr;
        }

        if (pDefaultItem != nullptr)
        {
            pDefaultItem->Release();
            pDefaultItem = nullptr;
        }
        return false;
    }
    
    PWSTR path;
    ret = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &path);

    if (FAILED(ret))
    {
        if (pShellItem != nullptr)
        {
            pShellItem->Release();
            pShellItem = nullptr;
        }

        if (pDlg != nullptr)
        {
            pDlg->Release();
            pDlg = nullptr;
        }

        if (pDefaultItem != nullptr)
        {
            pDefaultItem->Release();
            pDefaultItem = nullptr;
        }
        return false;
    }

    result = asdx::ToStringA(path);
    CoTaskMemFree(path);

    if (pShellItem != nullptr)
    {
        pShellItem->Release();
        pShellItem = nullptr;
    }

    if (pDlg != nullptr)
    {
        pDlg->Release();
        pDlg = nullptr;
    }

    if (pDefaultItem != nullptr)
    {
        pDefaultItem->Release();
        pDefaultItem = nullptr;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      情報ダイアログです.
//-----------------------------------------------------------------------------
void InfoDlg(const char* title, const char* msg)
{
    MessageBoxA( nullptr, msg, title, MB_ICONINFORMATION | MB_OK );
}

//-----------------------------------------------------------------------------
//      エラーダイアログです.
//-----------------------------------------------------------------------------
void ErrorDlg(const char* title, const char* msg )
{
    MessageBoxA( nullptr, msg, title, MB_ICONERROR | MB_OK );
}

//-----------------------------------------------------------------------------
//      DXGIフォーマットから1ピクセルあたりのビット数を取得します.
//-----------------------------------------------------------------------------
int GetBitsPerPixel(int dxgiFormat)
{
    switch(dxgiFormat)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 128;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 96;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_Y416:
    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
        return 64;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_AYUV:
    case DXGI_FORMAT_Y410:
    case DXGI_FORMAT_YUY2:
    //case XBOX_DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT:
    //case XBOX_DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT:
    //case XBOX_DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM:
        return 32;

    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
    //case XBOX_DXGI_FORMAT_D16_UNORM_S8_UINT:
    //case XBOX_DXGI_FORMAT_R16_UNORM_X8_TYPELESS:
    //case XBOX_DXGI_FORMAT_X16_TYPELESS_G8_UINT:
    //case WIN10_DXGI_FORMAT_V408:
        return 24;

    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_A8P8:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
    //case WIN10_DXGI_FORMAT_P208:
    //case WIN10_DXGI_FORMAT_V208:
        return 16;

    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_420_OPAQUE:
    case DXGI_FORMAT_NV11:
        return 12;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
    //case XBOX_DXGI_FORMAT_R4G4_UNORM:
        return 8;

    case DXGI_FORMAT_R1_UNORM:
        return 1;

    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 4;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 8;

    default:
        return 0;
    }
}

//-----------------------------------------------------------------------------
//      DXGIフォーマットから1ピクセルあたりのバイト数を取得します.
//-----------------------------------------------------------------------------
int GetBytePerPixel(int dxgiFormat)
{
    return GetBitsPerPixel(dxgiFormat) / 8;
}

//-----------------------------------------------------------------------------
//      SRGBフォーマットに変換します.
//-----------------------------------------------------------------------------
int MakeSRGB(int dxgiFormat)
{
    switch( dxgiFormat )
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    case DXGI_FORMAT_BC1_UNORM:
        return DXGI_FORMAT_BC1_UNORM_SRGB;

    case DXGI_FORMAT_BC2_UNORM:
        return DXGI_FORMAT_BC2_UNORM_SRGB;

    case DXGI_FORMAT_BC3_UNORM:
        return DXGI_FORMAT_BC3_UNORM_SRGB;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

    case DXGI_FORMAT_B8G8R8X8_UNORM:
        return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

    case DXGI_FORMAT_BC7_UNORM:
        return DXGI_FORMAT_BC7_UNORM_SRGB;

    default:
        return dxgiFormat;
    }
}

//-----------------------------------------------------------------------------
//      ディレクトリを一括削除します.
//-----------------------------------------------------------------------------
bool DeleteDirA(const char* path)
{
    WIN32_FIND_DATAA find;

    std::string targetDir = path;
    targetDir += "*";

    auto handle = FindFirstFileA(targetDir.c_str(), &find);
    if (handle == INVALID_HANDLE_VALUE)
    { return false; }

    std::string dir  = path;
    std::string file;

    if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        if (strcmp(find.cFileName, ".") != 0)
        {
            dir += find.cFileName;
            if (!DeleteDirA(dir.c_str()))
            {
                FindClose(handle);
                return false;
            }
        }
    }
    else
    {
        file = dir;
        file += find.cFileName;
        if (DeleteFileA(file.c_str()) == FALSE)
        {
            FindClose(handle);
            return false;
        }
    }

    while (FindNextFileA(handle, &find))
    {
        if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (strcmp(find.cFileName, "..") != 0)
            {
                dir = path;
                dir += find.cFileName;
                if (!DeleteDirA(dir.c_str()))
                {
                    FindClose(handle);
                    return false;
                }
            }
        }
        else
        {
            file = dir;
            file += find.cFileName;
            if (DeleteFileA(file.c_str()) == FALSE)
            {
                FindClose(handle);
                return false;
            }
        }
    }

    FindClose(handle);
    return (RemoveDirectoryA(path) != FALSE);
}

//-----------------------------------------------------------------------------
//      ディレクトリを一括削除します.
//-----------------------------------------------------------------------------
bool DeleteDirW(const wchar_t* path)
{
    WIN32_FIND_DATAW find;

    std::wstring targetDir = path;
    targetDir += L"*";

    auto handle = FindFirstFileW(targetDir.c_str(), &find);
    if (handle == INVALID_HANDLE_VALUE)
    { return false; }

    std::wstring dir  = path;
    std::wstring file;

    if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        if (wcscmp(find.cFileName, L".") != 0)
        {
            dir += find.cFileName;
            if (!DeleteDirW(dir.c_str()))
            {
                FindClose(handle);
                return false;
            }
        }
    }
    else
    {
        file = dir;
        file += find.cFileName;
        if (DeleteFileW(file.c_str()) == FALSE)
        {
            FindClose(handle);
            return false;
        }
    }

    while (FindNextFileW(handle, &find))
    {
        if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (wcscmp(find.cFileName, L"..") != 0)
            {
                dir = path;
                dir += find.cFileName;
                if (!DeleteDirW(dir.c_str()))
                {
                    FindClose(handle);
                    return false;
                }
            }
        }
        else
        {
            file = dir;
            file += find.cFileName;
            if (DeleteFileW(file.c_str()) == FALSE)
            {
                FindClose(handle);
                return false;
            }
        }
    }

    FindClose(handle);
    return (RemoveDirectoryW(path) != FALSE);
}

//-----------------------------------------------------------------------------
//      特定ディレクトリ下の指定拡張子を持つファイルリストを取得します.
//-----------------------------------------------------------------------------
bool SearchFilesA(const char* directory, const char* ext, std::list<std::string>& result)
{
    WIN32_FIND_DATAA find;

    std::string targetDir = directory;
    std::string dir = directory;

    auto pos = targetDir.find_last_of("\\");
    if (pos != targetDir.size() - 1)
    {
        targetDir += "\\";
        dir += "\\";
    }

    targetDir += "*";

    if (ext != nullptr && strcmp(ext, "") != 0)
    { targetDir += ext; }

    auto handle = FindFirstFileA(targetDir.c_str(), &find);
    if (handle == INVALID_HANDLE_VALUE)
    { return false; }

    std::string file;

    // ディレクトリじゃない場合.
    if ((find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        file = dir;
        file += find.cFileName;
        result.push_back(file);
    }

    while (FindNextFileA(handle, &find))
    {
        // ディレクトリじゃない場合
        if ((find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            file = dir;
            file += find.cFileName;
            result.push_back(file);
        }
    }

    FindClose(handle);
    return result.empty() == false;
}


//-----------------------------------------------------------------------------
//      フルパスに変換します.
//-----------------------------------------------------------------------------
std::string ToFullPathA(const char* path)
{
    char fullPath[512] = {};

    GetFullPathNameA(path, 512, fullPath, nullptr);
    return std::string(fullPath);
}

//-----------------------------------------------------------------------------
//      フルパスに変換します.
//-----------------------------------------------------------------------------
std::wstring ToFullPathW(const wchar_t* path)
{
    wchar_t fullPath[512] = {};
    GetFullPathNameW(path, 512, fullPath, nullptr);
    return std::wstring(fullPath);
}

//-----------------------------------------------------------------------------
//      相対パスに変換します.
//-----------------------------------------------------------------------------
std::string ToRelativePath(const char* base, const char* path, bool directory)
{
    char relativePath[MAX_PATH];
    DWORD attribute = directory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;

    char srcPath[512];
    GetFullPathNameA(path, 512, srcPath, nullptr);

    char basePath[512];
    GetFullPathNameA(base, 512, basePath, nullptr);

    if (PathRelativePathToA(relativePath, basePath, attribute, srcPath, attribute) == TRUE)
    { return std::string(relativePath); }

    return std::string();
}

//-----------------------------------------------------------------------------
//      バックスラッシュをスラッシュに変更します.
//-----------------------------------------------------------------------------
std::string ToSlash(const std::string& path)
{
    std::string ret = path;
    auto pos = ret.find('\\');
    while (pos != std::string::npos)
    {
        ret[pos] = '/';
        pos = ret.find('\\');
    }
    return ret;
}

//-----------------------------------------------------------------------------
//      文字列を置換します.
//-----------------------------------------------------------------------------
std::string Replace
(
    const std::string&  input,
    std::string         pattern,
    std::string         replace
)
{
    std::string result = input;
    auto pos = result.find( pattern );

    while( pos != std::string::npos )
    {
        result.replace( pos, pattern.length(), replace );
        pos = result.find( pattern, pos + replace.length() );
    }

    return result;
}

//-----------------------------------------------------------------------------
//      相対パスに変換し，スラッシュに変更します.
//-----------------------------------------------------------------------------
std::string ToRelativePathWithSlash(const std::string& base, const std::string& value)
{
    auto ext = asdx::GetExtA(base.c_str());
    auto is_dir = (ext.empty() || ext == "") ? true : false;

    auto path = ToRelativePath(base.c_str(), value.c_str(), is_dir);
    auto pos = path.find(".\\");
    if (pos != std::string::npos && pos == 0)
    { path = path.substr(2); }

    return ToSlash(path);
}

//-----------------------------------------------------------------------------
//      小文字に変換します.
//-----------------------------------------------------------------------------
std::string ToLower(const std::string& value)
{
    std::string result = value;
    std::transform( result.begin(), result.end(), result.begin(), tolower );
    return result;
}

//-----------------------------------------------------------------------------
//      環境変数を取得します.
//-----------------------------------------------------------------------------
std::string GetEnv(const char* name)
{
    size_t size;
    if (getenv_s(&size, nullptr, 0, name))
    { return ""; }

    if (size == 0)
    { return ""; }

    std::string result;
    result.resize(size + 1);
    getenv_s(&size, &result[0], size, name);
    result.resize(std::strlen(result.c_str()));
    result.shrink_to_fit();

    return result;
}

} // namespace asdx
