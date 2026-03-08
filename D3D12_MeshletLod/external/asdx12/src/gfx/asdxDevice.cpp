//-----------------------------------------------------------------------------
// File : asdxDevice.cpp
// Desc : Graphics Device.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <gfx/asdxDevice.h>
#include <gfx/asdxBuffer.h>
#include <gfx/asdxCommandQueue.h>
#include <gfx/asdxDisposer.h>
#include <gfx/asdxCommandList.h>
#include <fnd/asdxList.h>
#include <fnd/asdxSpinLock.h>
#include <fnd/asdxRef.h>
#include <fnd/asdxLogger.h>
#include <cassert>
#include <strsafe.h>
#include <vector>
#include <algorithm>
#include <tuple>
#include <ShlObj.h>


namespace {

#include "../res/shaders/Compiled/FullScreenVS.inc"

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
static const D3D12_INPUT_ELEMENT_DESC kQuadElements[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
};
static const D3D12_INPUT_LAYOUT_DESC kQuadLayout = { kQuadElements, 2 };

#define SS(filter, addressMode, maxAnisotropy, comparison, borderColor, shaderRegister) \
    { filter, addressMode, addressMode, addressMode, 0.0f, maxAnisotropy, comparison, borderColor, 0.0f, D3D12_FLOAT32_MAX, shaderRegister, 0, D3D12_SHADER_VISIBILITY_ALL }
static const D3D12_STATIC_SAMPLER_DESC kStaticSamplers[] = {
    SS(D3D12_FILTER_MIN_MAG_MIP_POINT,  D3D12_TEXTURE_ADDRESS_MODE_WRAP,   0,  D3D12_COMPARISON_FUNC_NEVER,      D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK, 0),
    SS(D3D12_FILTER_MIN_MAG_MIP_POINT,  D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  0,  D3D12_COMPARISON_FUNC_NEVER,      D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK, 1),
    SS(D3D12_FILTER_MIN_MAG_MIP_POINT,  D3D12_TEXTURE_ADDRESS_MODE_MIRROR, 0,  D3D12_COMPARISON_FUNC_NEVER,      D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK, 2),
    SS(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP,   0,  D3D12_COMPARISON_FUNC_NEVER,      D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK, 3),
    SS(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  0,  D3D12_COMPARISON_FUNC_NEVER,      D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK, 4),
    SS(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_MIRROR, 0,  D3D12_COMPARISON_FUNC_NEVER,      D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK, 5),
    SS(D3D12_FILTER_ANISOTROPIC,        D3D12_TEXTURE_ADDRESS_MODE_WRAP,   16, D3D12_COMPARISON_FUNC_NEVER,      D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK, 6),
    SS(D3D12_FILTER_ANISOTROPIC,        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  16, D3D12_COMPARISON_FUNC_NEVER,      D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK, 7),
    SS(D3D12_FILTER_ANISOTROPIC,        D3D12_TEXTURE_ADDRESS_MODE_MIRROR, 16, D3D12_COMPARISON_FUNC_NEVER,      D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK, 8),
    SS(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_BORDER, 0,  D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,      9),
    SS(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_BORDER, 0,  D3D12_COMPARISON_FUNC_GREATER,    D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,     10),
};
#undef SS

struct QuadVertex
{
    float   Position[2];
    float   TexCoord[2];

    QuadVertex(float x, float y, float u, float v)
    {
        Position[0] = x;
        Position[1] = y;
        TexCoord[0] = u;
        TexCoord[1] = v;
    }
};


//-------------------------------------------------------------------------------------------------
//      PIXキャプチャー用のDLLをロードします.
//-------------------------------------------------------------------------------------------------
void LoadPixGpuCpatureDll()
{
    LPWSTR programFilesPath = nullptr;
    SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

    wchar_t pixSearchPath[MAX_PATH] = {};
    StringCchCopy(pixSearchPath, MAX_PATH, programFilesPath);
    StringCchCat(pixSearchPath, MAX_PATH, L"\\Microsoft PIX\\*");

    WIN32_FIND_DATA findData;
    bool foundPixInstallation = false;
    wchar_t newestVersionFound[MAX_PATH] = {};

    HANDLE hFind = FindFirstFile(pixSearchPath, &findData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do 
        {
            if (((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) &&
                 (findData.cFileName[0] != '.'))
            {
                if (!foundPixInstallation || wcscmp(newestVersionFound, findData.cFileName) <= 0)
                {
                    foundPixInstallation = true;
                    StringCchCopy(newestVersionFound, _countof(newestVersionFound), findData.cFileName);
                }
            }
        } 
        while (FindNextFile(hFind, &findData) != 0);
    }

    FindClose(hFind);

    if (!foundPixInstallation)
    {
        return;
    }

    wchar_t dllPath[MAX_PATH] = {};
    StringCchCopy(dllPath, wcslen(pixSearchPath), pixSearchPath);
    StringCchCat(dllPath, MAX_PATH, &newestVersionFound[0]);
    StringCchCat(dllPath, MAX_PATH, L"\\WinPixGpuCapturer.dll");

    if (GetModuleHandleW(L"WinPixGpuCapturer.dll") == 0)
    {
        LoadLibraryW(dllPath);
    }
}

//-----------------------------------------------------------------------------
//      必要な中間データサイズを取得します.
//-----------------------------------------------------------------------------
inline UINT64 GetRequiredIntermediateSize
(
    ID3D12Device*           pDevice,
    D3D12_RESOURCE_DESC*    pDesc,
    UINT                    firstSubresource,
    UINT                    subresourceCount
) noexcept
{
    UINT64 requiredSize = 0;
    pDevice->GetCopyableFootprints(
        pDesc,
        firstSubresource,
        subresourceCount,
        0,
        nullptr,
        nullptr,
        nullptr,
        &requiredSize);
    return requiredSize;
}

//-----------------------------------------------------------------------------
//      サブリソースのコピーを行います.
//-----------------------------------------------------------------------------
inline void CopySubresource
(
    const D3D12_MEMCPY_DEST*        pDst,
    const D3D12_SUBRESOURCE_DATA*   pSrc,
    SIZE_T                          rowSizeInBytes,
    UINT                            rowCount,
    UINT                            sliceCount
) noexcept
{
    for (auto z=0u; z<sliceCount; ++z)
    {
        auto pDstSlice = static_cast<BYTE*>(pDst->pData)       + pDst->SlicePitch * z;
        auto pSrcSlice = static_cast<const BYTE*>(pSrc->pData) + pSrc->SlicePitch * LONG_PTR(z);
        for (auto y=0u; y<rowCount; ++y)
        {
            memcpy(pDstSlice + pDst->RowPitch * y,
                   pSrcSlice + pSrc->RowPitch * LONG_PTR(y),
                   rowSizeInBytes);
        }
    }
}

} // namespace


namespace asdx {

//-----------------------------------------------------------------------------
// Forward Declaration.
//-----------------------------------------------------------------------------
class DescriptorHeap;

///////////////////////////////////////////////////////////////////////////////
// Descriptor class
///////////////////////////////////////////////////////////////////////////////
class Descriptor : public IReference, public List<Descriptor>::Node
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    friend class DescriptorHeap;

public:
    //=========================================================================
    // public variables.
    //=========================================================================
    /* NOTHING */

    //=========================================================================
    // public methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      参照カウントを増やします.
    //-------------------------------------------------------------------------
    void AddRef() override;

    //-------------------------------------------------------------------------
    //! @brief      解放処理を行います.
    //-------------------------------------------------------------------------
    void Release() override;

    //-------------------------------------------------------------------------
    //! @brief      参照カウントを取得します.
    //-------------------------------------------------------------------------
    uint32_t GetCount() const override;

    //-------------------------------------------------------------------------
    //! @brief      CPUディスクリプタハンドルを取得します.
    //-------------------------------------------------------------------------
    D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const;

    //-------------------------------------------------------------------------
    //! @brief      GPUディスクリプタハンドルを取得します.
    //-------------------------------------------------------------------------
    D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const;

    //-------------------------------------------------------------------------
    //! @brief      インデックスを取得します.
    //-------------------------------------------------------------------------
    uint32_t GetIndex() const;

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    DescriptorHeap*             m_pHeap;        //!< ヒープへのポインタです.
    D3D12_CPU_DESCRIPTOR_HANDLE m_HandleCPU;    //!< CPUディスクリプタハンドルです.
    D3D12_GPU_DESCRIPTOR_HANDLE m_HandleGPU;    //!< GPUディスクリプタハンドルです.
    uint32_t                    m_Index;        //!< ディスクリプタインデックス.
    std::atomic<uint32_t>       m_RefCount;     //!< 参照カウンタです.

    //=========================================================================
    // private methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //-------------------------------------------------------------------------
    Descriptor();

    //-------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //-------------------------------------------------------------------------
    ~Descriptor();

    Descriptor      (const Descriptor&) = delete;
    void operator = (const Descriptor&) = delete;
};


///////////////////////////////////////////////////////////////////////////////
// DescriptorHeap class
///////////////////////////////////////////////////////////////////////////////
class DescriptorHeap
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    friend class Descriptor;

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
    //-------------------------------------------------------------------------
    DescriptorHeap();

