//-----------------------------------------------------------------------------
// File : SampleApp.cpp
// Desc : Sample Application Module.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "SampleApp.h"
#include "FileUtil.h"
#include "Logger.h"
#include "CommonStates.h"
#include "DirectXHelpers.h"
#include "SimpleMath.h"


//-----------------------------------------------------------------------------
// Using Statements
//-----------------------------------------------------------------------------
using namespace DirectX::SimpleMath;


namespace {

///////////////////////////////////////////////////////////////////////////////
// COLOR_SPACE_TYPE enum
///////////////////////////////////////////////////////////////////////////////
enum COLOR_SPACE_TYPE
{
    COLOR_SPACE_BT709,      // ITU-R BT.709
    COLOR_SPACE_BT2100_PQ,  // ITU-R BT.2100 PQ System.
};

///////////////////////////////////////////////////////////////////////////////
// TONEMAP_TYPE enum
///////////////////////////////////////////////////////////////////////////////
enum TONEMAP_TYPE
{
    TONEMAP_NONE = 0,   // トーンマップなし.
    TONEMAP_REINHARD,   // Reinhardトーンマップ.
    TONEMAP_GT,         // GTトーンマップ.
};

///////////////////////////////////////////////////////////////////////////////
// CbTonemap structure
///////////////////////////////////////////////////////////////////////////////
struct alignas(256) CbTonemap
{
    int     Type;               // トーンマップタイプ.
    int     ColorSpace;         // 出力色空間.
    float   BaseLuminance;      // 基準輝度値[nit].
    float   MaxLuminance;       // 最大輝度値[nit].
};

///////////////////////////////////////////////////////////////////////////////
// CbMesh structure
///////////////////////////////////////////////////////////////////////////////
struct alignas(256) CbMesh
{
    Matrix   World;      //!< ワールド行列です.
};

///////////////////////////////////////////////////////////////////////////////
// CbTransform structure
///////////////////////////////////////////////////////////////////////////////
struct alignas(256) CbTransform
{
    Matrix   View;       //!< ビュー行列です.
    Matrix   Proj;       //!< 射影行列です.
};

///////////////////////////////////////////////////////////////////////////////
// CbLight structure
///////////////////////////////////////////////////////////////////////////////
struct alignas(256) CbLight
{
    Vector3     LightPosition;          //!< ライト位置.
    float       LightInvSqrRadius;      //!< 半径2乗の逆数.
    
    Vector3     LightColor;             //!< ライトカラー.
    float       LightIntensity;         //!< ライト強度.
    
    Vector3     LightForward;           //!< ライトの照射方向.
    float       LightAngleScale;        //!< 1.0f / (cosInner - cosOuter).

    float       LightAngleOffset;       //!< -cosOuter * LightAngleScale.
    int         LightType;              //!< ライトタイプ.
    float       Padding[2];             //!< パディング.
};

///////////////////////////////////////////////////////////////////////////////
// CbCamera structure
///////////////////////////////////////////////////////////////////////////////
struct alignas(256) CbCamera
{
    Vector3  CameraPosition;    //!< カメラ位置です.
};

///////////////////////////////////////////////////////////////////////////////
// CbMaterial structure
///////////////////////////////////////////////////////////////////////////////
struct alignas(256) CbMaterial
{
    Vector3 BaseColor;  //!< 基本色.
    float   Alpha;      //!< 透過度.
    float   Roughness;  //!< 面の粗さです(範囲は[0,1]).
    float   Metallic;   //!< 金属度です(範囲は[0,1]).
};

//-----------------------------------------------------------------------------
//      色度を取得する.
//-----------------------------------------------------------------------------
inline UINT16 GetChromaticityCoord(double value)
{
    return UINT16(value * 50000);
}

//-----------------------------------------------------------------------------
//      スポットライトパラメータを計算します.
//-----------------------------------------------------------------------------
CbLight ComputeSpotLight
(
    int             lightType,
    const Vector3&  dir,
    const Vector3&  pos,
    float           radius,
    const Vector3&  color,
    float           intensity,
    float           innerAngle,
    float           outerAngle
)
{
    auto cosInnerAngle = cosf(innerAngle);
    auto cosOuterAngle = cosf(outerAngle);

    CbLight result;
    result.LightPosition     = pos;
    result.LightInvSqrRadius = 1.0f / (radius * radius);
    result.LightColor        = color;
    result.LightIntensity    = intensity;
    result.LightForward      = dir;
    result.LightAngleScale   = 1.0f / DirectX::XMMax(0.001f, (cosInnerAngle - cosOuterAngle));
    result.LightAngleOffset  = -cosOuterAngle * result.LightAngleScale;
    result.LightType         = lightType;
    return result;
}

//-----------------------------------------------------------------------------
//      ライトカラーを時間で変化させます.
//-----------------------------------------------------------------------------
Vector3 CalcLightColor(float time)
{
    auto c = fmodf(time, 3.0f);
    auto result = Vector3(0.25f, 0.25f, 0.25f);

    if (c < 1.0f)
    {
        result.x += 1.0f - c;
        result.y += c;
    }
    else if (c < 2.0f)
    {
        c -= 1.0f;
        result.y += 1.0f - c;
        result.z += c;
    }
    else
    {
        c -= 2.0f;
        result.z += 1.0f - c;
        result.x += c;
    }

    return result;
}

} // namespace


