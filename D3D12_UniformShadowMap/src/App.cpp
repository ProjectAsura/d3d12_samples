//-----------------------------------------------------------------------------
// File : App.cpp
// Desc : Sample Application.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "App.h"
#include <fnd/asdxLogger.h>
#include <fnd/asdxFileIO.h>
#include <fnd/asdxMisc.h>
#include <gfx/asdxDevice.h>
#include <gfx/asdxPresetState.h>
#include <edit/asdxGuiMgr.h>
#include <imgui.h>

#include <pix3.h>
#include <gfx/asdxScopedMarker.h>


namespace {

//-----------------------------------------------------------------------------
// Shaders
//-----------------------------------------------------------------------------
#include "../res/shaders/Compiled/MeshVS.inc"
#include "../res/shaders/Compiled/MeshPS.inc"

#include "../res/shaders/Compiled/ShadowVS.inc"
//#include "../res/shaders/Compiled/ShadowPS.inc"


///////////////////////////////////////////////////////////////////////////////
// ROOT_PARAM enum
///////////////////////////////////////////////////////////////////////////////
enum ROOT_PARAM
{
    ROOT_PARAM_B0,
    ROOT_PARAM_B1,
    ROOT_PARAM_B2,
    ROOT_PARAM_B3,
    ROOT_PARAM_T0,
    ROOT_PARAM_T1,
    ROOT_PARAM_T2,
    ROOT_PARAM_T3,
    ROOT_PARAM_T4,
    ROOT_PARAM_T5,
    ROOT_PARAM_T6,
    ROOT_PARAM_T7,
    MAX_ROOT_PARAM_COUNT,
};

///////////////////////////////////////////////////////////////////////////////
// SceneParam structure
///////////////////////////////////////////////////////////////////////////////
struct alignas(256) SceneParam
{
    asdx::Matrix    View;
    asdx::Matrix    Proj;
    asdx::Vector3   CameraPos;
    float           FieldOfView;
    float           NearClip;
    float           FarClip;
    float           TargetWidth;
    float           TargetHeight;
};

///////////////////////////////////////////////////////////////////////////////
// ModelParam structure
///////////////////////////////////////////////////////////////////////////////
struct alignas(256) ModelParam
{
    asdx::Matrix    World;
};

///////////////////////////////////////////////////////////////////////////////
// DirLightParam structure
///////////////////////////////////////////////////////////////////////////////
struct alignas(256) DirLightParam
{
    asdx::Vector3   LightDir;       // 照射方向.
    asdx::Vector3   LightColor;     // ライトカラー.
    float           LightIntensity;
};

///////////////////////////////////////////////////////////////////////////////
// ShadowSceneParam structure
///////////////////////////////////////////////////////////////////////////////
struct alignas(256) ShadowSceneParam
{
    asdx::Matrix    View;
    asdx::Matrix    Proj;
};

// 通常描画用入力レイアウト.
static const D3D12_INPUT_ELEMENT_DESC InputElements[] = {
    { "POSITION"   , 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL"     , 0, DXGI_FORMAT_R32G32B32_FLOAT   , 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TANGENT"    , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD"   , 0, DXGI_FORMAT_R32G32_FLOAT      , 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "COLOR"      , 0, DXGI_FORMAT_R8G8B8A8_UNORM    , 4, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

// シャドウマップ描画用入力レイアウト.
static const D3D12_INPUT_ELEMENT_DESC ShadowInputElements[] = {
    { "POSITION"   , 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD"   , 0, DXGI_FORMAT_R32G32_FLOAT      , 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};


} // namespace

///////////////////////////////////////////////////////////////////////////////
// SampleApp class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
SampleApp::SampleApp()
: asdx::App(L"Sample", 1920, 1080, nullptr, nullptr, nullptr)
{
    m_SwapChainFormat    = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_DepthStencilFormat = DXGI_FORMAT_D32_FLOAT;

    m_DeviceDesc.MaxShaderResourceCount = 8192;
    m_DeviceDesc.MaxSamplerCount        = 128;
    m_DeviceDesc.MaxColorTargetCount    = 256;
    m_DeviceDesc.MaxDepthTargetCount    = 256;

#if ASDX_DEBUG
    m_DeviceDesc.EnableDebug   = true;
    m_DeviceDesc.EnableCapture = true;
#endif
}

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
SampleApp::~SampleApp()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      初期化時の処理です.
//-----------------------------------------------------------------------------
bool SampleApp::OnInit()
{
    auto pDevice = asdx::GetD3D12Device();

    // コマンドリストをリセット.
    m_GfxCmdList.Reset();
    auto pCmd = m_GfxCmdList.GetD3D12CommandList();

    #if ASDX_ENABLE_IMGUI
    // GUI初期化.
    {
        if (!asdx::GuiMgr::Instance().Init(pCmd, m_hWnd, m_Width, m_Height, m_SwapChainFormat))
        {
            ELOGA("Error : GuiMgr::Init() Failed.");
            return false;
        }
    }
    #endif

    // テクスチャマネージャ初期化.
    if (!asdx::TextureManager::Instance().Init())
    {
        ELOG("Error : TextureManager::Init() Failed.");
        return false;
    }

    // モデルマネージャ初期化.
    if (!asdx::ModelManager::Instance().Init())
    {
        ELOG("Error : ModelManager::Init() Failed.");
        return false;
    }

    // モデルの読み込み.
    {
        const char* path = "../res/models/rt_camp_2025/rt_camp_2025.mdb";
        m_Model = asdx::ModelManager::Instance().GetOrCreate(path);
        if (!m_Model.IsValid())
        {
            ELOGA("Error : Model::Create() Failed.");
            return false;
        }
    }

    // 線分レンダラー初期化.
    if (!m_LineRenderer.Init(UINT16_MAX, m_SwapChainFormat, m_DepthStencilFormat))
    {
        ELOGA("Error : LineRenderer::Init() Failed.");
        return false;
    }

    // スプライトレンダラー初期化.
    if (!m_SpriteRenderer.Init(m_Width, m_Height, 512, 512, m_SwapChainFormat, m_DepthStencilFormat))
    {
        ELOGA("Error : SpriteRenderer::Init() Failed.");
        return false;
    }

    // ルートシグニチャ初期化.
    {
        D3D12_DESCRIPTOR_RANGE ranges[8] = {};
        asdx::InitRangeAsSRV(ranges[0], 0);
        asdx::InitRangeAsSRV(ranges[1], 1);
        asdx::InitRangeAsSRV(ranges[2], 2);
        asdx::InitRangeAsSRV(ranges[3], 3);
        asdx::InitRangeAsSRV(ranges[4], 4);
        asdx::InitRangeAsSRV(ranges[5], 5);
        asdx::InitRangeAsSRV(ranges[6], 6);
        asdx::InitRangeAsSRV(ranges[7], 7);

        D3D12_ROOT_PARAMETER params[MAX_ROOT_PARAM_COUNT] = {};
        asdx::InitAsCBV(params[ROOT_PARAM_B0], 0, D3D12_SHADER_VISIBILITY_ALL);
        asdx::InitAsCBV(params[ROOT_PARAM_B1], 1, D3D12_SHADER_VISIBILITY_ALL);
        asdx::InitAsCBV(params[ROOT_PARAM_B2], 2, D3D12_SHADER_VISIBILITY_ALL);
        asdx::InitAsCBV(params[ROOT_PARAM_B3], 3, D3D12_SHADER_VISIBILITY_ALL);
        asdx::InitAsTable(params[ROOT_PARAM_T0], 1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
        asdx::InitAsTable(params[ROOT_PARAM_T1], 1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
        asdx::InitAsTable(params[ROOT_PARAM_T2], 1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);
        asdx::InitAsTable(params[ROOT_PARAM_T3], 1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);
        asdx::InitAsTable(params[ROOT_PARAM_T4], 1, &ranges[4], D3D12_SHADER_VISIBILITY_PIXEL);
        asdx::InitAsTable(params[ROOT_PARAM_T5], 1, &ranges[5], D3D12_SHADER_VISIBILITY_PIXEL);
        asdx::InitAsTable(params[ROOT_PARAM_T6], 1, &ranges[6], D3D12_SHADER_VISIBILITY_PIXEL);
        asdx::InitAsTable(params[ROOT_PARAM_T7], 1, &ranges[7], D3D12_SHADER_VISIBILITY_PIXEL);

        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters      = _countof(params);
        desc.pParameters        = params;
        desc.NumStaticSamplers  = _countof(asdx::Preset::StaticSamplers);
        desc.pStaticSamplers    = asdx::Preset::StaticSamplers;
        desc.Flags              = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        if (!asdx::InitRootSignature(pDevice, &desc, m_RootSig.GetAddress()))
        {
            ELOGA("Error : InitRootSignature() Failed.");
            return false;
        }
    }

    // 不透明ステートの初期化.
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature                 = m_RootSig.GetPtr();
        desc.VS                             = { MeshVS, sizeof(MeshVS) };
        desc.PS                             = { MeshPS, sizeof(MeshPS) };
        desc.BlendState                     = asdx::Preset::Opaque;
        desc.SampleMask                     = D3D12_DEFAULT_SAMPLE_MASK;
        desc.RasterizerState                = asdx::Preset::CullBack;
        desc.DepthStencilState              = asdx::Preset::DepthReadWrite;
        desc.InputLayout.NumElements        = _countof(InputElements);
        desc.InputLayout.pInputElementDescs = InputElements;
        desc.PrimitiveTopologyType          = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets               = 1;
        desc.RTVFormats[0]                  = m_SwapChainFormat;
        desc.DSVFormat                      = m_DepthStencilFormat;
        desc.SampleDesc.Count               = 1;
        desc.SampleDesc.Quality             = 0;

        if (!m_OpaqueState.Init(&desc))
        {
            ELOGA("Error : PipelineStateManager::Create() Failed.");
            return false;
        }
    }

    // 半透明用ステートの初期化.
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature                 = m_RootSig.GetPtr();
        desc.VS                             = { MeshVS, sizeof(MeshVS) };
        desc.PS                             = { MeshPS, sizeof(MeshPS) };
        desc.BlendState                     = asdx::Preset::AlphaBlend;
        desc.SampleMask                     = D3D12_DEFAULT_SAMPLE_MASK;
        desc.RasterizerState                = asdx::Preset::CullBack;
        desc.DepthStencilState              = asdx::Preset::DepthReadOnly;
        desc.InputLayout.NumElements        = _countof(InputElements);
        desc.InputLayout.pInputElementDescs = InputElements;
        desc.PrimitiveTopologyType          = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets               = 1;
        desc.RTVFormats[0]                  = m_SwapChainFormat;
        desc.DSVFormat                      = m_DepthStencilFormat;
        desc.SampleDesc.Count               = 1;
        desc.SampleDesc.Quality             = 0;

        if (!m_AlphaBlendState.Init(&desc))
        {
            ELOGA("Error : PipelineStateManager::Create() Failed.");
            return false;
        }
    }

    // シャドウ描画用ステートの初期化.
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature                 = m_RootSig.GetPtr();
        desc.VS                             = { ShadowVS, sizeof(ShadowVS) };
        //desc.PS                             = { ShadowPS, sizeof(ShadowPS) };
        desc.BlendState                     = asdx::Preset::Opaque;
        desc.SampleMask                     = D3D12_DEFAULT_SAMPLE_MASK;
        desc.RasterizerState                = asdx::Preset::CullFront;
        desc.DepthStencilState              = asdx::Preset::DepthReadWrite;
        desc.InputLayout.NumElements        = _countof(ShadowInputElements);
        desc.InputLayout.pInputElementDescs = ShadowInputElements;
        desc.PrimitiveTopologyType          = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets               = 1;
        desc.RTVFormats[0]                  = m_SwapChainFormat;
        desc.DSVFormat                      = m_DepthStencilFormat;
        desc.SampleDesc.Count               = 1;
        desc.SampleDesc.Quality             = 0;

        if (!m_ShadowState.Init(&desc))
        {
            ELOGA("Error : PipelineStateManager::Create() Failed.");
            return false;
        }
    }

    // 定数バッファ初期化.
    {
        if (!m_SceneBuffer.Init(sizeof(SceneParam)))
        {
            ELOGA("Error : Buffer::Init() Failed.");
            return false;
        }

        if (!m_DirLightBuffer.Init(sizeof(DirLightParam)))
        {
            ELOGA("Error : Buffer::Init() Failed.");
            return false;
        }

        if (!m_ShadowSceneBuffer.Init(sizeof(ShadowSceneParam)))
        {
            ELOGA("Error : Buffer::Init() Failed.");
            return false;
        }

        if (!m_ModelParamBuffer.Init(sizeof(ModelParam)))
        {
            ELOGA("Error : Buffer::Init() Failed.");
            return false;
        }
    }

    // モデルバッファ更新.
    {
        auto ptr = m_ModelParamBuffer.MapAs<ModelParam>();
        ptr->World = asdx::Matrix::CreateIdentity();
        m_ModelParamBuffer.Unmap( );
    }

    // テクスチャ初期化.
    {
        m_DFGMap = asdx::TextureManager::Instance().GetOrCreate("../external/asdx12/res/textures/env_brdf.txb");
        if (!m_DFGMap.IsValid())
        {
            ELOGA("Error : EnvBRDF Init Failed.");
            return false;
        }

        m_DiffuseLDMap = asdx::TextureManager::Instance().GetOrCreate("../res/textures/treasure_island.d.txb");
        if (!m_DiffuseLDMap.IsValid())
        {
            ELOGA("Error : DiffuseLD Init Failed.");
            return false;
        }

        m_SpecularLDMap = asdx::TextureManager::Instance().GetOrCreate("../res/textures/treasure_island.s.txb");
        if (!m_SpecularLDMap.IsValid())
        {
            ELOGA("Error : SpecularLD Init Failed.");
            return false;
        }
    }

    // シャドウマップ初期化.
    {
        asdx::TargetDesc desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width              = 1024;
        desc.Height             = 1024;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.Format             = DXGI_FORMAT_D32_FLOAT;
        desc.InitState          = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        desc.ClearDepth         = 1.0f;

        if (!m_ShadowMap.Init(&desc))
        {
            ELOGA("Error : DepthTarget::Init() Failed.");
            return false;
        }
    }

    // サンプラー初期化.
    {
        if (!m_LinerClamp.Init(&asdx::Sampler::LinearClamp))
        {
            ELOG("Error : Sampler::Init() Failed.");
            return false;
        }
    }

    // コマンドの記録を終了.
    pCmd->Close();

    // セットアップコマンド実行.
    {
        ID3D12CommandList* pCmds[] = { pCmd };

        // グラフィックスキューを取得.
        auto pGraphicsQueue = asdx::GetGraphicsQueue();

        // コマンドを実行.
        pGraphicsQueue->Execute(_countof(pCmds), pCmds);

        // 待機点を発行.
        m_FrameWaitPoint = pGraphicsQueue->Signal();

        // 完了を待機.
        pGraphicsQueue->Sync(m_FrameWaitPoint);
    }

    m_Camera.Init(
        asdx::Vector3(0.0f, 1.0f, -5.0f),   // カメラ位置.
        asdx::Vector3(0.0f, 1.0f, 0.0f),    // カメラ注視点.
        asdx::Vector3(0.0f, 1.0f, 0.0f),    // カメラ上方向.
        0.1f,                               // ニアクリップ距離.
        1000.0f                             // ファークリップ距離.
    );
    m_Camera.Present();

    return true;
}

//-----------------------------------------------------------------------------
//      終了時の処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnTerm()
{
    #if ASDX_ENABLE_IMGUI
    {
        // GUI終了処理.
        asdx::GuiMgr::Instance().Term();
    }
    #endif

    m_SpriteRenderer.Term();
    m_LineRenderer.Term();

    // シャドウマップ初期化.
    m_ShadowMap.Term();

    // 定数バッファ解放.
    m_SceneBuffer      .Term();
    m_DirLightBuffer   .Term();
    m_ShadowSceneBuffer.Term();
    m_ModelParamBuffer .Term();

    // サンプラー解放.
    m_LinerClamp.Term();

    // テクスチャ解放.
    m_DFGMap       .Reset();
    m_SpecularLDMap.Reset();
    m_DiffuseLDMap .Reset();

    // ステート解放.
    m_AlphaBlendState.Term();
    m_OpaqueState    .Term();
    m_ShadowState    .Term();

    // ルートシグニチャ解放.
    m_RootSig.Reset();

    // モデル解放.
    m_Model.Reset();

    // モデルマネージャ終了処理.
    asdx::ModelManager::Instance().Term();

    // テクスチャマネージャ終了処理.
    asdx::TextureManager::Instance().Term();
}

//-----------------------------------------------------------------------------
//      フレーム遷移処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnFrameMove(const asdx::App::FrameEventArgs& args)
{
    #if ASDX_ENABLE_IMGUI
    {
        // ImGuiフレーム開始処理.
        asdx::GuiMgr::Instance().Update(m_Width, m_Height);

        if (ImGui::Begin(asdx::ToChar(u8"デバッグ設定")))
        {
            ImGui::Checkbox(asdx::ToChar(u8"シャドウ描画"), &m_EnableShadow);
            ImGui::Checkbox(asdx::ToChar(u8"シャドウ錐台表示"), &m_ShowShadowFrustum);
            ImGui::Checkbox(asdx::ToChar(u8"シャドウマップ表示"), &m_ShowShadowMap);
            ImGui::Checkbox(asdx::ToChar(u8"シーンボックス表示"), &m_ShowSceneBox);
            ImGui::Checkbox(asdx::ToChar(u8"シーンスフィア表示"), &m_ShowSceneSphere);

            ImGui::End();
        }
    }
    #endif

    // 線分レンダラーリセット処理.
    m_LineRenderer.Reset();

    // スプライトレンダラーリセット処理.
    m_SpriteRenderer.Reset();
    m_SpriteRenderer.SetScreenSize(m_Width, m_Height);

    // シーン定数バッファ更新.
    {
        auto fov            = asdx::ToRadian(37.5f);
        auto aspectRatio    = float(m_Width) / float(m_Height);
        auto nearClip       = m_Camera.GetNearClip();
        auto farClip        = m_Camera.GetFarClip();

        SceneParam param = {};
        param.View          = m_Camera.GetView();
        param.Proj          = asdx::Matrix::CreatePerspectiveFieldOfView(fov, aspectRatio, nearClip, farClip);
        param.CameraPos     = m_Camera.GetPosition();
        param.FieldOfView   = fov;
        param.NearClip      = nearClip;
        param.FarClip       = farClip;
        param.TargetWidth   = float(m_Width);
        param.TargetHeight  = float(m_Height);

        m_SceneBuffer.Update(&param, sizeof(param), 0);
    }

    // ライトバッファ更新.
    {
        auto theta = asdx::ToRadian(m_DirLightAngle.y);
        auto phi   = asdx::ToRadian(m_DirLightAngle.x);

        m_DirLightForward.x = cos(theta) * cos(phi);
        m_DirLightForward.y = sin(theta);
        m_DirLightForward.z = cos(theta) * sin(phi);

        DirLightParam param = {};

        param.LightColor     = asdx::Vector3(1.0f, 1.0f, 1.0f);
        param.LightDir       = m_DirLightForward;
        param.LightIntensity = 1.0f;

        m_DirLightBuffer.Update(&param, sizeof(param));
    }

    // シャドウシーンバッファ更新.
    {
        asdx::Vector3 right, upward;
        asdx::CalcONB(m_DirLightForward, right, upward);

        const auto& sphere = m_Model.GetSphere();
        const auto& box    = m_Model.GetBox();
        auto corners = box.GetCorners();

        if (m_ShowSceneBox)
            asdx::DrawWireBox(m_LineRenderer, box.Mini, box.Maxi, asdx::Vector4(0.0f, 1.0f, 0.0f, 1.0f));

        if (m_ShowSceneSphere)
            asdx::DrawWireSphere(m_LineRenderer, sphere.Center, sphere.Radius, asdx::Vector4(0.0f, 0.0f, 1.0f, 1.0f));

        auto nearClip  = 1.0f;
        auto cameraPos = sphere.Center + m_DirLightForward * (nearClip - sphere.Radius);

        // ライトからのビュー行列を作成.
        m_ShadowView = asdx::Matrix::CreateLookTo(cameraPos, m_DirLightForward, upward);

        // ライト空間に変換.
        for(auto i=0; i<corners.size(); ++i)
            corners[i] = asdx::Vector3::TransformCoord(corners[i], m_ShadowView);

        // ライト空間でのAABBを求める.
        auto lightBox = asdx::BoundingBox3::Create(&corners[0].x, corners.size(), sizeof(asdx::Vector3));
        auto w = abs(lightBox.Maxi.x - lightBox.Mini.x);
        auto h = abs(lightBox.Maxi.y - lightBox.Mini.y);
        auto d = abs(lightBox.Maxi.z - lightBox.Mini.z);

        // 最適なカメラ位置とファークリップ距離を求める.
        cameraPos = sphere.Center + m_DirLightForward * (nearClip - d * 0.5f);
        auto farClip = nearClip + d;

        // ライト空間のビュー行列と射影行列を求める.
        m_ShadowView = asdx::Matrix::CreateLookTo(cameraPos, m_DirLightForward, upward);
        m_ShadowProj = asdx::Matrix::CreateOrthographic(w, h, nearClip, farClip);

        // 定数バッファ更新.
        ShadowSceneParam param = {};
        param.View  = m_ShadowView;
        param.Proj  = m_ShadowProj;
        m_ShadowSceneBuffer.Update(&param, sizeof(param));

        // シャドウ錐台表示.
        if (m_ShowShadowFrustum)
        {
            auto viewProj    = m_ShadowView * m_ShadowProj;
            auto invViewProj = asdx::Matrix::Invert(viewProj);
            asdx::DrawWireFrustum(m_LineRenderer, invViewProj, asdx::Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        }

        // シャドウマップ表示.
        if (m_ShowShadowMap)
        {
            auto drawSize = 400;
            m_SpriteRenderer.SetTexture(m_ShadowMap.GetGpuHandleSRV(), m_LinerClamp.GetGpuHandle());
            m_SpriteRenderer.Add(0, m_Height - drawSize, drawSize, drawSize);
        }
    }
}

//-----------------------------------------------------------------------------
//      フレーム描画処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnFrameRender(const asdx::App::FrameEventArgs& args)
{
    auto idx = GetCurrentBackBufferIndex();

    // コマンド記録開始.
    m_GfxCmdList.Reset();
    auto pCmd = m_GfxCmdList.GetD3D12CommandList();

    // シャドウマップに描画.
    if (m_EnableShadow)
    {
        asdx::ScopedMarker marker(pCmd, "Shadow Map Pass");

        m_ShadowMap.ChangeState(pCmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        auto desc = m_ShadowMap.GetDesc();
        D3D12_VIEWPORT viewport = {};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width    = float(desc.Width);
        viewport.Height   = float(desc.Height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        D3D12_RECT scissor = {};
        scissor.left   = 0;
        scissor.right  = LONG(desc.Width);
        scissor.top    = 0;
        scissor.bottom = LONG(desc.Height);

        auto handleDSV = m_ShadowMap.GetCpuHandleDSV();
        pCmd->OMSetRenderTargets(0, nullptr, FALSE, &handleDSV);
        pCmd->ClearDepthStencilView(handleDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        pCmd->RSSetViewports(1, &viewport);
        pCmd->RSSetScissorRects(1, &scissor);

        pCmd->SetGraphicsRootSignature(m_RootSig.GetPtr());
        m_ShadowState.SetState(pCmd);
        pCmd->SetGraphicsRootConstantBufferView(ROOT_PARAM_B0, m_ShadowSceneBuffer.GetGpuAddress());
        pCmd->SetGraphicsRootConstantBufferView(ROOT_PARAM_B1, m_ModelParamBuffer.GetGpuAddress());
        pCmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        if (m_Model.IsValid())
        {
            for(auto i=0u; i<m_Model.GetMeshCount(); ++i)
            {
                const auto pMesh = m_Model.GetMesh(i);
                if (!pMesh->IsVisible())
                    continue;

                D3D12_VERTEX_BUFFER_VIEW VBVs[] = {
                    pMesh->GetPositions().GetVBV(),
                    pMesh->GetTexCoords().GetVBV(),
                };

                const auto IBV = pMesh->GetIndices().GetIBV();

                pCmd->IASetVertexBuffers(0, _countof(VBVs), VBVs);
                pCmd->IASetIndexBuffer(&IBV);
                pCmd->DrawIndexedInstanced(pMesh->GetIndexCount(), 1, 0, 0, 0);
            }
        }
    }

    // カラーバッファに描画.
    {
        asdx::ScopedMarker marker(pCmd, "Main Pass");

        m_ShadowMap.ChangeState(pCmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_ColorTarget[idx].ChangeState(pCmd, D3D12_RESOURCE_STATE_RENDER_TARGET);

        auto handleRTV = m_ColorTarget[idx].GetCpuHandleRTV();
        auto handleDSV = m_DepthTarget.GetCpuHandleDSV();
        pCmd->OMSetRenderTargets(1, &handleRTV, FALSE, &handleDSV);
        pCmd->ClearRenderTargetView(handleRTV, m_ClearColor, 0, nullptr);
        pCmd->ClearDepthStencilView(handleDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        pCmd->RSSetViewports(1, &m_Viewport);
        pCmd->RSSetScissorRects(1, &m_ScissorRect);

        pCmd->SetGraphicsRootSignature(m_RootSig.GetPtr());
        pCmd->SetGraphicsRootConstantBufferView(ROOT_PARAM_B0, m_SceneBuffer.GetGpuAddress());
        pCmd->SetGraphicsRootConstantBufferView(ROOT_PARAM_B1, m_ModelParamBuffer.GetGpuAddress());
        pCmd->SetGraphicsRootConstantBufferView(ROOT_PARAM_B3, m_DirLightBuffer.GetGpuAddress());
        pCmd->SetGraphicsRootDescriptorTable(ROOT_PARAM_T4, m_DFGMap       .GetHandleGPU());
        pCmd->SetGraphicsRootDescriptorTable(ROOT_PARAM_T5, m_DiffuseLDMap .GetHandleGPU());
        pCmd->SetGraphicsRootDescriptorTable(ROOT_PARAM_T6, m_SpecularLDMap.GetHandleGPU());
        pCmd->SetGraphicsRootDescriptorTable(ROOT_PARAM_T7, m_ShadowMap    .GetGpuHandleSRV());

        pCmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        if (m_Model.IsValid())
        {
            for(auto i=0u; i<m_Model.GetMeshCount(); ++i)
            {
                const auto pMesh = m_Model.GetMesh(i);
                if (!pMesh->IsVisible())
                    continue;

                const auto pMaterial = m_Model.GetMaterial(pMesh->GetMaterialId());
                const auto alphaMode = pMaterial->GetAlphaMode();

                if (alphaMode == asdx::AlphaMode::Blend)
                    m_AlphaBlendState.SetState(pCmd);
                else
                    m_OpaqueState.SetState(pCmd);

                D3D12_VERTEX_BUFFER_VIEW VBVs[] = {
                    pMesh->GetPositions().GetVBV(),
                    pMesh->GetNormals  ().GetVBV(),
                    pMesh->GetTangents ().GetVBV(),
                    pMesh->GetTexCoords().GetVBV(),
                    pMesh->GetColors   ().GetVBV(),
                };

                const auto IBV = pMesh->GetIndices().GetIBV();

                pCmd->SetGraphicsRootConstantBufferView(ROOT_PARAM_B2, pMaterial->GetBuffer().GetGpuAddress());
                pCmd->SetGraphicsRootDescriptorTable   (ROOT_PARAM_T0, pMaterial->GetTexture(asdx::Material::TEXTURE_BASE_COLOR).GetHandleGPU());
                pCmd->SetGraphicsRootDescriptorTable   (ROOT_PARAM_T1, pMaterial->GetTexture(asdx::Material::TEXTURE_NORMAL)    .GetHandleGPU());
                pCmd->SetGraphicsRootDescriptorTable   (ROOT_PARAM_T2, pMaterial->GetTexture(asdx::Material::TEXTURE_ORM)       .GetHandleGPU());
                pCmd->SetGraphicsRootDescriptorTable   (ROOT_PARAM_T3, pMaterial->GetTexture(asdx::Material::TEXTURE_EMISSIVE)  .GetHandleGPU());

                pCmd->IASetVertexBuffers(0, _countof(VBVs), VBVs);
                pCmd->IASetIndexBuffer(&IBV);
                pCmd->DrawIndexedInstanced(pMesh->GetIndexCount(), 1, 0, 0, 0);
            }
        }

        // 線分描画.
        m_LineRenderer.SetPipelineState(pCmd);
        pCmd->SetGraphicsRootConstantBufferView(0, m_SceneBuffer.GetGpuAddress());
        m_LineRenderer.Draw(pCmd);

        // スプライト描画.
        m_SpriteRenderer.SetPipelineState(pCmd);
        m_SpriteRenderer.Draw(pCmd);

        #if ASDX_ENABLE_IMGUI
        {
            // ImGui描画処理.
            asdx::GuiMgr::Instance().Draw(pCmd);
        }
        #endif

        // カラーバッファ表示用バリア.
        m_ColorTarget[idx].ChangeState(pCmd, D3D12_RESOURCE_STATE_PRESENT);
    }


    // コマンド記録終了.
    pCmd->Close();

    // コマンドを実行.
    {
        auto pGraphicsQueue = asdx::GetGraphicsQueue();

        // 前フレームの描画の完了を待機.
        if (m_FrameWaitPoint.IsValid())
        { pGraphicsQueue->Sync(m_FrameWaitPoint); }

        // コマンドを実行.
        if (asdx::TextureManager::Instance().HasCommand())
        {
            auto pTexCmd = asdx::TextureManager::Instance().Swap();

            ID3D12CommandList* pCmds[] = { pCmd, pTexCmd };
            pGraphicsQueue->Execute(_countof(pCmds), pCmds);
        }
        else
        {
            ID3D12CommandList* pCmds[] = { pCmd };

            pGraphicsQueue->Execute(_countof(pCmds), pCmds);
        }

        // 待機点を発行.
        m_FrameWaitPoint = pGraphicsQueue->Signal();
    }

    // 画面に表示.
    Present(1);

    // フレーム同期.
    asdx::FrameSync();
}

//-----------------------------------------------------------------------------
//      リサイズ時の処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnResize(const asdx::App::ResizeEventArgs& args)
{
}

//-----------------------------------------------------------------------------
//      キー処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnKey(const asdx::App::KeyEventArgs& args)
{
    m_Camera.OnKey(args.KeyCode, args.IsKeyDown, args.IsAltDown);
    #if ASDX_ENABLE_IMGUI
    {
        // ImGuiのキー処理.
        asdx::GuiMgr::Instance().OnKey(args.KeyCode, args.IsKeyDown, args.IsAltDown);
    }
    #endif
}

//-----------------------------------------------------------------------------
//      マウス処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnMouse(const asdx::App::MouseEventArgs& args)
{
    if(args.IsAltDown)
    {
        // カメラのマウス処理.
        m_Camera.OnMouse(
            args.X,
            args.Y,
            args.WheelDelta,
            args.IsDownL,
            args.IsDownR,
            args.IsDownM,
            args.IsDownX1,
            args.IsDownX2);
    }
    else
    {
        #if ASDX_ENABLE_IMGUI
        {
            // ImGuiのマウス処理.
            asdx::GuiMgr::Instance().OnMouse(
                args.X,
                args.Y,
                args.WheelDelta,
                args.IsDownL,
                args.IsDownM,
                args.IsDownR);
        }
        #endif
    }
}

//-----------------------------------------------------------------------------
//      タイピング処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnTyping(uint32_t keyCode)
{
    #if ASDX_ENABLE_IMGUI
    {
        // タイピング時の処理です.
        asdx::GuiMgr::Instance().OnTyping(keyCode);
    }
    #endif
}