    //-------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //-------------------------------------------------------------------------
    ~DescriptorHeap();

    //-------------------------------------------------------------------------
    //! @brief      初期化処理を行います.
    //-------------------------------------------------------------------------
    bool Init(ID3D12Device* pDevice, const D3D12_DESCRIPTOR_HEAP_DESC* pDesc);

    //-------------------------------------------------------------------------
    //! @brief      終了処理を行います.
    //-------------------------------------------------------------------------
    void Term();

    //-------------------------------------------------------------------------
    //! @brief      ディスクリプタを生成します.
    //!
    //! @return     生成したディスクリプタを返却します. 生成に失敗した場合は nullptr が返却されます.
    //-------------------------------------------------------------------------
    Descriptor* Alloc();

    //-------------------------------------------------------------------------
    //! @brief      利用可能なハンドル数を取得します.
    //!
    //! @return     利用可能なハンドル数を返却します.
    //-------------------------------------------------------------------------
    uint32_t GetAvailableCount() const;

    //-------------------------------------------------------------------------
    //! @brief      割り当て済みハンドル数を取得します.
    //!
    //! @return     割り当て済みハンドル数を返却します.
    //-------------------------------------------------------------------------
    uint32_t GetAllocatedCount() const;

    //-------------------------------------------------------------------------
    //! @brief      ハンドル総数を取得します.
    //!
    //! @return     ハンドル総数を返却します.
    //-------------------------------------------------------------------------
    uint32_t GetHandleCount() const;

    //-------------------------------------------------------------------------
    //! @brief      ディスクリプタヒープを取得します.
    //-------------------------------------------------------------------------
    ID3D12DescriptorHeap* GetD3D12DescriptorHeap() const;

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    ID3D12DescriptorHeap*   m_pHeap;            //!< ディスクリプタヒープです.
    List<Descriptor>        m_UsedList;         //!< 使用中リスト.
    List<Descriptor>        m_FreeList;         //!< 未使用リスト.
    Descriptor*             m_Descriptors;      //!< ディスクリプタ.
    uint32_t                m_IncrementSize;    //!< インクリメントサイズです.

    //=========================================================================
    // private methods.
    //=========================================================================
    void Free(Descriptor* pValue);

    DescriptorHeap  (const DescriptorHeap&) = delete;
    void operator = (const DescriptorHeap&) = delete;
};

///////////////////////////////////////////////////////////////////////////////
// GraphicsSystem class
///////////////////////////////////////////////////////////////////////////////
class GraphicsSystem
{
    //========================================================================
    // list of friend classes and methods.
    //========================================================================
    /* NOTHING */

public:

    //=========================================================================
    // public variables.
    //=========================================================================


    //=========================================================================
    // public methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      唯一のインスタンスを取得します.
    //!
    //! @return     唯一のインスタンスを返却します.
    //-------------------------------------------------------------------------
    static GraphicsSystem& Instance();

    //-------------------------------------------------------------------------
    //! @brief      初期化処理を行います.
    //!
    //! @param[in]      desc        構成設定です.
    //! @retval true    初期化に成功.
    //! @retval fasle   初期化に失敗.
    //-------------------------------------------------------------------------
    bool Init(const DeviceDesc& desc);

    //-------------------------------------------------------------------------
    //! @brief      終了処理を行います.
    //-------------------------------------------------------------------------
    void Term();

    //-------------------------------------------------------------------------
    //! @brief      ID3D12Device8を取得します.
    //!
    //! @return     ID3D12Device8を返却します.
    //-------------------------------------------------------------------------
    ID3D12Device8* GetDevice() const;

    //-------------------------------------------------------------------------
    //! @brief      IDXGIFactory7を取得します.
    //!
    //! @return     IDXGIFactory7を返却します.
    //-------------------------------------------------------------------------
    IDXGIFactory7* GetFactory() const;

    //-------------------------------------------------------------------------
    //! @brief      グラフィックスキューを取得します.
    //!
    //! @return     グラフィックスキューを返却します.
    //-------------------------------------------------------------------------
    CommandQueue* GetGraphicsQueue() const;

    //-------------------------------------------------------------------------
    //! @brief      コンピュートキューを取得します.
    //!
    //! @return     コンピュートキューを返却します.
    //-------------------------------------------------------------------------
    CommandQueue* GetComputeQueue() const;

    //-------------------------------------------------------------------------
    //! @brief      コピーキューを取得します.
    //!
    //! @return     コピーキューを返却します.
    //-------------------------------------------------------------------------
    CommandQueue* GetCopyQueue() const;

    //-------------------------------------------------------------------------
    //! @brief      ビデオデコードキューを取得します.
    //!
    //! @return     ビデオデコードキューを返却します.
    //-------------------------------------------------------------------------
    CommandQueue* GetVideoDecodeQueue() const;

    //-------------------------------------------------------------------------
    //! @brief      ビデオプロセスキューを取得します.
    //!
    //! @return     ビデオプロセスキューを返却します.
    //-------------------------------------------------------------------------
    CommandQueue* GetVideoProcessQueue() const;

    //-------------------------------------------------------------------------
    //! @brief      ビデオエンコードキューを取得します.
    //!
    //! @return     ビデオエンコードキューを返却します.
    //-------------------------------------------------------------------------
    CommandQueue* GetVideoEncodeQueue() const;

    //-------------------------------------------------------------------------
    //! @brief      ディスクリプターを確保します.
    //!
    //! @param[in]      index       ディスクリプタタイプです.
    //! @param[out]     ppResult    ディスクリプタの確保先です.
    //! @retval true    確保に成功.
    //! @retval false   確保に失敗.
    //-------------------------------------------------------------------------
    bool AllocHandle(uint8_t heapType, Descriptor** ppResult);

    //-------------------------------------------------------------------------
    //! @brief      アロー演算子です.
    //-------------------------------------------------------------------------
    ID3D12Device8* operator-> () const;

    //-------------------------------------------------------------------------
    //! @brief      ディスクリプタヒープを設定します.
    //-------------------------------------------------------------------------
    void SetDescriptorHeaps(ID3D12GraphicsCommandList* pCmdList);

    //-------------------------------------------------------------------------
    //! @brief      コマンドキューの実行完了を待機します.
    //-------------------------------------------------------------------------
    void WaitIdle();

    //-------------------------------------------------------------------------
    //! @brief      オブジェクトディスポーザーに追加します.
    //!
    //! @param[in]      pResource       破棄リソース.
    //--------------------------------------------------------------------------
    void Dispose(ID3D12Object*& pResource);

    //-------------------------------------------------------------------------
    //! @brief      ディスクリプタディスポーザーに追加します.
    //!
    //! @param[in]      pDescriptor     破棄ディスクリプタ.
    //-------------------------------------------------------------------------
    void Dispose(Descriptor*& pDescriptor);

    //-------------------------------------------------------------------------
    //! @brief      フレーム同期を行います.
    //-------------------------------------------------------------------------
    void FrameSync();

    //-------------------------------------------------------------------------
    //! @brief      強制破棄を行います.
    //-------------------------------------------------------------------------
    void ClearDisposer();

    //-------------------------------------------------------------------------
    //! @brief      サポートされているディスプレイ解像度を取得します.
    //! 
    //! @param[in]          format          フォーマット.
    //! @param[out]         infos           ディスプレイ情報.
    //-------------------------------------------------------------------------
    void GetDisplayInfo(DXGI_FORMAT format, std::vector<DisplayInfo>& infos);

    //-------------------------------------------------------------------------
    //! @brief      フルスクリーン矩形用頂点バッファを取得します.
    //-------------------------------------------------------------------------
    const VertexBuffer& GetQuadVB() const { return m_QuadVB; }

    //-------------------------------------------------------------------------
    //! @brief      コマンドシグニチャを取得します.
    //-------------------------------------------------------------------------
    ID3D12CommandSignature* GetCommandSignature(COMMAND_SIGNATURE_TYPE type) const
    { return m_pCommandSig[type]; }

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    static GraphicsSystem           s_Instance;                 //!< シングルトンインスタンス.
    RefPtr<IDXGIFactory7>           m_pFactory;                 //!< DXGIファクトリーです.
    RefPtr<IDXGIAdapter1>           m_pAdapter;                 //!< DXGIアダプターです.
    RefPtr<IDXGIOutput6>            m_pOutput;                  //!< DXGIアウトプットです.
    RefPtr<ID3D12Debug3>            m_pDebug;                   //!< デバッグオブジェクト.
    RefPtr<ID3D12InfoQueue>         m_pInfoQueue;               //!< インフォキュー.
    RefPtr<ID3D12Device8>           m_pDevice;                  //!< デバイス.
    RefPtr<CommandQueue>            m_pGraphicsQueue;           //!< グラフィックスキュー.
    RefPtr<CommandQueue>            m_pComputeQueue;            //!< コンピュートキュー.
    RefPtr<CommandQueue>            m_pCopyQueue;               //!< コピーキュー.
    RefPtr<CommandQueue>            m_pVideoDecodeQueue;        //!< ビデオデコードキュー.
    RefPtr<CommandQueue>            m_pVideoProcessQueue;       //!< ビデオプロセスキュー.
    RefPtr<CommandQueue>            m_pVideoEncodeQueue;        //!< ビデオエンコードキュー.
    DescriptorHeap                  m_DescriptorHeap[4];        //!< ディスクリプタヒープ.
    Disposer<ID3D12Object>          m_ObjectDisposer;           //!< オブジェクトディスポーザー.
    Disposer<Descriptor>            m_DescriptorDisposer;       //!< ディスクリプタディスポーザー.
    SpinLock                        m_SpinLock;                 //!< スピンロックです.
    VertexBuffer                    m_QuadVB;
    ID3D12CommandSignature*         m_pCommandSig[MAX_COUNT_COMMAND_SIGNATURE_TYPE] = {};