///////////////////////////////////////////////////////////////////////////////
// SampleApp class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
SampleApp::SampleApp(uint32_t width, uint32_t height)
: App(width, height, DXGI_FORMAT_R10G10B10A2_UNORM)
, m_TonemapType     (TONEMAP_NONE)
, m_ColorSpace      (COLOR_SPACE_BT709)
, m_BaseLuminance   (100.0f)
, m_MaxLuminance    (100.0f)
, m_Exposure        (1.0f)
, m_LightType       (0)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
SampleApp::~SampleApp()
{ /* DO_NOTHING */ }

//-------------------------------------------------------------------------------------------------
//      初期化時の処理です.
//-----------------------------------------------------------------------------
bool SampleApp::OnInit()
{
    // メッシュをロード.
    {
        std::wstring path;

        // ファイルパスを検索.
        if (!SearchFilePath(L"res/material_test/material_test.obj", path))
        {
            ELOG("Error : File Not Found.");
            return false;
        }

        std::wstring dir = GetDirectoryPath(path.c_str());

        std::vector<ResMesh>        resMesh;
        std::vector<ResMaterial>    resMaterial;

        // メッシュリソースをロード.
        if (!LoadMesh(path.c_str(), resMesh, resMaterial))
        {
            ELOG("Error : Load Mesh Failed. filepath = %ls", path.c_str());
            return false;
        }

        // メモリを予約.
        m_pMesh.reserve(resMesh.size());

        // メッシュを初期化.
        for (size_t i = 0; i < resMesh.size(); ++i)
        {
            // メッシュ生成.
            auto mesh = new (std::nothrow) Mesh();

            // チェック.
            if (mesh == nullptr)
            {
                ELOG("Error : Out of memory.");
                return false;
            }

            // 初期化処理.
            if (!mesh->Init(m_pDevice.Get(), resMesh[i]))
            {
                ELOG("Error : Mesh Initialize Failed.");
                delete mesh;
                return false;
            }

            // 成功したら登録.
            m_pMesh.push_back(mesh);
        }

        // メモリ最適化.
        m_pMesh.shrink_to_fit();

        // マテリアル初期化.
        if (!m_Material.Init(
            m_pDevice.Get(),
            m_pPool[POOL_TYPE_RES],
            sizeof(CbMaterial),
            resMaterial.size()))
        {
            ELOG("Error : Material::Init() Failed.");
            return false;
        }

        // リソースバッチを用意.
        DirectX::ResourceUploadBatch batch(m_pDevice.Get());

        // バッチ開始.
        batch.Begin();

        // テクスチャとマテリアルを設定.
        {
            /* ここではマテリアルが決め打ちであることを前提にハードコーディングしています. */
            m_Material.SetTexture(0, TU_BASE_COLOR, dir + L"wall_bc.dds", batch);
            m_Material.SetTexture(0, TU_METALLIC,   dir + L"wall_m.dds",  batch);
            m_Material.SetTexture(0, TU_ROUGHNESS,  dir + L"wall_r.dds",  batch);
            m_Material.SetTexture(0, TU_NORMAL,     dir + L"wall_n.dds",  batch);

            m_Material.SetTexture(1, TU_BASE_COLOR, dir + L"matball_bc.dds", batch);
            m_Material.SetTexture(1, TU_METALLIC,   dir + L"matball_m.dds",  batch);
            m_Material.SetTexture(1, TU_ROUGHNESS,  dir + L"matball_r.dds",  batch);
            m_Material.SetTexture(1, TU_NORMAL,     dir + L"matball_n.dds",  batch);
        }

        // バッチ終了.
        auto future = batch.End(m_pQueue.Get());

        // バッチ完了を待機.
        future.wait();
    }

    // ライトバッファの生成.
    {
        for (auto i=0; i<FrameCount; ++i)
        {
            if (!m_LightCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbLight)))
            {
                ELOG("Error : ConstantBuffer::Init() Failed.");
                return false;
            }
        }
    }

    // カメラバッファの設定.
    {
        for (auto i=0; i<FrameCount; ++i)
        {
            if (!m_CameraCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbCamera)))
            {
                ELOG("Error : ConstantBuffer::Init() Failed.");
                return false;
            }
        }
    }

    // シーン用カラーターゲットの生成.
    {
        float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };

        if (!m_SceneColorTarget.Init(
            m_pDevice.Get(),
            m_pPool[POOL_TYPE_RTV],
            m_pPool[POOL_TYPE_RES],
            m_Width,
            m_Height,
            DXGI_FORMAT_R10G10B10A2_UNORM,
            clearColor))
        {
            ELOG("Error : ColorTarget::Init() Failed.");
            return false;
        }
    }

    // シーン用深度ターゲットの生成.
    {
        if (!m_SceneDepthTarget.Init(
            m_pDevice.Get(),
            m_pPool[POOL_TYPE_DSV],
            nullptr,
            m_Width,
            m_Height,
            DXGI_FORMAT_D32_FLOAT,
            1.0f,
            0))
        {
            ELOG("Error : DepthTarget::Init() Failed.");
            return false;
        }
    }

    // シーン用ルートシグニチャの生成.
    {
        RootSignature::Desc desc;
        desc.Begin(8)
            .SetCBV(ShaderStage::VS, 0, 0)
            .SetCBV(ShaderStage::VS, 1, 1)
            .SetCBV(ShaderStage::PS, 2, 1)
            .SetCBV(ShaderStage::PS, 3, 2)
            .SetSRV(ShaderStage::PS, 4, 0)
            .SetSRV(ShaderStage::PS, 5, 1)
            .SetSRV(ShaderStage::PS, 6, 2)
            .SetSRV(ShaderStage::PS, 7, 3)
            .AllowIL()
            .AddStaticSmp(ShaderStage::PS, 0, SamplerState::LinearWrap)
            .AddStaticSmp(ShaderStage::PS, 1, SamplerState::LinearWrap)
            .AddStaticSmp(ShaderStage::PS, 2, SamplerState::LinearWrap)
            .AddStaticSmp(ShaderStage::PS, 3, SamplerState::LinearWrap)
            .End();

        if (!m_SceneRootSig.Init(m_pDevice.Get(), desc.GetDesc()))
        {
            ELOG("Error : RootSignature::Init() Failed.");
            return false;
        }
    }

    // シーン用パイプラインステートの生成.
    {
        std::wstring vsPath;
        std::wstring psPath;

        // 頂点シェーダを検索.
        if (!SearchFilePath(L"BasicVS.cso", vsPath))
        {
            ELOG("Error : Vertex Shader Not Found.");
            return false;
        }

        // ピクセルシェーダを検索.
        if (!SearchFilePath(L"BasicPS.cso", psPath))
        {
            ELOG("Error : Pixel Shader Node Found.");
            return false;
        }

        ComPtr<ID3DBlob> pVSBlob;
        ComPtr<ID3DBlob> pPSBlob;

        // 頂点シェーダを読み込む.
        auto hr = D3DReadFileToBlob(vsPath.c_str(), pVSBlob.GetAddressOf());
        if (FAILED(hr))
        {
            ELOG("Error : D3DReadFiledToBlob() Failed. path = %ls", vsPath.c_str());
            return false;
        }

        // ピクセルシェーダを読み込む.
        hr = D3DReadFileToBlob(psPath.c_str(), pPSBlob.GetAddressOf());
        if (FAILED(hr))
        {
            ELOG("Error : D3DReadFileToBlob() Failed. path = %ls", psPath.c_str());
            return false;
        }

        D3D12_INPUT_ELEMENT_DESC elements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // グラフィックスパイプラインステートを設定.
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.InputLayout            = { elements, 4 };
        desc.pRootSignature         = m_SceneRootSig.GetPtr();
        desc.VS                     = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
        desc.PS                     = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
        desc.RasterizerState        = DirectX::CommonStates::CullNone;
        desc.BlendState             = DirectX::CommonStates::Opaque;
        desc.DepthStencilState      = DirectX::CommonStates::DepthDefault;
        desc.SampleMask             = UINT_MAX;
        desc.PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets       = 1;
        desc.RTVFormats[0]          = m_SceneColorTarget.GetRTVDesc().Format;
        desc.DSVFormat              = m_SceneDepthTarget.GetDSVDesc().Format;
        desc.SampleDesc.Count       = 1;
        desc.SampleDesc.Quality     = 0;

        // パイプラインステートを生成.
        hr = m_pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_pScenePSO.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. retcode = 0x%x", hr);
            return false;
        }
    }

    // トーンマップ用ルートシグニチャの生成.
    {
        RootSignature::Desc desc;
        desc.Begin(2)
            .SetCBV(ShaderStage::PS, 0, 0)
            .SetSRV(ShaderStage::PS, 1, 0)
            .AllowIL()
            .AddStaticSmp(ShaderStage::PS, 0, SamplerState::LinearWrap)
            .End();

        if (!m_TonemapRootSig.Init(m_pDevice.Get(), desc.GetDesc()))
        {
            ELOG("Error : RootSignature::Init() Failed.");
            return false;
        }
    }

    // トーンマップ用パイプラインステートの生成.
    {
        std::wstring vsPath;
        std::wstring psPath;

        // 頂点シェーダを検索.
        if (!SearchFilePath(L"TonemapVS.cso", vsPath))
        {
            ELOG( "Error : Vertex Shader Not Found.");
            return false;
        }

        // ピクセルシェーダを検索.
        if (!SearchFilePath(L"TonemapPS.cso", psPath))
        {
            ELOG( "Error : Pixel Shader Node Found.");
            return false;
        }

        ComPtr<ID3DBlob> pVSBlob;
        ComPtr<ID3DBlob> pPSBlob;

        // 頂点シェーダを読み込む.
        auto hr = D3DReadFileToBlob( vsPath.c_str(), pVSBlob.GetAddressOf() );
        if ( FAILED(hr) )
        {
            ELOG( "Error : D3DReadFiledToBlob() Failed. path = %ls", vsPath.c_str() );
            return false;
        }

        // ピクセルシェーダを読み込む.
        hr = D3DReadFileToBlob( psPath.c_str(), pPSBlob.GetAddressOf() );
        if ( FAILED(hr) )
        {
            ELOG( "Error : D3DReadFileToBlob() Failed. path = %ls", psPath.c_str() );
            return false;
        }

        D3D12_INPUT_ELEMENT_DESC elements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        // グラフィックスパイプラインステートを設定.
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.InputLayout            = { elements, 2 };
        desc.pRootSignature         = m_TonemapRootSig.GetPtr();
        desc.VS                     = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
        desc.PS                     = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
        desc.RasterizerState        = DirectX::CommonStates::CullNone;
        desc.BlendState             = DirectX::CommonStates::Opaque;
        desc.DepthStencilState      = DirectX::CommonStates::DepthDefault;
        desc.SampleMask             = UINT_MAX;
        desc.PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets       = 1;
        desc.RTVFormats[0]          = m_ColorTarget[0].GetRTVDesc().Format;
        desc.DSVFormat              = m_DepthTarget.GetDSVDesc().Format;
        desc.SampleDesc.Count       = 1;
        desc.SampleDesc.Quality     = 0;

        // パイプラインステートを生成.
        hr = m_pDevice->CreateGraphicsPipelineState( &desc, IID_PPV_ARGS(m_pTonemapPSO.GetAddressOf()) );
        if ( FAILED(hr) )
        {
            ELOG( "Error : ID3D12Device::CreateGraphicsPipelineState() Failed. retcode = 0x%x", hr );
            return false;
        }
    }

    // 頂点バッファの生成.
    {
        struct Vertex
        {
            float px;
            float py;

            float tx;
            float ty;
        };

        if (!m_QuadVB.Init<Vertex>(m_pDevice.Get(), 3))
        {
            ELOG( "Error : VertexBuffer::Init() Failed.");
            return false;
        }

        auto ptr = m_QuadVB.Map<Vertex>();
        assert(ptr != nullptr);
        ptr[0].px = -1.0f;  ptr[0].py =  1.0f;  ptr[0].tx = 0.0f;   ptr[0].ty = -1.0f;
        ptr[1].px =  3.0f;  ptr[1].py =  1.0f;  ptr[1].tx = 2.0f;   ptr[1].ty = -1.0f;
        ptr[2].px = -1.0f;  ptr[2].py = -3.0f;  ptr[2].tx = 0.0f;   ptr[2].ty = 1.0f;
        m_QuadVB.Unmap();
    }

    for(auto i=0; i<FrameCount; ++i)
    {
        if (!m_TonemapCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbTonemap)))
        {
            ELOG("Error : ConstantBuffer::Init() Failed.");
            return false;
        }
    }

    // 変換行列用の定数バッファの生成.
    {
        for (auto i = 0u; i<FrameCount; ++i)
        {
            // 定数バッファ初期化.
            if (!m_TransformCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbTransform)))
            {
                ELOG("Error : ConstantBuffer::Init() Failed.");
                return false;
            }

            // カメラ設定.
            auto eyePos     = Vector3(0.0f, 0.0f, 1.0f);
            auto targetPos  = Vector3::Zero;
            auto upward     = Vector3::UnitY;

            // 垂直画角とアスペクト比の設定.
            auto fovY = DirectX::XMConvertToRadians(37.5f);
            auto aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);

            // 変換行列を設定.
            auto ptr = m_TransformCB[i].GetPtr<CbTransform>();
            ptr->View   = Matrix::CreateLookAt(eyePos, targetPos, upward);
            ptr->Proj   = Matrix::CreatePerspectiveFieldOfView(fovY, aspect, 1.0f, 1000.0f);
        }
    }

    // メッシュ用バッファの生成.
    {
        for (auto i = 0; i < FrameCount; ++i)
        {
            if (!m_MeshCB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbMesh)))
            {
                ELOG("Error : ConstantBuffer::Init() Failed.");
                return false;
            }

            auto ptr = m_MeshCB[i].GetPtr<CbMesh>();
            ptr->World = Matrix::Identity;
        }
    }

