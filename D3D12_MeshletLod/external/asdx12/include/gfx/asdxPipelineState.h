//-----------------------------------------------------------------------------
// File : asdxPipelineState.h
// Desc : Pipeline State.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <vector>
#include <map>
#include <atomic>
#include <d3d12.h>
#include <fnd/asdxRef.h>
#include <gfx/asdxView.h>
#include <edit/asdxFileWatcher.h>


#if defined(DEBUG) || defined(_DEBUG)
#define D3DCOMPILE_DEBUG 1      // デバッグ情報がシェーダー BLOB に出力されるようする.
#endif//defined(DEBUG) || defined(_DEBUG)


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// SHADER_TYPE
///////////////////////////////////////////////////////////////////////////////
enum SHADER_TYPE
{
    SHADER_TYPE_VS,      // Vertex Shader.
    SHADER_TYPE_PS,      // Pixel Shader.
    SHADER_TYPE_AS,      // Amplification Shader.
    SHADER_TYPE_MS,      // Mesh Shader.
    SHADER_TYPE_CS       // Compute Shader.
};

///////////////////////////////////////////////////////////////////////////////
// PIPELINE_TYPE
///////////////////////////////////////////////////////////////////////////////
enum PIPELINE_TYPE
{
    PIPELINE_TYPE_GRAPHICS,     //!< Legacy Graphics Pipeline.
    PIPELINE_TYPE_COMPUTE,      //!< Compute Pipeline.
    PIPELINE_TYPE_GEOMETRY,     //!< Mesh Shader Pipeline.
};

///////////////////////////////////////////////////////////////////////////////
// Preset class
///////////////////////////////////////////////////////////////////////////////
class Preset
{
public:
    static const D3D12_RASTERIZER_DESC CullNone;    //!< カリング無し.
    static const D3D12_RASTERIZER_DESC CullBack;    //!< 背面カリング. 
    static const D3D12_RASTERIZER_DESC CullFront;   //!< 前面カリング.
    static const D3D12_RASTERIZER_DESC Wireframe;   //!< ワイヤーフレーム.

    static const D3D12_DEPTH_STENCILOP_DESC StencilDefault; //!< ステンシルデフォルト.

    static const D3D12_DEPTH_STENCIL_DESC DepthDefault;     //!< 深度テストON ・書き込み有り.
    static const D3D12_DEPTH_STENCIL_DESC DepthNone;        //!< 深度テストOFF・書き込み無し.
    static const D3D12_DEPTH_STENCIL_DESC DepthReadOnly;    //!< 深度テストON ・書き込み無し.
    static const D3D12_DEPTH_STENCIL_DESC DepthWriteOnly;   //!< 深度テストOFF・書き込み有り.

    static const D3D12_RENDER_TARGET_BLEND_DESC RTB_Opaque;         //!< 不透明.
    static const D3D12_RENDER_TARGET_BLEND_DESC RTB_AlphaBlend;     //!< アルファブレンド.
    static const D3D12_RENDER_TARGET_BLEND_DESC RTB_Additive;       //!< 加算.
    static const D3D12_RENDER_TARGET_BLEND_DESC RTB_Subtract;       //!< 減算.
    static const D3D12_RENDER_TARGET_BLEND_DESC RTB_Premultiplied;  //!< 事前乗算済みアルファ.
    static const D3D12_RENDER_TARGET_BLEND_DESC RTB_Multiply;       //!< 乗算.
    static const D3D12_RENDER_TARGET_BLEND_DESC RTB_Screen;         //!< スクリーン.

    static const D3D12_BLEND_DESC Opaque;           //!< 不透明.
    static const D3D12_BLEND_DESC AlphaBlend;       //!< アルファブレンド.
    static const D3D12_BLEND_DESC Additive;         //!< 加算.
    static const D3D12_BLEND_DESC Subtract;         //!< 減算.
    static const D3D12_BLEND_DESC Premultiplied;    //!< 事前乗算済みアルファ.
    static const D3D12_BLEND_DESC Multiply;         //!< 乗算.
    static const D3D12_BLEND_DESC Screen;           //!< スクリーン.

    static const D3D12_SHADER_BYTECODE FullScreenVS;    //!< フルスクリーン用頂点シェーダ.
    static const D3D12_SHADER_BYTECODE CopyPS;          //!< コピー用ピクセルシェーダ.
    static const D3D12_SHADER_BYTECODE SpriteVS;        //!< スプライト用頂点シェーダ.
    static const D3D12_SHADER_BYTECODE SpritePS;        //!< スプライト用ピクセルシェーダ.
};