    //=========================================================================
    // private methods
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //-------------------------------------------------------------------------
    GraphicsSystem() = default;

    //-------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //-------------------------------------------------------------------------
    ~GraphicsSystem() = default;

    GraphicsSystem              (const GraphicsSystem&) = delete;   // アクセス禁止.
    GraphicsSystem& operator =  (const GraphicsSystem&) = delete;   // アクセス禁止.
};



///////////////////////////////////////////////////////////////////////////////
// Descriptor class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
Descriptor::Descriptor()
: List<Descriptor>::Node()
, m_pHeap       (nullptr)
, m_HandleCPU   ()
, m_HandleGPU   ()
, m_Index       (UINT32_MAX)
, m_RefCount    (1)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
Descriptor::~Descriptor()
{
    m_pHeap = nullptr;
}

//-----------------------------------------------------------------------------
//      参照カウントを増やします.
//-----------------------------------------------------------------------------
void Descriptor::AddRef()
{ m_RefCount++; }

//-----------------------------------------------------------------------------
//      解放処理を行います.
//-----------------------------------------------------------------------------
void Descriptor::Release()
{
    if (m_RefCount > 0)
    {
        m_RefCount--;
        if (m_RefCount == 0)
        {
            auto pHeap = m_pHeap;
            if (pHeap != nullptr)
            { pHeap->Free(this); }
        }
    }
}

//-----------------------------------------------------------------------------
//      参照カウントを取得します.
//-----------------------------------------------------------------------------
uint32_t Descriptor::GetCount() const
{ return m_RefCount; }

//-----------------------------------------------------------------------------
//      CPUディスクリプタハンドルを取得します.
//-----------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE Descriptor::GetHandleCPU() const
{ return m_HandleCPU; }

//-----------------------------------------------------------------------------
//      GPUディスクリプタハンドルを取得します.
//-----------------------------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE Descriptor::GetHandleGPU() const
{ return m_HandleGPU; }

//-----------------------------------------------------------------------------
//      インデックスを取得します.
//-----------------------------------------------------------------------------
uint32_t Descriptor::GetIndex() const
{ return m_Index; }