#if 0
    // テクスチャロード.
    {
        DirectX::ResourceUploadBatch batch(m_pDevice.Get());

        // バッチ開始.
        batch.Begin();

        //-------------------------------------------
        //テクスチャ読み込みが必要な処理を以下に記述.
        //-------------------------------------------


        // バッチ終了.
        auto future = batch.End(m_pQueue.Get());

        // 完了を待機.
        future.wait();
    }
#endif

    // 開始時間を記録.
    m_StartTime = std::chrono::system_clock::now();

    return true;
}

//-----------------------------------------------------------------------------
//      終了時の処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnTerm()
{
    m_QuadVB.Term();
    for(auto i=0; i<FrameCount; ++i)
    {
        m_TonemapCB  [i].Term();
        m_LightCB    [i].Term();
        m_CameraCB   [i].Term();
        m_TransformCB[i].Term();
    }

    // メッシュ破棄.
    for (size_t i = 0; i<m_pMesh.size(); ++i)
    {
        SafeTerm(m_pMesh[i]);
    }
    m_pMesh.clear();
    m_pMesh.shrink_to_fit();

    // マテリアル破棄.
    m_Material.Term();

    for(auto i=0; i<FrameCount; ++i)
    {
        m_MeshCB[i].Term();
    }

    m_SceneColorTarget.Term();
    m_SceneDepthTarget.Term();

    m_pScenePSO   .Reset();
    m_SceneRootSig.Term();

    m_pTonemapPSO   .Reset();
    m_TonemapRootSig.Term();
}