///////////////////////////////////////////////////////////////////////////////
// GEOMETRY_PIPELINE_STATE_DESC structure
///////////////////////////////////////////////////////////////////////////////
struct GEOMETRY_PIPELINE_STATE_DESC
{
    ID3D12RootSignature*        pRootSignature;         //!< ルートシグニチャ.
    D3D12_SHADER_BYTECODE       AS;                     //!< 増幅シェーダ.
    D3D12_SHADER_BYTECODE       MS;                     //!< メッシュシェーダ.
    D3D12_SHADER_BYTECODE       PS;                     //!< ピクセルシェーダ.
    D3D12_BLEND_DESC            BlendState;             //!< ブレンドステート.
    UINT                        SampleMask;             //!< サンプルマスク.
    D3D12_RASTERIZER_DESC       RasterizerState;        //!< ラスタライザーステート.
    D3D12_DEPTH_STENCIL_DESC    DepthStencilState;      //!< 深度ステンシルステート.
    D3D12_RT_FORMAT_ARRAY       RTVFormats;             //!< レンダーターゲットフォーマット.
    DXGI_FORMAT                 DSVFormat;              //!< 深度ステンシルビューフォーマット.
    DXGI_SAMPLE_DESC            SampleDesc;             //!< サンプル設定.
    UINT                        NodeMask;               //!< ノードマスク.
    D3D12_CACHED_PIPELINE_STATE CachedPSO;              //!< キャッシュ済みPSO.
    D3D12_PIPELINE_STATE_FLAGS  Flags;                  //!< フラグ.
};