///////////////////////////////////////////////////////////////////////////////
// DescritptorHeap class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
DescriptorHeap::DescriptorHeap()
: m_pHeap           (nullptr)
, m_FreeList        ()
, m_UsedList        ()
, m_Descriptors     (nullptr)
, m_IncrementSize   (0)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
DescriptorHeap::~DescriptorHeap()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool DescriptorHeap::Init(ID3D12Device* pDevice, const D3D12_DESCRIPTOR_HEAP_DESC* pDesc)
{
    if (pDevice == nullptr || pDesc == nullptr)
    { return false; }

    if (pDesc->NumDescriptors == 0)
    { return true; }

    auto hr = pDevice->CreateDescriptorHeap(pDesc, IID_PPV_ARGS(&m_pHeap));
    if ( FAILED(hr) )
    {
        ELOG("Error : ID3D12Device::CreateDescriptorHeap() Failed. errcode = 0x%x", hr);
        return false;
    }

    m_pHeap->SetName(L"asdxDescriptorHeap");

    // インクリメントサイズを取得.
    m_IncrementSize = pDevice->GetDescriptorHandleIncrementSize(pDesc->Type);

    // ディスクリプタ生成.
    m_Descriptors = new Descriptor[pDesc->NumDescriptors];

    auto hasHandleGPU = (pDesc->Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    for(auto i=0u; i<pDesc->NumDescriptors; ++i)
    {
        // ヒープを設定.
        m_Descriptors[i].m_pHeap = this;

        // Dynamic Resource 用の番号を割り当て.
        m_Descriptors[i].m_Index = i;

        // CPUハンドルをディスクリプタを割り当て.
        {
            auto handleCPU = m_pHeap->GetCPUDescriptorHandleForHeapStart();
            handleCPU.ptr += UINT64(m_IncrementSize) * i;
            m_Descriptors[i].m_HandleCPU = handleCPU;
        }

        // GPUハンドルをディスクリプタを割り当て
        if (hasHandleGPU)
        {
            auto handleGPU = m_pHeap->GetGPUDescriptorHandleForHeapStart();
            handleGPU.ptr += UINT64(m_IncrementSize) * i;
            m_Descriptors[i].m_HandleGPU = handleGPU;
        }

        // 未使用リストに追加.
        m_FreeList.push_back(&m_Descriptors[i]);
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void DescriptorHeap::Term()
{
    /* この関数内で落ちる場合は解放漏れがあります */
    m_FreeList.clear();
    m_UsedList.clear();

    if (m_pHeap != nullptr)
    {
        m_pHeap->Release();
        m_pHeap = nullptr;
    }

    if (m_Descriptors != nullptr)
    {
        delete[] m_Descriptors;
        m_Descriptors = nullptr;
    }
}

//-----------------------------------------------------------------------------
//      ディスクリプタを生成します.
//-----------------------------------------------------------------------------
Descriptor* DescriptorHeap::Alloc()
{
    auto& node = m_FreeList.front();
    m_UsedList.push_back(&node);
    return &node;
}

//-----------------------------------------------------------------------------
//      ディスクリプタを破棄します.
//-----------------------------------------------------------------------------
void DescriptorHeap::Free(Descriptor* pValue)
{
    if (pValue == nullptr)
    { return; }

    m_UsedList.erase(pValue);
    m_FreeList.push_back(pValue);
}

//-----------------------------------------------------------------------------
//      ディスクリプタヒープを取得します.
//-----------------------------------------------------------------------------
ID3D12DescriptorHeap* DescriptorHeap::GetD3D12DescriptorHeap() const
{ return m_pHeap; }

//-----------------------------------------------------------------------------
//      割り当て済みハンドル数を取得します.
//-----------------------------------------------------------------------------
uint32_t DescriptorHeap::GetAllocatedCount() const
{ return uint32_t(m_UsedList.size()); }

//-----------------------------------------------------------------------------
//      割り当て可能なハンドル数を取得します.
//-----------------------------------------------------------------------------
uint32_t DescriptorHeap::GetAvailableCount() const
{ return uint32_t(m_FreeList.size()); }

//-----------------------------------------------------------------------------
//      ハンドル数を取得します.
//-----------------------------------------------------------------------------
uint32_t DescriptorHeap::GetHandleCount() const
{ return uint32_t(m_UsedList.size() + m_FreeList.size()); }
 

///////////////////////////////////////////////////////////////////////////////
// GraphicsSystem
///////////////////////////////////////////////////////////////////////////////
GraphicsSystem GraphicsSystem::s_Instance = {};

//-----------------------------------------------------------------------------
//      唯一のインスタンスを取得します.
//-----------------------------------------------------------------------------
GraphicsSystem& GraphicsSystem::Instance()
{ return s_Instance; }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool GraphicsSystem::Init(const DeviceDesc& deviceDesc)
{
    // PIXキャプチャー設定.
    if (deviceDesc.EnableCapture)
    { LoadPixGpuCpatureDll(); }

    if (deviceDesc.EnableDebug)
    {
        asdx::RefPtr<ID3D12Debug> debug;
        auto hr = D3D12GetDebugInterface(IID_PPV_ARGS(debug.GetAddress()));
        if (SUCCEEDED(hr))
        {
            hr = debug->QueryInterface(IID_PPV_ARGS(m_pDebug.GetAddress()));
            if (SUCCEEDED(hr))
            {
                m_pDebug->EnableDebugLayer();
                m_pDebug->SetEnableGPUBasedValidation(TRUE);
            }
        }
    }

    if (deviceDesc.EnableDRED)
    {
        // DRED有効化.
        asdx::RefPtr<ID3D12DeviceRemovedExtendedDataSettings1> dred;
        auto hr = D3D12GetDebugInterface(IID_PPV_ARGS(dred.GetAddress()));
        if (SUCCEEDED(hr))
        {
            dred->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            dred->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            dred->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        }
    }

    // DXGIファクトリを生成.
    {
        uint32_t flags = 0;
        if (deviceDesc.EnableDebug)
        { flags |= DXGI_CREATE_FACTORY_DEBUG; }

        RefPtr<IDXGIFactory2> factory;
        auto hr = CreateDXGIFactory2( flags, IID_PPV_ARGS(factory.GetAddress()) );
        if ( FAILED(hr) )
        {
            ELOG("Error : CreateDXGIFactory2() Failed. errcode = 0x%x", hr);
            return false;
        }

        hr = factory->QueryInterface(IID_PPV_ARGS(m_pFactory.GetAddress()));
        if ( FAILED(hr) )
        {
            ELOG("Error : QueryInterface() Failed. errcode = 0x%x", hr);
            return false;
        }
    }

    // DXGIアダプター生成.
    {
        RefPtr<IDXGIAdapter1> pAdapter;
        for(auto adapterId=0;
            DXGI_ERROR_NOT_FOUND != m_pFactory->EnumAdapterByGpuPreference(adapterId, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(pAdapter.ReleaseAndGetAddress()));
            adapterId++)
        {
            DXGI_ADAPTER_DESC1 desc;
            auto hr = pAdapter->GetDesc1(&desc);
            if (FAILED(hr))
            { continue; }

            hr = D3D12CreateDevice(pAdapter.GetPtr(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr);
            if (SUCCEEDED(hr))
            {
                if (m_pAdapter.GetPtr() == nullptr)
                { m_pAdapter = pAdapter.GetPtr(); }

                RefPtr<IDXGIOutput> pOutput;
                hr = pAdapter->EnumOutputs(0, pOutput.GetAddress());
                if (FAILED(hr))
                { continue; }

                hr = pOutput->QueryInterface(IID_PPV_ARGS(m_pOutput.GetAddress()));
                if (SUCCEEDED(hr))
                { break; }
            }
        }
    }

    // デバイス生成.
    {
        asdx::RefPtr<ID3D12Device> device;
        auto hr = D3D12CreateDevice( m_pAdapter.GetPtr(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(device.GetAddress()) );
        if (FAILED(hr))
        {
            ELOG("Error : D3D12CreateDevice() Failed. errcode = 0x%x", hr);
            return false;
        }

        // ID3D12Device8に変換.
        hr = device->QueryInterface(IID_PPV_ARGS(m_pDevice.GetAddress()));
        if (FAILED(hr))
        {
            ELOG("Error : QueryInterface() Failed. errcode = 0x%x", hr);
            return false;
        }

        m_pDevice->SetName(L"asdxDevice");

        // ID3D12InfoQueueに変換.
        if (deviceDesc.EnableDebug)
        {
            hr = m_pDevice->QueryInterface(IID_PPV_ARGS(m_pInfoQueue.GetAddress()));
            if (SUCCEEDED(hr))
            {
                // エラー発生時にブレークさせる.
                if (deviceDesc.EnableBreakOnError)
                { m_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE); }

                // 警告発生時にブレークさせる.
                if (deviceDesc.EnableBreakOnWarning)
                { m_pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE); }

                // 無視するメッセージID.
                D3D12_MESSAGE_ID denyIds[] = {
                    D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                    D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
                };

                // 無視するメッセージレベル.
                D3D12_MESSAGE_SEVERITY severities[] = {
                    D3D12_MESSAGE_SEVERITY_INFO
                };

                D3D12_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs          = _countof(denyIds);
                filter.DenyList.pIDList         = denyIds;
                filter.DenyList.NumSeverities   = _countof(severities);
                filter.DenyList.pSeverityList   = severities;

                m_pInfoQueue->PushStorageFilter(&filter);
            }
        }
    }

    // 定数バッファ・シェーダリソース・アンオーダードアクセスビュー用ディスクリプタヒープ.
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = deviceDesc.MaxShaderResourceCount;
        desc.Type  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if ( !m_DescriptorHeap[desc.Type].Init(m_pDevice.GetPtr(), &desc ) )
        {
            ELOG("Error : DescriptorHeap::Init() Failed.");
            return false;
        }
    }

    // サンプラー用ディスクリプタヒープ.
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = deviceDesc.MaxSamplerCount;
        desc.Type  = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if ( !m_DescriptorHeap[desc.Type].Init(m_pDevice.GetPtr(), &desc ) )
        {
            ELOG("Error : DescriptorHeap::Init() Failed");
            return false;
        }
    }

    // レンダーターゲットビュー用ディスクリプタヒープ.
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = deviceDesc.MaxColorTargetCount;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        if ( !m_DescriptorHeap[desc.Type].Init(m_pDevice.GetPtr(), &desc ) )
        {
            ELOG("Error : DescriptorHeap::Init() Failed.");
            return false;
        }
    }

    // 深度ステンシルビュー用ディスクリプタヒープ.
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = deviceDesc.MaxDepthTargetCount;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        if ( !m_DescriptorHeap[desc.Type].Init(m_pDevice.GetPtr(), &desc ) )
        {
            ELOG("Error : DescriptorHeap::Init() Failed");
            return false;
        }
    }

    // グラフィックスキューの生成.
    auto ret = CommandQueue::Create(
        m_pDevice.GetPtr(), D3D12_COMMAND_LIST_TYPE_DIRECT, m_pGraphicsQueue.GetAddress());
    if (!ret)
    {
        ELOG("Error : Queue::Create() Failed.");
        return false;
    }

    // コンピュートキューの生成.
    ret = CommandQueue::Create(
        m_pDevice.GetPtr(), D3D12_COMMAND_LIST_TYPE_COMPUTE, m_pComputeQueue.GetAddress());
    if (!ret)
    {
        ELOG("Error : Queue::Create() Failed.");
        return false;
    }

    // コピーキューの生成.
    ret = CommandQueue::Create(
        m_pDevice.GetPtr(), D3D12_COMMAND_LIST_TYPE_COPY, m_pCopyQueue.GetAddress());
    if (!ret)
    {
        ELOG("Error : Queue::Create() Failed.");
        return false;
    }

    if (deviceDesc.EnableVideoQueue)
    {
        // ビデオデコードキューの生成.
        ret = CommandQueue::Create(
            m_pDevice.GetPtr(), D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE, m_pVideoDecodeQueue.GetAddress());
        if (!ret)
        {
            ELOG("Error : Queue::Create() Failed.");
            return false;
        }

        // ビデオプロセスキューの生成.
        ret = CommandQueue::Create(
            m_pDevice.GetPtr(), D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS, m_pVideoProcessQueue.GetAddress());
        if (!ret)
        {
            ELOG("Error : Queue::Create() Failed.");
            return false;
        }

        // ビデオエンコードキューの生成.
        ret = CommandQueue::Create(
            m_pDevice.GetPtr(), D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE, m_pVideoEncodeQueue.GetAddress());
        if (!ret)
        {
            ELOG("Error : Queue::Create() Failed.");
            return false;
        }
    }

    // 矩形用
    {
        QuadVertex vertices[] = {
            QuadVertex(-1.0f,  1.0f, 0.0f,  0.0f),
            QuadVertex( 3.0f,  1.0f, 2.0f,  0.0f),
            QuadVertex(-1.0f, -3.0f, 0.0f,  2.0f)
        };

        auto size = sizeof(vertices);
        auto stride = uint32_t(sizeof(vertices[0]));

        if (!m_QuadVB.Init(size, stride))
        {
            ELOG("Error : VertexBuffer::Init() Failed.");
            return false;
        }

        auto dst = m_QuadVB.Map<QuadVertex>();
        memcpy(dst, vertices, size);
        m_QuadVB.Unmap();
    }

#if 1
    // コマンドシグニチャ生成.
    {
        {
            D3D12_INDIRECT_ARGUMENT_DESC arg = {};
            arg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

            D3D12_COMMAND_SIGNATURE_DESC desc = {};
            desc.ByteStride         = sizeof(D3D12_DRAW_ARGUMENTS);
            desc.NumArgumentDescs   = 1;
            desc.pArgumentDescs     = &arg;

            auto hr = m_pDevice->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(&m_pCommandSig[COMMAND_SIGNATURE_TYPE_DRAW]));
            if (FAILED(hr))
            {
                ELOG("Error : ID3D12Device::CreateCommandSignature() Failed. errcode = 0x%x", hr);
                return false;
            }
        }

        {
            D3D12_INDIRECT_ARGUMENT_DESC arg = {};
            arg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

            D3D12_COMMAND_SIGNATURE_DESC desc = {};
            desc.ByteStride         = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
            desc.NumArgumentDescs   = 1;
            desc.pArgumentDescs     = &arg;

            auto hr = m_pDevice->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(&m_pCommandSig[COMMAND_SIGNATURE_TYPE_DRAW_INDEXED]));
            if (FAILED(hr))
            {
                ELOG("Error : ID3D12Device::CreateCommandSignature() Failed. errcode = 0x%x", hr);
                return false;
            }
        }

        {
            D3D12_INDIRECT_ARGUMENT_DESC arg = {};
            arg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

            D3D12_COMMAND_SIGNATURE_DESC desc = {};
            desc.ByteStride         = sizeof(D3D12_DISPATCH_ARGUMENTS);
            desc.NumArgumentDescs   = 1;
            desc.pArgumentDescs     = &arg;

            auto hr = m_pDevice->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(&m_pCommandSig[COMMAND_SIGNATURE_TYPE_DISPATCH]));
            if (FAILED(hr))
            {
                ELOG("Error : ID3D12Device::CreateCommandSignature() Failed. errcode = 0x%x", hr);
                return false;
            }
        }

        {
            D3D12_INDIRECT_ARGUMENT_DESC arg = {};
            arg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS;

            D3D12_COMMAND_SIGNATURE_DESC desc = {};
            desc.ByteStride         = sizeof(D3D12_DISPATCH_RAYS_DESC);
            desc.NumArgumentDescs   = 1;
            desc.pArgumentDescs     = &arg;

            auto hr = m_pDevice->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(&m_pCommandSig[COMMAND_SIGNATURE_TYPE_DISPATCH_RAYS]));
            if (FAILED(hr))
            {
                ELOG("Error : ID3D12Device::CreateCommandSignature() Failed. errcode = 0x%x", hr);
                return false;
            }
        }

        {
            D3D12_INDIRECT_ARGUMENT_DESC arg = {};
            arg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;

            D3D12_COMMAND_SIGNATURE_DESC desc = {};
            desc.ByteStride         = sizeof(D3D12_DISPATCH_MESH_ARGUMENTS);
            desc.NumArgumentDescs   = 1;
            desc.pArgumentDescs     = &arg;

            auto hr = m_pDevice->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(&m_pCommandSig[COMMAND_SIGNATURE_TYPE_DISPATCH_MESH]));
            if (FAILED(hr))
            {
                ELOG("Error : ID3D12Device::CreateCommandSignature() Failed. errcode = 0x%x", hr);
                return false;
            }
        }
    }
