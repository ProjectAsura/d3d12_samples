//-----------------------------------------------------------------------------
// File : asdxFileWatcher.cpp
// Desc : File Wacher.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <vector>
#include <string>
#include <Windows.h>
#include <edit/asdxFileWatcher.h>
#include <fnd/asdxLogger.h>


namespace {

//-----------------------------------------------------------------------------
//      マルチバイト文字列に変換します.
//-----------------------------------------------------------------------------
std::string ToStringA(const std::wstring& value)
{
    auto length = WideCharToMultiByte(CP_ACP, 0, value.c_str(), int(value.size() + 1), nullptr, 0, nullptr, nullptr);
    auto buffer = new char[length];

    WideCharToMultiByte(CP_ACP, 0, value.c_str(), int(value.size() + 1), buffer, length, nullptr, nullptr);

    std::string result(buffer);
    delete[] buffer;

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// Worker structure
///////////////////////////////////////////////////////////////////////////////
struct Worker
{
    HANDLE                      hEvent          = nullptr;
    HANDLE                      hDir            = nullptr;
    uint32_t                    WaitTimeMsec    = 0;
    std::vector<uint8_t>        Buffer          = {};
    std::string                 DirectoryPath   = {};
    std::atomic<bool>*          pFinish         = nullptr;

    std::list<asdx::IFileUpdateListener*> pListeners = {};

    Worker()
    { /* DO_NOTHING */ }

    ~Worker()
    { /* DO_NOTHING */ }

    bool Prepare(asdx::FileWatcher::Desc& desc, std::atomic<bool>* pFlags)
    {
        hDir = CreateFileA(
            desc.DirectoryPath,
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            nullptr
        );
        if (hDir == INVALID_HANDLE_VALUE)
        {
            ELOGA("Error : CreateFileA() Failed. path = %s, errcode = 0x%x", desc.DirectoryPath, GetLastError());
            hDir = nullptr;
            return false;
        }

        hEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
        if (hEvent == nullptr)
        {
            ELOGA("Error : CreateEventA() Failed.");
            CloseHandle(hDir);
            hDir = nullptr;
            return false;
        }

        pFinish         = pFlags;
        DirectoryPath   = desc.DirectoryPath;
        pListeners      = std::move(desc.pListeners);
        WaitTimeMsec    = desc.WaitTimeMsec;
        Buffer.resize(desc.BufferSize);

        return true;
    }

    void operator()()
    {
        void* pBuf = Buffer.data();
        auto bufSize = uint32_t(Buffer.size());

        // フィルタ.
        uint32_t filter =
            FILE_NOTIFY_CHANGE_FILE_NAME  |   // ファイル名の変更.
            FILE_NOTIFY_CHANGE_DIR_NAME   |   // ディレクトリ名の変更.
            FILE_NOTIFY_CHANGE_ATTRIBUTES |   // 属性の変更.
            FILE_NOTIFY_CHANGE_SIZE       |   // サイズの変更.
            FILE_NOTIFY_CHANGE_LAST_WRITE;    // 最終書き込み日時の変更.

        // 終了フラグが立つまでループ.
        for (;;)
        {
            ResetEvent(hEvent);

            OVERLAPPED olp = {};
            olp.hEvent = hEvent;

            // 変更を監視.
            if (!ReadDirectoryChangesW(
                hDir,
                pBuf,
                bufSize,
                TRUE,
                filter,
                nullptr,
                &olp,
                nullptr))
            {
                ELOG("LOG_LEVEL_ERROR : ReadDirectoryChangesW() Failed.");
                break;
            }

            while (!pFinish->load())
            {
                auto ret = WaitForSingleObject(hEvent, WaitTimeMsec);
                if (ret != WAIT_TIMEOUT)
                {
                    break;
                }
            }

            // 終了フラグが立っていたら終了.
            if (pFinish->load())
            {
                // 非同期I/Oをキャンセル.
                CancelIo(hDir);

                // Overlapped構造体をシステムが使わなくなるまで待機する.
                WaitForSingleObject(hEvent, INFINITE);

                // おしまい.
                break;
            }

            // 非同期I/Oの結果を取得.
            DWORD retSize = 0;
            if (!GetOverlappedResult(hDir, &olp, &retSize, FALSE))
            {
                ELOG("LOG_LEVEL_ERROR : GetOverlappedResult() Failed.");
                break;
            }

            // バッファオーバーフローでない場合.
            if (retSize != 0)
            {
                auto pInfos = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(pBuf);
                size_t offset = 0;

                for (;;)
                {
                    // ファイル名取得.
                    auto path = ToStringA(pInfos->FileName);

                    // 末尾にカンマが来ることがあるので，それを取り除く.
                    auto pos = path.find_last_of(',');
                    if (pos != std::string::npos && pos == path.size() - 1)
                    { path = path.substr(0, pos); }

                    // 強制的に開いて閉じる.
                    // これでたま～にファイルがオープンできない問題を解決できる.
                    {
                        auto handle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
                        if (handle != INVALID_HANDLE_VALUE)
                        { CloseHandle(handle); }
                    }

                    asdx::FileUpdateEventArgs args;
                    args.Type           = asdx::ACTION_TYPE(pInfos->Action);
                    args.DirectoryPath  = DirectoryPath;
                    args.RelativePath   = path;

                    for(auto& listener : pListeners)
                    { listener->OnUpdate(args); }

                    // 次のエントリがなければ終了.
                    if (pInfos->NextEntryOffset == 0)
                    { break; }

                    // 次のエントリまで移動.
                    pInfos = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<uint8_t*>(pInfos) + pInfos->NextEntryOffset);
                }
            }
        }

        CloseHandle(hEvent);
        CloseHandle(hDir);

        hEvent      = nullptr;
        hDir        = nullptr;
        pFinish     = nullptr;
        pListeners.clear();
        Buffer.clear();
        Buffer.shrink_to_fit();
    }
};

} // namespace

namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// FileWatcher class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
FileWatcher::FileWatcher()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
FileWatcher::~FileWatcher()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool FileWatcher::Init(Desc& desc)
{
    // 念のために終了させる.
    Term();

    // 終了フラグを下す.
    m_Finish = false;

    // ワーカーを初期化.
    Worker worker;
    if (!worker.Prepare(desc, &m_Finish))
    { return false; }

    // 監視スレッド起動.
    m_pThread = new std::thread(worker);
    if (m_pThread == nullptr)
    { return false; }

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void FileWatcher::Term()
{
    if (m_pThread == nullptr)
    { return; }

    // 終了フラグを立てる.
    m_Finish = true;

    // join可能になるまで待つ.
    while (!m_pThread->joinable())
    { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }

    // joinする
    if (m_pThread->joinable())
    { m_pThread->join(); }

    // スレッド破棄.
    delete m_pThread;
    m_pThread = nullptr;
}

} // namespace asdx
