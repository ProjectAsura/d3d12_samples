//-----------------------------------------------------------------------------
// File : asdxP4VHelper.cpp
// Desc : Perforce Helix P4V Helper.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <edit/asdxP4VHelper.h>
#include <fnd/asdxMisc.h>
#include <Windows.h>


namespace asdx {

//-----------------------------------------------------------------------------
//      デフォルトのチェンジリストに対象ファイルを追加します.
//-----------------------------------------------------------------------------
bool P4Add(const char* path, const char* changeList)
{
    char fullPath[MAX_PATH];
    char cmd[2048];

    GetFullPathNameA(path, MAX_PATH, fullPath, nullptr);
    sprintf_s(cmd, "p4 add -d -c %s %s", changeList, fullPath);

    return RunProcess(cmd);
}

//-----------------------------------------------------------------------------
//      デフォルトのチェンジリストに対象ファイルをチェックアウトします.
//-----------------------------------------------------------------------------
bool P4Checkout(const char* path, const char* changeList)
{
    char fullPath[MAX_PATH];
    char cmd[2048];

    GetFullPathNameA(path, MAX_PATH, fullPath, nullptr);
    sprintf_s(cmd, "p4 edit -c %s %s", changeList, fullPath);

    return RunProcess(cmd);
}

//-----------------------------------------------------------------------------
//      対象ファイルの変更を元に戻します.
//-----------------------------------------------------------------------------
bool P4Revert(const char* path, const char* changeList)
{
    char fullPath[MAX_PATH];
    char cmd[2048];

    GetFullPathNameA(path, MAX_PATH, fullPath, nullptr);
    sprintf_s(cmd, "p4 revert -a -c %s %s", changeList, fullPath);

    return RunProcess(cmd);
}

//-----------------------------------------------------------------------------
//      対象ファイルを削除目的でマークします.
//-----------------------------------------------------------------------------
bool P4Delete(const char* path, const char* changeList)
{
    char fullPath[MAX_PATH];
    char cmd[2048];

    GetFullPathNameA(path, MAX_PATH, fullPath, nullptr);
    sprintf_s(cmd, "p4 delete -c %s %s", changeList, fullPath);

    return RunProcess(cmd);
}

//-----------------------------------------------------------------------------
//      ファイルを追加、削除、および/または編集目的で作業状態にし、Perforce外部での変更内容とワークスペースとを一致させます
//-----------------------------------------------------------------------------
bool P4Reconcile(const char* path, const char* changeList)
{
    char fullPath[MAX_PATH];
    char cmd[2048];

    GetFullPathNameA(path, MAX_PATH, fullPath, nullptr);
    sprintf_s(cmd, "p4 reconcile -c %s -a -d -e %s", changeList, fullPath);

    return RunProcess(cmd);
}

} // namespace asdx