#endif

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void GraphicsSystem::Term()
{
    for(auto i=0; i<MAX_COUNT_COMMAND_SIGNATURE_TYPE; ++i)
    {
        if (m_pCommandSig[i] != nullptr)
        {
            m_pCommandSig[i]->Release();
            m_pCommandSig[i] = nullptr;
        }
    }

    m_QuadVB.Term();

    m_ObjectDisposer    .Clear();
    m_DescriptorDisposer.Clear();

    m_pGraphicsQueue    .Reset();
    m_pComputeQueue     .Reset();
    m_pCopyQueue        .Reset();
    m_pVideoDecodeQueue .Reset();
    m_pVideoProcessQueue.Reset();
    m_pVideoEncodeQueue .Reset();

    for(auto i=0; i<4; ++i)
    {
        m_DescriptorHeap[i].Term();
    }

    m_pOutput   .Reset();
    m_pDevice   .Reset();
    m_pInfoQueue.Reset();
    m_pDebug    .Reset();
    m_pAdapter  .Reset();
    m_pFactory  .Reset();
}

//-----------------------------------------------------------------------------
//      ID3D12Device8を取得します.
//-----------------------------------------------------------------------------
ID3D12Device8* GraphicsSystem::GetDevice() const
{ return m_pDevice.GetPtr(); }

//-----------------------------------------------------------------------------
//      IDXGIFactory7を取得します.
//-----------------------------------------------------------------------------
IDXGIFactory7* GraphicsSystem::GetFactory() const
{ return m_pFactory.GetPtr(); }

//-----------------------------------------------------------------------------
//      グラフィックスキューを取得します.
//-----------------------------------------------------------------------------
CommandQueue* GraphicsSystem::GetGraphicsQueue() const
{ return m_pGraphicsQueue.GetPtr(); }

//-----------------------------------------------------------------------------
//      コンピュートキューを取得します.
//-----------------------------------------------------------------------------
CommandQueue* GraphicsSystem::GetComputeQueue() const
{ return m_pComputeQueue.GetPtr(); }

//-----------------------------------------------------------------------------
//      コピーキューを取得します.
//-----------------------------------------------------------------------------
CommandQueue* GraphicsSystem::GetCopyQueue() const
{ return m_pCopyQueue.GetPtr(); }

//-----------------------------------------------------------------------------
//      ビデオデコードキューを取得します.
//-----------------------------------------------------------------------------
CommandQueue* GraphicsSystem::GetVideoDecodeQueue() const
{ return m_pVideoDecodeQueue.GetPtr(); }

//-----------------------------------------------------------------------------
//      ビデオプロセスキューを取得します.
//-----------------------------------------------------------------------------
CommandQueue* GraphicsSystem::GetVideoProcessQueue() const
{ return m_pVideoProcessQueue.GetPtr(); }

//-----------------------------------------------------------------------------
//      ビデオエンコードキューを取得します.
//-----------------------------------------------------------------------------
CommandQueue* GraphicsSystem::GetVideoEncodeQueue() const
{ return m_pVideoEncodeQueue.GetPtr(); }

//-----------------------------------------------------------------------------
//      ディスクリプターを確保します.
//-----------------------------------------------------------------------------
bool GraphicsSystem::AllocHandle(uint8_t heapType, Descriptor** ppDescriptor)
{
    ScopedLock locker(&m_SpinLock);

    if (heapType >= 4)
    { return false; }

    auto descriptor = m_DescriptorHeap[heapType].Alloc();
    if (descriptor == nullptr)
    { return false; }

    *ppDescriptor = descriptor;
    return true;
}

//-----------------------------------------------------------------------------
//      アロー演算子です.
//-----------------------------------------------------------------------------
ID3D12Device8* GraphicsSystem::operator-> () const
{ return m_pDevice.GetPtr(); }

//-----------------------------------------------------------------------------
//      ディスクリプタヒープを設定します.
//-----------------------------------------------------------------------------
void GraphicsSystem::SetDescriptorHeaps(ID3D12GraphicsCommandList* pCmdList)
{
    if (pCmdList == nullptr)
    { return; }

    // コピーコマンドリストには SetDescriptoreHeaps() は使えない.
    if (pCmdList->GetType() == D3D12_COMMAND_LIST_TYPE_COPY)
    { return; }

    ID3D12DescriptorHeap* pHeaps[] = {
        m_DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetD3D12DescriptorHeap(),
        m_DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER].GetD3D12DescriptorHeap()
    };

    pCmdList->SetDescriptorHeaps(2, pHeaps);
}

//-----------------------------------------------------------------------------
//      コマンドキューの実行完了を待機します.
//-----------------------------------------------------------------------------
void GraphicsSystem::WaitIdle()
{
    if (m_pGraphicsQueue.GetPtr() != nullptr)
    {
        auto waitPoint = m_pGraphicsQueue->Signal();
        m_pGraphicsQueue->Sync(waitPoint);
    }

    if (m_pComputeQueue.GetPtr() != nullptr)
    {
        auto waitPoint = m_pComputeQueue->Signal();
        m_pComputeQueue->Sync(waitPoint);
    }

    if (m_pCopyQueue.GetPtr() != nullptr)
    {
        auto waitPoint = m_pCopyQueue->Signal();
        m_pCopyQueue->Sync(waitPoint);
    }

    if (m_pVideoDecodeQueue.GetPtr() != nullptr)
    {
        auto waitPoint = m_pVideoDecodeQueue->Signal();
        m_pVideoDecodeQueue->Sync(waitPoint);
    }

    if (m_pVideoEncodeQueue.GetPtr() != nullptr)
    {
        auto waitPoint = m_pVideoEncodeQueue->Signal();
        m_pVideoEncodeQueue->Sync(waitPoint);
    }

    if (m_pVideoProcessQueue.GetPtr() != nullptr)
    {
        auto waitPoint = m_pVideoProcessQueue->Signal();
        m_pVideoProcessQueue->Sync(waitPoint);
    }
}

//-----------------------------------------------------------------------------
//      オブジェクトディスポーザーに追加します.
//-----------------------------------------------------------------------------
void GraphicsSystem::Dispose(ID3D12Object*& pResource)
{ m_ObjectDisposer.Push(pResource); }

//-----------------------------------------------------------------------------
//      ディスクリプタディスポーザーに追加します.
//-----------------------------------------------------------------------------
void GraphicsSystem::Dispose(Descriptor*& pDescriptor)
{ m_DescriptorDisposer.Push(pDescriptor); }

//-----------------------------------------------------------------------------
//      フレーム同期を取ります.
//-----------------------------------------------------------------------------
void GraphicsSystem::FrameSync()
{
    m_ObjectDisposer    .FrameSync();
    m_DescriptorDisposer.FrameSync();
}

//-----------------------------------------------------------------------------
//      強制破棄を行います.
//-----------------------------------------------------------------------------
void GraphicsSystem::ClearDisposer()
{
    m_ObjectDisposer    .Clear();
    m_DescriptorDisposer.Clear();
}