//-----------------------------------------------------------------------------
//      描画時の処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnRender()
{
    // コマンドリストの記録を開始.
    auto pCmd = m_CommandList.Reset();

    ID3D12DescriptorHeap* const pHeaps[] = {
        m_pPool[POOL_TYPE_RES]->GetHeap(),
    };

    pCmd->SetDescriptorHeaps(1, pHeaps);

    {
        // ディスクリプタ取得.
        auto handleRTV = m_SceneColorTarget.GetHandleRTV();
        auto handleDSV = m_SceneDepthTarget.GetHandleDSV();

        // 書き込み用リソースバリア設定.
        DirectX::TransitionResource(pCmd,
            m_SceneColorTarget.GetResource(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET);

        // レンダーターゲットを設定.
        pCmd->OMSetRenderTargets(1, &handleRTV->HandleCPU, FALSE, &handleDSV->HandleCPU);

        // レンダーターゲットをクリア.
        m_SceneColorTarget.ClearView(pCmd);
        m_SceneDepthTarget.ClearView(pCmd);

        // シーンの描画.
        DrawScene(pCmd);

        // 読み込み用リソースバリア設定.
        DirectX::TransitionResource(pCmd,
            m_SceneColorTarget.GetResource(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    // フレームバッファに描画.
    {
        // 書き込み用リソースバリア設定.
        DirectX::TransitionResource(pCmd,
            m_ColorTarget[m_FrameIndex].GetResource(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);

        // ディスクリプタ取得.
        auto handleRTV = m_ColorTarget[m_FrameIndex].GetHandleRTV();
        auto handleDSV = m_DepthTarget.GetHandleDSV();

        // レンダーターゲットを設定.
        pCmd->OMSetRenderTargets(1, &handleRTV->HandleCPU, FALSE, &handleDSV->HandleCPU);

        // レンダーターゲットをクリア.
        m_ColorTarget[m_FrameIndex].ClearView(pCmd);
        m_DepthTarget.ClearView(pCmd);

        // トーンマップを適用.
        DrawTonemap(pCmd);

        // 表示用リソースバリア設定.
        DirectX::TransitionResource(pCmd,
            m_ColorTarget[m_FrameIndex].GetResource(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
    }

    // コマンドリストの記録を終了.
    pCmd->Close();

    // コマンドリストを実行.
    ID3D12CommandList* pLists[] = { pCmd };
    m_pQueue->ExecuteCommandLists( 1, pLists );

    // 画面に表示.
    Present(1);
}

//-----------------------------------------------------------------------------
//      シーンを描画します.
//-----------------------------------------------------------------------------
void SampleApp::DrawScene(ID3D12GraphicsCommandList* pCmd)
{
    auto cameraPos = Vector3(1.0f, 0.5f, 3.0f);

    auto currTime = std::chrono::system_clock::now();
    auto dt = float(std::chrono::duration_cast<std::chrono::milliseconds>(currTime - m_StartTime).count()) / 1000.0f;
    auto lightColor = CalcLightColor(dt * 0.25f);

    // ライトバッファの更新.
    {
        auto pos    = Vector3(-1.5f, 0.0f, 1.5f);
        auto dir    = Vector3(1.0f, -0.1f, -1.0f);
        dir.Normalize();

        auto ptr = m_LightCB[m_FrameIndex].GetPtr<CbLight>();
        *ptr = ComputeSpotLight(
            m_LightType,
            dir,
            pos,
            3.0f,
            lightColor,
            810.0f,
            DirectX::XMConvertToRadians(5.0f),
            DirectX::XMConvertToRadians(20.0f));
    }

    // カメラバッファの更新.
    {
        auto ptr = m_CameraCB[m_FrameIndex].GetPtr<CbCamera>();
        ptr->CameraPosition = cameraPos;
    }

    // メッシュのワールド行列の更新
    {
        auto ptr = m_MeshCB[m_FrameIndex].GetPtr<CbMesh>();
        ptr->World = Matrix::Identity;
    }

    // 変換パラメータの更新
    {
        auto fovY = DirectX::XMConvertToRadians(37.5f);
        auto aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);

        auto ptr = m_TransformCB[m_FrameIndex].GetPtr<CbTransform>();
        ptr->View  = Matrix::CreateLookAt(cameraPos, Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
        ptr->Proj  = Matrix::CreatePerspectiveFieldOfView(fovY, aspect, 1.0f, 1000.0f);
    }

    pCmd->SetGraphicsRootSignature(m_SceneRootSig.GetPtr());
    pCmd->SetGraphicsRootDescriptorTable(0, m_TransformCB[m_FrameIndex].GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(2, m_LightCB    [m_FrameIndex].GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(3, m_CameraCB   [m_FrameIndex].GetHandleGPU());
    pCmd->SetPipelineState(m_pScenePSO.Get());
    pCmd->RSSetViewports(1, &m_Viewport);
    pCmd->RSSetScissorRects(1, &m_Scissor);

    // オブジェクトを描画.
    {
        pCmd->SetGraphicsRootDescriptorTable(1, m_MeshCB[m_FrameIndex].GetHandleGPU());
        DrawMesh(pCmd);
    }
}

//-----------------------------------------------------------------------------
//      メッシュを描画します.
//-----------------------------------------------------------------------------
void SampleApp::DrawMesh(ID3D12GraphicsCommandList* pCmd)
{
    for (size_t i = 0; i<m_pMesh.size(); ++i)
    {
        // マテリアルIDを取得.
        auto id = m_pMesh[i]->GetMaterialId();

        // テクスチャを設定.
        pCmd->SetGraphicsRootDescriptorTable(4, m_Material.GetTextureHandle(id, TU_BASE_COLOR));
        pCmd->SetGraphicsRootDescriptorTable(5, m_Material.GetTextureHandle(id, TU_METALLIC));
        pCmd->SetGraphicsRootDescriptorTable(6, m_Material.GetTextureHandle(id, TU_ROUGHNESS));
        pCmd->SetGraphicsRootDescriptorTable(7, m_Material.GetTextureHandle(id, TU_NORMAL));

        // メッシュを描画.
        m_pMesh[i]->Draw(pCmd);
    }
}

//-----------------------------------------------------------------------------
//      トーンマップを適用します.
//-----------------------------------------------------------------------------
void SampleApp::DrawTonemap(ID3D12GraphicsCommandList* pCmd)
{
    // 定数バッファ更新
    {
        auto ptr = m_TonemapCB[m_FrameIndex].GetPtr<CbTonemap>();
        ptr->Type           = m_TonemapType;
        ptr->ColorSpace     = m_ColorSpace;
        ptr->BaseLuminance  = m_BaseLuminance;
        ptr->MaxLuminance   = m_MaxLuminance;
    }

    pCmd->SetGraphicsRootSignature(m_TonemapRootSig.GetPtr());
    pCmd->SetGraphicsRootDescriptorTable(0, m_TonemapCB[m_FrameIndex].GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(1, m_SceneColorTarget.GetHandleSRV()->HandleGPU);

    pCmd->SetPipelineState(m_pTonemapPSO.Get());
    pCmd->RSSetViewports(1, &m_Viewport);
    pCmd->RSSetScissorRects(1, &m_Scissor);

    auto VBV = m_QuadVB.GetView();
    pCmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCmd->IASetVertexBuffers(0, 1, &VBV);
    pCmd->DrawInstanced(3, 1, 0, 0);
}

//-----------------------------------------------------------------------------
//      ディスプレイモードを変更します.
//-----------------------------------------------------------------------------
void SampleApp::ChangeDisplayMode(bool hdr)
{
    if (hdr)
    {
        if (!IsSupportHDR())
        {
            MessageBox(
                nullptr,
                TEXT("HDRがサポートされていないディスプレイです."),
                TEXT("HDR非サポート"),
                MB_OK | MB_ICONINFORMATION);
            ELOG("Error : Display not support HDR.");
            return;
        }

        auto hr = m_pSwapChain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
        if (FAILED(hr))
        {
            MessageBox(
                nullptr,
                TEXT("ITU-R BT.2100 PQ Systemの色域設定に失敗しました"),
                TEXT("色域設定失敗"),
                MB_OK | MB_ICONERROR);
            ELOG("Error : IDXGISwapChain::SetColorSpace1() Failed.");
            return;
        }

        DXGI_HDR_METADATA_HDR10 metaData = {};

        // ITU-R BT.2100の原刺激と白色点を設定.
        metaData.RedPrimary[0]      = GetChromaticityCoord(0.708);
        metaData.RedPrimary[1]      = GetChromaticityCoord(0.292);
        metaData.BluePrimary[0]     = GetChromaticityCoord(0.170);
        metaData.BluePrimary[1]     = GetChromaticityCoord(0.797);
        metaData.GreenPrimary[0]    = GetChromaticityCoord(0.131);
        metaData.GreenPrimary[1]    = GetChromaticityCoord(0.046);
        metaData.WhitePoint[0]      = GetChromaticityCoord(0.3127);
        metaData.WhitePoint[1]      = GetChromaticityCoord(0.3290);

        // ディスプレイがサポートすると最大輝度値と最小輝度値を設定.
        metaData.MaxMasteringLuminance = UINT(GetMaxLuminance() * 10000);
        metaData.MinMasteringLuminance = UINT(GetMinLuminance() * 0.001);

        // 最大値を 2000 [nit]に設定.
        metaData.MaxContentLightLevel = 2000;

        hr = m_pSwapChain->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(DXGI_HDR_METADATA_HDR10), &metaData);
        if (FAILED(hr))
        {
            ELOG("Error : IDXGISwapChain::SetHDRMetaData() Failed.");
        }

        m_BaseLuminance = 100.0f;
        m_MaxLuminance  = GetMaxLuminance();

        // 成功したことを知らせるダイアログを出す.
        std::string message;
        message += "HDRディスプレイ用に設定を変更しました\n\n";
        message += "色空間：ITU-R BT.2100 PQ\n";
        message += "最大輝度値：";
        message += std::to_string(GetMaxLuminance());
        message += " [nit]\n";
        message += "最小輝度値：";
        message += std::to_string(GetMinLuminance());
        message += " [nit]\n";

        MessageBoxA(nullptr,
            message.c_str(),
            "HDR設定成功",
            MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        auto hr = m_pSwapChain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
        if (FAILED(hr))
        {
            MessageBox(
                nullptr,
                TEXT("ITU-R BT.709の色域設定に失敗しました"),
                TEXT("色域設定失敗"),
                MB_OK | MB_ICONERROR);
            ELOG("Error : IDXGISwapChain::SetColorSpace1() Failed.");
            return;
        }

        DXGI_HDR_METADATA_HDR10 metaData = {};

        // ITU-R BT.709の原刺激と白色点を設定.
        metaData.RedPrimary[0]      = GetChromaticityCoord(0.640);
        metaData.RedPrimary[1]      = GetChromaticityCoord(0.330);
        metaData.BluePrimary[0]     = GetChromaticityCoord(0.300);
        metaData.BluePrimary[1]     = GetChromaticityCoord(0.600);
        metaData.GreenPrimary[0]    = GetChromaticityCoord(0.150);
        metaData.GreenPrimary[1]    = GetChromaticityCoord(0.060);
        metaData.WhitePoint[0]      = GetChromaticityCoord(0.3127);
        metaData.WhitePoint[1]      = GetChromaticityCoord(0.3290);

        // ディスプレイがサポートすると最大輝度値と最小輝度値を設定.
        metaData.MaxMasteringLuminance = UINT(GetMaxLuminance() * 10000);
        metaData.MinMasteringLuminance = UINT(GetMinLuminance() * 0.001);

        // 最大値を 100[nit] に設定.
        metaData.MaxContentLightLevel = 100;

        hr = m_pSwapChain->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(DXGI_HDR_METADATA_HDR10), &metaData);
        if (FAILED(hr))
        {
            ELOG("Error : IDXGISwapChain::SetHDRMetaData() Failed.");
        }

        m_BaseLuminance = 100.0f;
        m_MaxLuminance  = 100.0f;

        // 成功したことを知らせるダイアログを出す.
        std::string message;
        message += "SDRディスプレイ用に設定を変更しました\n\n";
        message += "色空間：ITU-R BT.709\n";
        message += "最大輝度値：";
        message += std::to_string(GetMaxLuminance());
        message += " [nit]\n";
        message += "最小輝度値：";
        message += std::to_string(GetMinLuminance());
        message += " [nit]\n";
        MessageBoxA(nullptr,
            message.c_str(),
            "SDR設定成功",
            MB_OK | MB_ICONINFORMATION);
    }
}

//-----------------------------------------------------------------------------
//      メッセージプロシージャです.
//-----------------------------------------------------------------------------
void SampleApp::OnMsgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    // キーボードの処理.
    if ( ( msg == WM_KEYDOWN )
      || ( msg == WM_SYSKEYDOWN )
      || ( msg == WM_KEYUP )
      || ( msg == WM_SYSKEYUP ) )
    {
        DWORD mask = ( 1 << 29 );

        auto isKeyDown = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
        auto isAltDown = ((lp & mask) != 0 );
        auto keyCode   = uint32_t(wp);

        if (isKeyDown)
        {
            switch (keyCode)
            {
            case VK_ESCAPE:
                {
                    PostQuitMessage(0);
                }
                break;

            // HDRモード.
            case 'H':
                {
                    ChangeDisplayMode(true);
                }
                break;

            // SDRモード.
            case 'S':
                {
                    ChangeDisplayMode(false);
                }
                break;

            // トーンマップなし.
            case 'N':
                {
                    m_TonemapType = TONEMAP_NONE;
                }
                break;

            // Reinhardトーンマップ.
            case 'R':
                {
                    m_TonemapType = TONEMAP_REINHARD;
                }
                break;

            // GTトーンマップ
            case 'G':
                {
                    m_TonemapType = TONEMAP_GT;
                }
                break;

            // スポットライト計算手法の切り替え
            case 'L':
                {
                    m_LightType = (m_LightType + 1) % 3;

                    // 距離減衰なし.
                    if (m_LightType == 0)
                    {
                        printf_s("SpotLight : Default\n");
                    }
                    // [Karis 2013] Brian Karis, "Real Shading in Unreal Engine4", SIGGRAPH 2013 Course: Physically Based Shading in Theory and Parctice.
                    else if (m_LightType == 1)
                    {
                        printf_s("SpotLight : [Karis 2013]\n");
                    }
                    // [Lagarde, Rousiers 2014] Sébastien Lagarde, Charles de Reousiers, "Moving Frostbite to Physically Based Rendering 2.0", SIGGRAPH 2014 Course: Physically Based Shading in Theory and Practice.
                    else if (m_LightType == 2)
                    {
                        printf_s("SpotLight : [Lagarde, Rousiers 2014]\n"); 
                    }
                }
                break;
            }
        }
    }
}