///////////////////////////////////////////////////////////////////////////////
// PipelineState class
///////////////////////////////////////////////////////////////////////////////
class PipelineState : public IFileUpdateListener
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
    //-------------------------------------------------------------------------
    PipelineState();

    //-------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //-------------------------------------------------------------------------
    ~PipelineState();

    //-------------------------------------------------------------------------
    //! @brief      グラフィックスパイプラインとして初期化します.
    //!
    //! @param[in]      pDevice     デバイスです.
    //! @param[in]      pDesc       構成設定です.
    //! @retval true    初期化に成功.
    //! @retval false   初期化に失敗.
    //-------------------------------------------------------------------------
    bool Init(ID3D12Device* pDevice, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc);

    //-------------------------------------------------------------------------
    //! @brief      コンピュートパイプラインとして初期化します.
    //!
    //! @param[in]      pDevice     デバイスです.
    //! @param[in]      pDesc       構成設定です.
    //! @retval true    初期化に成功.
    //! @retval false   初期化に失敗.
    //-------------------------------------------------------------------------
    bool Init(ID3D12Device* pDevice, const D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc);

    //-------------------------------------------------------------------------
    //! @brief      ジオメトリパイプラインとして初期化します.
    //!
    //! @param[in]      pDevice     デバイスです.
    //! @param[in]      pDesc       構成設定です.
    //! @retval true    初期化に成功.
    //! @retval false   初期化に失敗.
    //-------------------------------------------------------------------------
    bool Init(ID3D12Device2* pDevice, const GEOMETRY_PIPELINE_STATE_DESC* pDesc);

    //-------------------------------------------------------------------------
    //! @brief      終了処理を行います.
    //-------------------------------------------------------------------------
    void Term();

    //-------------------------------------------------------------------------
    //! @brief      パイプラインステートを設定します.
    //! 
    //! @param[in]      pCmdList        コマンドリスト.
    //-------------------------------------------------------------------------
    void SetState(ID3D12GraphicsCommandList* pCmdList);

    //-------------------------------------------------------------------------
    //! @brief      パイプラインタイプを取得します.
    //! 
    //! @return     パイプラインタイプを返却します.
    //-------------------------------------------------------------------------
    PIPELINE_TYPE GetType() const;

    //-------------------------------------------------------------------------
    //! @brief      頂点シェーダのリロードパスを設定します.
    //! 
    //! @param[in]      path        監視対象となるファイルパス.
    //-------------------------------------------------------------------------
    void SetReloadPathVS(const char* path, const char* shaderModel);

    //-------------------------------------------------------------------------
    //! @brief      頂点シェーダのリロードパスを設定します.
    //! 
    //! @param[in]      path        監視対象となるファイルパス.
    //-------------------------------------------------------------------------
    void SetReloadPathPS(const char* path, const char* shaderModel);

    //-------------------------------------------------------------------------
    //! @brief      頂点シェーダのリロードパスを設定します.
    //! 
    //! @param[in]      path        監視対象となるファイルパス.
    //-------------------------------------------------------------------------
    void SetReloadPathCS(const char* path, const char* shaderModel);

    //-------------------------------------------------------------------------
    //! @brief      頂点シェーダのリロードパスを設定します.
    //! 
    //! @param[in]      path        監視対象となるファイルパス.
    //-------------------------------------------------------------------------
    void SetReloadPathAS(const char* path, const char* shaderModel);

    //-------------------------------------------------------------------------
    //! @brief      頂点シェーダのリロードパスを設定します.
    //! 
    //! @param[in]      path        監視対象となるファイルパス.
    //-------------------------------------------------------------------------
    void SetReloadPathMS(const char* path, const char* shaderModel);

    //-------------------------------------------------------------------------
    //! @brief      シェーダリロードの為のインクルードディレクトリを設定します.
    //! 
    //! @param[in]      dirs        インクルードディレクトリ.
    //-------------------------------------------------------------------------
    void SetIncludeDirs(const std::vector<std::string>& dirs);

    //-------------------------------------------------------------------------
    //! @brief      依存ファイル名を設定します.
    //! 
    //! @param[in]      path        ファイルパス.
    //-------------------------------------------------------------------------
    void SetDependencies(const std::vector<std::string>& dependencies);

    //-------------------------------------------------------------------------
    //! @brief      ファイル更新時の処理です.
    //-------------------------------------------------------------------------
    void OnUpdate(const FileUpdateEventArgs& args) override;

    //-------------------------------------------------------------------------
    //! @brief      リビルドを要求します.
    //-------------------------------------------------------------------------
    void RequestRebuild() { m_Dirty = true; }

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    union Desc
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC  Graphics;
        D3D12_COMPUTE_PIPELINE_STATE_DESC   Compute;
        GEOMETRY_PIPELINE_STATE_DESC        Geometry;
    } m_Desc = {};

    PIPELINE_TYPE               m_Type;
    RefPtr<ID3D12PipelineState> m_DefaultPSO;
    RefPtr<ID3D12PipelineState> m_ReloadedPSO;

    std::string                 m_ReloadPathVS;
    std::string                 m_ReloadPathPS;
    std::string                 m_ReloadPathCS;
    std::string                 m_ReloadPathAS;
    std::string                 m_ReloadPathMS;

    std::string                 m_ShaderModelVS;
    std::string                 m_ShaderModelPS;
    std::string                 m_ShaderModelCS;
    std::string                 m_ShaderModelAS;
    std::string                 m_ShaderModelMS;

    std::vector<std::string>    m_Dependencies;

    std::vector<uint8_t>        m_VS;
    std::vector<uint8_t>        m_PS;
    std::vector<uint8_t>        m_CS;
    std::vector<uint8_t>        m_MS;
    std::vector<uint8_t>        m_AS;
    std::vector<std::string>    m_IncludeDirs;

    std::atomic<bool> m_Dirty = false;

    void Rebuild();
    bool ReloadShader(const char* path, const char* shaderModel, std::vector<uint8_t>& result);
};

void InitRangeAsSRV(D3D12_DESCRIPTOR_RANGE& range, UINT registerIndex, UINT count = 1, UINT registerSpace = 0);
void InitRangeAsUAV(D3D12_DESCRIPTOR_RANGE& range, UINT registerIndex, UINT count = 1, UINT registerSpace = 0);
void InitAsConstants(D3D12_ROOT_PARAMETER& param, UINT registerIndex, UINT count, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace = 0);
void InitAsCBV(D3D12_ROOT_PARAMETER& param, UINT registerIndex, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace = 0);
void InitAsSRV(D3D12_ROOT_PARAMETER& param, UINT registerIndex, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace = 0);
void InitAsUAV(D3D12_ROOT_PARAMETER& param, UINT registerIndex, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace = 0);
void InitAsTable(D3D12_ROOT_PARAMETER& param, UINT count, const D3D12_DESCRIPTOR_RANGE* range, D3D12_SHADER_VISIBILITY visiblity);
bool InitRootSignature(ID3D12Device* pDevice, const D3D12_ROOT_SIGNATURE_DESC* pDesc, ID3D12RootSignature** ppRootSig);

} // namespace asdx