//-----------------------------------------------------------------------------
//      ディスプレイ情報を取得します.
//-----------------------------------------------------------------------------
void GraphicsSystem::GetDisplayInfo(DXGI_FORMAT format, std::vector<DisplayInfo>& infos)
{
    if (!m_pOutput)
    { return; }

    UINT count = 0;
    auto hr = m_pOutput->GetDisplayModeList(format, DXGI_ENUM_MODES_SCALING, &count, nullptr);
    if (FAILED(hr) || count == 0)
    { return; }

    std::vector<DXGI_MODE_DESC> descs;
    descs.resize(count);

    hr = m_pOutput->GetDisplayModeList(format, DXGI_ENUM_MODES_SCALING, &count, descs.data());
    if (FAILED(hr))
    { return; }

    infos.resize(count);
    for(size_t i=0; i<infos.size(); ++i)
    {
        infos[i].Width          = descs[i].Width;
        infos[i].Height         = descs[i].Height;
        infos[i].RefreshRate    = descs[i].RefreshRate;
    }

    // 解像度が大きい順にする.
    std::sort(infos.begin(), infos.end(), [](DisplayInfo& lhs, DisplayInfo& rhs)
    {
        auto refreshRateLhs = double(lhs.RefreshRate.Numerator) / double(lhs.RefreshRate.Denominator);
        auto refreshRateRhs = double(rhs.RefreshRate.Numerator) / double(rhs.RefreshRate.Denominator);
        return std::tie(lhs.Width, lhs.Height, refreshRateLhs) > std::tie(rhs.Width, rhs.Height, refreshRateRhs);
    });
}

//-----------------------------------------------------------------------------
//      システムを初期化します.
//-----------------------------------------------------------------------------
bool SystemInit(const DeviceDesc& desc)
{ return GraphicsSystem::Instance().Init(desc); }

//-----------------------------------------------------------------------------
//      システムを終了します.
//-----------------------------------------------------------------------------
void SystemTerm()
{ GraphicsSystem::Instance().Term(); }

//-----------------------------------------------------------------------------
//      システムがアイドル状態になるまで待機します.
//-----------------------------------------------------------------------------
void SystemWaitIdle()
{ GraphicsSystem::Instance().WaitIdle(); }

//-----------------------------------------------------------------------------
//      フレーム同期します.
//-----------------------------------------------------------------------------
void FrameSync()
{ GraphicsSystem::Instance().FrameSync(); }

//-----------------------------------------------------------------------------
//      オブジェクトを破棄します.
//-----------------------------------------------------------------------------
void DisposeObject(ID3D12Object*& pObject)
{ GraphicsSystem::Instance().Dispose(pObject); }

//-----------------------------------------------------------------------------
//      ディスクリプタを破棄します.
//-----------------------------------------------------------------------------
void DisposeDescriptor(Descriptor*& pDescriptor)
{ GraphicsSystem::Instance().Dispose(pDescriptor); }

//-----------------------------------------------------------------------------
//      ディスポーザーをクリアします.
//-----------------------------------------------------------------------------
void ClearDisposer()
{ GraphicsSystem::Instance().ClearDisposer(); }

//-----------------------------------------------------------------------------
//      ディスクリプタヒープを設定します.
//-----------------------------------------------------------------------------
void SetDescriptorHeaps(ID3D12GraphicsCommandList* pCmd)
{ GraphicsSystem::Instance().SetDescriptorHeaps(pCmd); }

//-----------------------------------------------------------------------------
//      グラフィックスキューを取得します.
//-----------------------------------------------------------------------------
CommandQueue* GetGraphicsQueue()
{ return GraphicsSystem::Instance().GetGraphicsQueue(); }

//-----------------------------------------------------------------------------
//      コンピュートキューを取得します.
//-----------------------------------------------------------------------------
CommandQueue* GetComputeQueue()
{ return GraphicsSystem::Instance().GetComputeQueue(); }

//-----------------------------------------------------------------------------
//      コピーキューを取得します.
//-----------------------------------------------------------------------------
CommandQueue* GetCopyQueue()
{ return GraphicsSystem::Instance().GetCopyQueue(); }

//-----------------------------------------------------------------------------
//      ビデオプロセスキューを取得します.
//-----------------------------------------------------------------------------
CommandQueue* GetVideoProcessQueue()
{ return GraphicsSystem::Instance().GetVideoProcessQueue(); }

//-----------------------------------------------------------------------------
//      ビデオエンコードキューを取得します.
//-----------------------------------------------------------------------------
CommandQueue* GetVideoEncodeQueue()
{ return GraphicsSystem::Instance().GetVideoEncodeQueue(); }

//-----------------------------------------------------------------------------
//      ビデオデコードキューを取得します.
//-----------------------------------------------------------------------------
CommandQueue* GetVideoDecodeQueue()
{ return GraphicsSystem::Instance().GetVideoDecodeQueue(); }

//-----------------------------------------------------------------------------
//      ディスクリプタを割り当てます.
//-----------------------------------------------------------------------------
bool AllocDescriptor(uint8_t heapType, Descriptor** ppResult)
{ return GraphicsSystem::Instance().AllocHandle(heapType, ppResult); }

//-----------------------------------------------------------------------------
//      デバイスを取得します.
//-----------------------------------------------------------------------------
ID3D12Device8* GetD3D12Device() 
{ return GraphicsSystem::Instance().GetDevice(); }

//-----------------------------------------------------------------------------
//      DXGIファクトリを取得します.
//-----------------------------------------------------------------------------
IDXGIFactory7* GetDXGIFactory()
{ return GraphicsSystem::Instance().GetFactory(); }

//-----------------------------------------------------------------------------
//      ディスプレイ情報を取得します.
//-----------------------------------------------------------------------------
void GetDisplayInfo(DXGI_FORMAT format, std::vector<DisplayInfo>& result)
{ GraphicsSystem::Instance().GetDisplayInfo(format, result); }

//-----------------------------------------------------------------------------
//      サブリソースを更新します.
//-----------------------------------------------------------------------------
void UpdateSubResources
(
    ID3D12GraphicsCommandList*      pCmdList,
    ID3D12Resource*                 pDstResource,
    uint32_t                        subResourceCount,
    uint32_t                        subResourceOffset,
    const D3D12_SUBRESOURCE_DATA*   subResources
)
{
    if (pCmdList == nullptr || pDstResource == nullptr || subResources == nullptr || subResourceCount == 0)
    { return; }

    auto device  = GetD3D12Device();
    auto dstDesc = pDstResource->GetDesc();

    D3D12_RESOURCE_DESC uploadDesc = {
        D3D12_RESOURCE_DIMENSION_BUFFER,
        0,
        GetRequiredIntermediateSize(device, &dstDesc, subResourceOffset, subResourceCount),
        1,
        1,
        1,
        DXGI_FORMAT_UNKNOWN,
        { 1, 0 },
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_NONE
    };

    D3D12_HEAP_PROPERTIES props = {
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        1,
        1
    };

    ID3D12Resource* pSrcResource = nullptr;
    auto hr = device->CreateCommittedResource(
        &props,
        D3D12_HEAP_FLAG_NONE,
        &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&pSrcResource));
    if (FAILED(hr))
    {
        ELOG("Error : ID3D12Device::CreateCommitedResource() Failed. errcode = 0x%x", hr);
        return;
    }

    // コマンドを生成.
    {
        auto count = subResourceCount;

        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts;
        std::vector<UINT>                               rows;
        std::vector<UINT64>                             rowSizeInBytes;

        layouts       .resize(count);
        rows          .resize(count);
        rowSizeInBytes.resize(count);

        UINT64 requiredSize = 0;
        device->GetCopyableFootprints(
            &dstDesc, subResourceOffset, count, 0, layouts.data(), rows.data(), rowSizeInBytes.data(), &requiredSize);

        BYTE* pData = nullptr;
        hr = pSrcResource->Map(0, nullptr, reinterpret_cast<void**>(&pData));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Resource::Map() Failed. errcode = 0x%x", hr);
            pSrcResource->Release();
            return;
        }

        for(auto i=0u; i<count; ++i)
        {
            D3D12_MEMCPY_DEST dstData = {};
            dstData.pData       = pData + layouts[i].Offset;
            dstData.RowPitch    = layouts[i].Footprint.RowPitch;
            dstData.SlicePitch  = SIZE_T(layouts[i].Footprint.RowPitch) * SIZE_T(rows[i]);

            CopySubresource(
                &dstData,
                &subResources[i],
                SIZE_T(rowSizeInBytes[i]),
                rows[i],
                layouts[i].Footprint.Depth);
        }
        pSrcResource->Unmap(0, nullptr);

        if (dstDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            pCmdList->CopyBufferRegion(
                pDstResource,
                0,
                pSrcResource,
                layouts[0].Offset,
                layouts[0].Footprint.Width);
        }
        else
        {
            for(auto i=0u; i<count; ++i)
            {
                D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
                dstLoc.pResource        = pDstResource;
                dstLoc.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                dstLoc.SubresourceIndex = i + subResourceOffset;

                D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
                srcLoc.pResource        = pSrcResource;
                srcLoc.Type             = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                srcLoc.PlacedFootprint  = layouts[i];

                pCmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
            }
        }
    }

    Dispose(pSrcResource);
}

//-----------------------------------------------------------------------------
//      バッファを更新します.
//-----------------------------------------------------------------------------
void UpdateBuffer
(
    ID3D12GraphicsCommandList*  pCmdList,
    ID3D12Resource*             pDstResource,
    const void*                 pSrcResource
)
{
    if (pCmdList == nullptr || pSrcResource == nullptr || pDstResource == nullptr)
    { return; }

    auto desc = pDstResource->GetDesc();

    D3D12_SUBRESOURCE_DATA res = {};
    res.pData       = pSrcResource;
    res.RowPitch    = desc.Width;
    res.SlicePitch  = desc.Width;

    UpdateSubResources(pCmdList, pDstResource, 1, 0, &res);
}

//-----------------------------------------------------------------------------
//      テクスチャを更新します.
//-----------------------------------------------------------------------------
void UpdateTexture
(
    ID3D12GraphicsCommandList*  pCmdList,
    ID3D12Resource*             pDstResource,
    const ResTexture*           pResTexture
)
{
    if (pCmdList == nullptr || pDstResource == nullptr || pResTexture == nullptr)
    { return; }

    auto device = GetD3D12Device();
    auto dstDesc = pDstResource->GetDesc();

    auto count = pResTexture->MipMapCount * pResTexture->SurfaceCount;

    D3D12_RESOURCE_DESC uploadDesc = {
        D3D12_RESOURCE_DIMENSION_BUFFER,
        0,
        GetRequiredIntermediateSize(device, &dstDesc, 0, count),
        1,
        1,
        1,
        DXGI_FORMAT_UNKNOWN,
        { 1, 0 },
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_NONE
    };

    D3D12_HEAP_PROPERTIES props = {
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        1,
        1
    };

    ID3D12Resource* pSrcResource = nullptr;
    auto hr = device->CreateCommittedResource(
        &props,
        D3D12_HEAP_FLAG_NONE,
        &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&pSrcResource));
    if (FAILED(hr))
    {
        ELOG("Error : ID3D12Device::CreateCommitedResource() Failed. errcode = 0x%x", hr);
        return;
    }

    // コマンドを生成.
    {
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts;
        std::vector<UINT>                               rows;
        std::vector<UINT64>                             rowSizeInBytes;

        layouts       .resize(count);
        rows          .resize(count);
        rowSizeInBytes.resize(count);

        UINT64 requiredSize = 0;
        device->GetCopyableFootprints(
            &dstDesc, 0, count, 0, layouts.data(), rows.data(), rowSizeInBytes.data(), &requiredSize);

        BYTE* pData = nullptr;
        hr = pSrcResource->Map(0, nullptr, reinterpret_cast<void**>(&pData));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Resource::Map() Failed. errcode = 0x%x", hr);
            pSrcResource->Release();
            return;
        }

        for(auto i=0u; i<count; ++i)
        {
            D3D12_SUBRESOURCE_DATA srcData = {};
            srcData.pData       = pResTexture->pResources[i].pPixels;
            srcData.RowPitch    = pResTexture->pResources[i].Pitch;
            srcData.SlicePitch  = pResTexture->pResources[i].SlicePitch;
            assert(layouts[i].Footprint.Width  == pResTexture->pResources[i].Width);
            assert(layouts[i].Footprint.Height == pResTexture->pResources[i].Height);

            D3D12_MEMCPY_DEST dstData = {};
            dstData.pData       = pData + layouts[i].Offset;
            dstData.RowPitch    = layouts[i].Footprint.RowPitch;
            dstData.SlicePitch  = SIZE_T(layouts[i].Footprint.RowPitch) * SIZE_T(rows[i]);

            CopySubresource(
                &dstData,
                &srcData,
                SIZE_T(rowSizeInBytes[i]),
                rows[i],
                layouts[i].Footprint.Depth);
        }
        pSrcResource->Unmap(0, nullptr);

        if (dstDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            pCmdList->CopyBufferRegion(
                pDstResource,
                0,
                pSrcResource,
                layouts[0].Offset,
                layouts[0].Footprint.Width);
        }
        else
        {
            for(auto i=0u; i<count; ++i)
            {
                D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
                dstLoc.pResource        = pDstResource;
                dstLoc.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                dstLoc.SubresourceIndex = i;

                D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
                srcLoc.pResource        = pSrcResource;
                srcLoc.Type             = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                srcLoc.PlacedFootprint  = layouts[i];

                pCmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
            }
        }
    }

    Dispose(pSrcResource);
}


///////////////////////////////////////////////////////////////////////////////
// ConstantBufferView class
///////////////////////////////////////////////////////////////////////////////
class ConstantBufferView : public IConstantBufferView
{
public:
    ConstantBufferView
    (
        ID3D12Resource*                         pResource,
        const D3D12_CONSTANT_BUFFER_VIEW_DESC*  pDesc,
        Descriptor*                             pDescriptor
    )
    : m_RefCount  (1)
    , m_Resource  (pResource)
    , m_Descriptor(pDescriptor)
    {
        m_Resource->AddRef();
        memcpy(&m_Desc, pDesc, sizeof(m_Desc));
    }

    ~ConstantBufferView()
    {
        DisposeDescriptor(m_Descriptor);
        Dispose(m_Resource); 
    }

    void AddRef() override
    { m_RefCount++; }

    void Release() override
    {
        m_RefCount--;
        if (m_RefCount == 0)
        { delete this; }
    }

    uint32_t GetCount() const override
    { return m_RefCount; }

    D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const override
    { return m_Descriptor->GetHandleCPU(); }

    D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const override
    { return m_Descriptor->GetHandleGPU(); }

    uint32_t GetDescriptorIndex() const override
    { return m_Descriptor->GetIndex(); }

    ID3D12Resource* GetResource() const override
    { return m_Resource; }

    D3D12_CONSTANT_BUFFER_VIEW_DESC GetDesc() const override
    { return m_Desc; }

private:
    std::atomic<uint32_t>           m_RefCount;
    ID3D12Resource*                 m_Resource;
    Descriptor*                     m_Descriptor;
    D3D12_CONSTANT_BUFFER_VIEW_DESC m_Desc;
};


///////////////////////////////////////////////////////////////////////////////
// RenderTargetView class
///////////////////////////////////////////////////////////////////////////////
class RenderTargetView : public IRenderTargetView
{
public:
    RenderTargetView
    (
        ID3D12Resource*                         pResource,
        const D3D12_RENDER_TARGET_VIEW_DESC*    pDesc,
        Descriptor*                             pDescriptor
    )
    : m_RefCount    (1)
    , m_Resource    (pResource)
    , m_Descriptor  (pDescriptor)
    {
        m_Resource->AddRef();
        memcpy(&m_Desc, pDesc, sizeof(m_Desc));
    }

    ~RenderTargetView()
    {
        DisposeDescriptor(m_Descriptor);
        Dispose(m_Resource);
    }

    void AddRef() override
    { m_RefCount++; }

    void Release() override
    {
        m_RefCount--;
        if (m_RefCount == 0)
        { delete this; }
    }

    uint32_t GetCount() const override
    { return m_RefCount; }

    D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const override
    { return m_Descriptor->GetHandleCPU(); }

    D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const override
    { return m_Descriptor->GetHandleGPU(); }

    uint32_t GetDescriptorIndex() const override
    { return m_Descriptor->GetIndex(); }

    ID3D12Resource* GetResource() const override
    { return m_Resource; }

    D3D12_RENDER_TARGET_VIEW_DESC GetDesc() const override
    { return m_Desc; }

private:
    std::atomic<uint32_t>           m_RefCount;
    ID3D12Resource*                 m_Resource;
    Descriptor*                     m_Descriptor;
    D3D12_RENDER_TARGET_VIEW_DESC   m_Desc;
};


///////////////////////////////////////////////////////////////////////////////
// DepthStencilView class
///////////////////////////////////////////////////////////////////////////////
class DepthStencilView : public IDepthStencilView
{
public:
    DepthStencilView
    (
        ID3D12Resource*                         pResource,
        const D3D12_DEPTH_STENCIL_VIEW_DESC*    pDesc,
        Descriptor*                             pDescriptor
    )
    : m_RefCount    (1)
    , m_Resource    (pResource)
    , m_Descriptor  (pDescriptor)
    {
        m_Resource->AddRef();
        memcpy(&m_Desc, pDesc, sizeof(m_Desc));
    }

    ~DepthStencilView()
    {
        DisposeDescriptor(m_Descriptor);
        Dispose(m_Resource);
    }

    void AddRef() override
    { m_RefCount++; }

    void Release() override
    {
        m_RefCount--;
        if (m_RefCount == 0)
        { delete this; }
    }

    uint32_t GetCount() const override
    { return m_RefCount; }

    D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const override
    { return m_Descriptor->GetHandleCPU(); }

    D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const override
    { return m_Descriptor->GetHandleGPU(); }

    uint32_t GetDescriptorIndex() const override
    { return m_Descriptor->GetIndex(); }

    ID3D12Resource* GetResource() const override
    { return m_Resource; }

    D3D12_DEPTH_STENCIL_VIEW_DESC GetDesc() const override
    { return m_Desc; }
   
private:
    std::atomic<uint32_t>           m_RefCount;
    ID3D12Resource*                 m_Resource;
    Descriptor*                     m_Descriptor;
    D3D12_DEPTH_STENCIL_VIEW_DESC   m_Desc;
};

///////////////////////////////////////////////////////////////////////////////
// ShaderResourceView class
///////////////////////////////////////////////////////////////////////////////
class ShaderResourceView : public IShaderResourceView
{
public:
    ShaderResourceView
    (
        ID3D12Resource*                         pResource,
        const D3D12_SHADER_RESOURCE_VIEW_DESC*  pDesc,
        Descriptor*                             pDescriptor
    )
    : m_RefCount    (1)
    , m_Resource    (pResource)
    , m_Descriptor  (pDescriptor)
    {
        m_Resource->AddRef();
        memcpy(&m_Desc, pDesc, sizeof(m_Desc));
    }

    ~ShaderResourceView()
    {
        DisposeDescriptor(m_Descriptor);
        Dispose(m_Resource);
    }

    void AddRef() override
    { m_RefCount++; }

    void Release() override
    {
        m_RefCount--;
        if (m_RefCount == 0)
        { delete this; }
    }

    uint32_t GetCount() const override
    { return m_RefCount; }

    D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const override
    { return m_Descriptor->GetHandleCPU(); }

    D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const override
    { return m_Descriptor->GetHandleGPU(); }

    uint32_t GetDescriptorIndex() const override
    { return m_Descriptor->GetIndex(); }

    ID3D12Resource* GetResource() const override
    { return m_Resource; }

    D3D12_SHADER_RESOURCE_VIEW_DESC GetDesc() const override
    { return m_Desc; }

private:
    std::atomic<uint32_t>           m_RefCount;
    ID3D12Resource*                 m_Resource;
    Descriptor*                     m_Descriptor;
    D3D12_SHADER_RESOURCE_VIEW_DESC m_Desc;
};

///////////////////////////////////////////////////////////////////////////////
// UnorderedAccessView class
///////////////////////////////////////////////////////////////////////////////
class UnorderedAccessView : public IUnorderedAccessView
{
public:
    UnorderedAccessView
    (
        ID3D12Resource* pResource,
        ID3D12Resource* pCounterResource,
        const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc,
        Descriptor* pDescriptor
    )
    : m_RefCount        (1)
    , m_Resource        (pResource)
    , m_CounterResource (pCounterResource)
    , m_Descriptor      (pDescriptor)
    {
        m_Resource->AddRef();
        if (m_CounterResource != nullptr)
        { m_CounterResource->AddRef(); }
        memcpy(&m_Desc, pDesc, sizeof(m_Desc));
    }

    ~UnorderedAccessView()
    {
        DisposeDescriptor(m_Descriptor);
        Dispose(m_Resource);
        Dispose(m_CounterResource);
    }

    void AddRef() override
    { m_RefCount++; }

    void Release() override
    {
        m_RefCount--;
        if (m_RefCount == 0)
        { delete this; }
    }

    uint32_t GetCount() const override
    { return m_RefCount; }

    D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const override
    { return m_Descriptor->GetHandleCPU(); }

    D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU() const override
    { return m_Descriptor->GetHandleGPU(); }

    uint32_t GetDescriptorIndex() const override
    { return m_Descriptor->GetIndex(); }

    ID3D12Resource* GetResource() const override
    { return m_Resource; }

    ID3D12Resource* GetCounterResource() const override
    { return m_CounterResource; }

    D3D12_UNORDERED_ACCESS_VIEW_DESC GetDesc() const override
    { return m_Desc; }

private:
    std::atomic<uint32_t>               m_RefCount;
    ID3D12Resource*                     m_Resource;
    ID3D12Resource*                     m_CounterResource;
    Descriptor*                         m_Descriptor;
    D3D12_UNORDERED_ACCESS_VIEW_DESC    m_Desc;
};

//-----------------------------------------------------------------------------
//      定数バッファを生成します.
//-----------------------------------------------------------------------------
bool CreateConstantBufferView
(
    ID3D12Resource*                         pResource,
    const D3D12_CONSTANT_BUFFER_VIEW_DESC*  pDesc,
    IConstantBufferView**                   ppView
)
{
    Descriptor* pDescriptor = nullptr;
    auto ret = AllocDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, &pDescriptor);
    if (!ret)
    { return false; }

    GetD3D12Device()->CreateConstantBufferView(pDesc, pDescriptor->GetHandleCPU());

    auto instance = new ConstantBufferView(pResource, pDesc, pDescriptor);
    *ppView = instance;

    return true;
}

//-----------------------------------------------------------------------------
//      レンダーターゲットビューを生成します.
//-----------------------------------------------------------------------------
bool CreateRenderTargetView
(
    ID3D12Resource*                         pResource,
    const D3D12_RENDER_TARGET_VIEW_DESC*    pDesc,
    IRenderTargetView**                     ppView
)
{
    Descriptor* pDescriptor = nullptr;
    auto ret = AllocDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, &pDescriptor);
    if (!ret)
    { return false; }

    GetD3D12Device()->CreateRenderTargetView(pResource, pDesc, pDescriptor->GetHandleCPU());

    auto instance = new RenderTargetView(pResource, pDesc, pDescriptor);
    *ppView = instance;

    return true;
}

//-----------------------------------------------------------------------------
//      深度ステンシルビューを生成します.
//-----------------------------------------------------------------------------
bool CreateDepthStencilView
(
    ID3D12Resource*                         pResource,
    const D3D12_DEPTH_STENCIL_VIEW_DESC*    pDesc,
    IDepthStencilView**                     ppView
)
{
    Descriptor* pDescriptor = nullptr;
    auto ret = AllocDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, &pDescriptor);
    if (!ret)
    { return false; }

    GetD3D12Device()->CreateDepthStencilView(pResource, pDesc, pDescriptor->GetHandleCPU());

    auto instance = new DepthStencilView(pResource, pDesc, pDescriptor);
    *ppView = instance;

    return true;
}

//-----------------------------------------------------------------------------
//      シェーダリソースビューを生成します.
//-----------------------------------------------------------------------------
bool CreateShaderResourceView
(
    ID3D12Resource*                         pResource,
    const D3D12_SHADER_RESOURCE_VIEW_DESC*  pDesc,
    IShaderResourceView**                   ppView
)
{
    Descriptor* pDescriptor = nullptr;
    auto ret = AllocDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, &pDescriptor);
    if (!ret)
    { return false; }

    GetD3D12Device()->CreateShaderResourceView(pResource, pDesc, pDescriptor->GetHandleCPU());

    auto instance = new ShaderResourceView(pResource, pDesc, pDescriptor);
    *ppView = instance;

    return true;
}

//-----------------------------------------------------------------------------
//      アンオーダードアクセスビューを生成します.
//-----------------------------------------------------------------------------
bool CreateUnorderedAccessView
(
    ID3D12Resource*                         pResource,
    ID3D12Resource*                         pCounterResource,
    const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc,
    IUnorderedAccessView**                  ppView
)
{
    Descriptor* pDescriptor = nullptr;
    auto ret = AllocDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, &pDescriptor);
    if (!ret)
    { return false; }

    GetD3D12Device()->CreateUnorderedAccessView(pResource, pCounterResource, pDesc, pDescriptor->GetHandleCPU());

    auto instance = new UnorderedAccessView(pResource, pCounterResource, pDesc, pDescriptor);
    *ppView = instance;

    return true;
}


//-----------------------------------------------------------------------------
//      フルスクリーン矩形用入力レイアウトを取得します.
//-----------------------------------------------------------------------------
D3D12_INPUT_LAYOUT_DESC GetQuadLayout()
{ return kQuadLayout; }

//-----------------------------------------------------------------------------
//      フルスクリーン矩形用頂点シェーダを取得します.
//-----------------------------------------------------------------------------
D3D12_SHADER_BYTECODE GetQuadVS()
{
    D3D12_SHADER_BYTECODE result = {};
    result.pShaderBytecode = FullScreenVS;
    result.BytecodeLength  = sizeof(FullScreenVS);
    return result;
}

//-----------------------------------------------------------------------------
//      フルスクリーン矩形を描画します.
//-----------------------------------------------------------------------------
void DrawQuad(ID3D12GraphicsCommandList* pCmd)
{
    auto vbv = GraphicsSystem::Instance().GetQuadVB().GetVBV();
    pCmd->IASetVertexBuffers(0, 1, &vbv);
    pCmd->IASetIndexBuffer(nullptr);
    pCmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCmd->DrawInstanced(3, 1, 0, 0);
}

//-----------------------------------------------------------------------------
//      コマンドシグニチャを取得します.
//-----------------------------------------------------------------------------
ID3D12CommandSignature* GetCommandSignature(COMMAND_SIGNATURE_TYPE type)
{ return GraphicsSystem::Instance().GetCommandSignature(type); }

//-----------------------------------------------------------------------------
//      スタティックサンプラーを取得します.
//-----------------------------------------------------------------------------
const D3D12_STATIC_SAMPLER_DESC* GetStaticSamplers()
{ return kStaticSamplers; }

//-----------------------------------------------------------------------------
//      スタティックサンプラー数を取得します.
//-----------------------------------------------------------------------------
uint32_t GetStaticSamplerCounts()
{ return _countof(kStaticSamplers); }

} // namespace asdx
