//-----------------------------------------------------------------------------
// File : SampleApp.cpp
// Desc : Sample Application.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "SampleApp.h"
#include <fnd/asdxLogger.h>
#include <fnd/asdxMisc.h>
#include <gfx/asdxDevice.h>
#include <edit/asdxGuiMgr.h>
#include <Meshlet.h>
#include <MeshOBJ.h>
#include <imgui.h>


namespace {

//-----------------------------------------------------------------------------
// Shaders.
//-----------------------------------------------------------------------------
#include "../../res/shader/Compiled/MeshletCullingAS.inc"
#include "../../res/shader/Compiled/MeshletCullingMS.inc"
#include "../../res/shader/Compiled/SimplePS.inc"

///////////////////////////////////////////////////////////////////////////////
// ROOT_INDEX enum
///////////////////////////////////////////////////////////////////////////////
enum ROOT_INDEX
{
    ROOT_B0,
    ROOT_B1,
    ROOT_T0,
    ROOT_T1,
    ROOT_T2,
    ROOT_T3,
    ROOT_T4,
    ROOT_T5,
    ROOT_T6,
};

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
static const uint32_t kMaxInstanceCount = 512;

///////////////////////////////////////////////////////////////////////////////
// TransformParam structure
///////////////////////////////////////////////////////////////////////////////
struct alignas(256) TransformParam
{
    asdx::Matrix    View;
    asdx::Matrix    Proj;
    asdx::Matrix    ViewProj;
    asdx::Vector3   CameraPos;
    float           Padding0;
    asdx::Vector4   Planes[6];
    asdx::Vector4   RenderTargetSize;

    asdx::Matrix    DebugView;
    asdx::Matrix    DebugProj;
    asdx::Matrix    DebugViewProj;
    asdx::Vector3   DebugCameraPos;
    float           DebugPadding0;
    asdx::Vector4   DebugPlanes[6];
};

///////////////////////////////////////////////////////////////////////////////
// MeshInstanceParam structure
///////////////////////////////////////////////////////////////////////////////
struct MeshInstanceParam
{
    asdx::Matrix    CurrWorld;
    asdx::Matrix    PrevWorld;
};

//-----------------------------------------------------------------------------
//      視錐台カリングを行います.
//-----------------------------------------------------------------------------
bool Contains(const asdx::Vector4& sphere, const std::array<asdx::Vector4, 6>& planes)
{
    asdx::Vector4 center(sphere.x, sphere.y, sphere.z, 1.0f);

    for(auto i=0; i<6; ++i)
    {
        if (asdx::Vector4::Dot(center, planes[i]) < -sphere.w)
            return false;
    }

    return true;
}

} // namespace

///////////////////////////////////////////////////////////////////////////////
// SampleApp class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
SampleApp::SampleApp()
: asdx::Application(L"Meshlet Culling", 1920, 1080, nullptr, nullptr, nullptr)
{
    m_SwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_ClearDepth      = 0.0f;

    m_DeviceDesc.MaxShaderResourceCount = 4096;
    m_DeviceDesc.MaxSamplerCount        = 256;
    m_DeviceDesc.MaxColorTargetCount    = 256;
    m_DeviceDesc.MaxDepthTargetCount    = 256;

    //m_DeviceDesc.EnableDebug = true;

    m_ClearColor[0] = 0.5f;
    m_ClearColor[1] = 0.5f;
    m_ClearColor[2] = 0.5f;
    m_ClearColor[3] = 1.0f;
}

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
SampleApp::~SampleApp()
{
}

//-----------------------------------------------------------------------------
//      初期化処理です.
//-----------------------------------------------------------------------------
bool SampleApp::OnInit()
{
    auto pDevice  = asdx::GetD3D12Device();
    auto pCmdList = m_GfxCmdList.Reset();

    ResMeshlets meshlets;

#if 0
    // メッシュレットを生成します.
    if (!CreateMeshlets("../../res/model/bunny.obj", meshlets))
    {
        ELOGA("Error : CreateMeshlet() Failed.");
        return false;
    }

    if (SaveResMeshlets("../../res/model/bunny.meshlets", meshlets))
    {
        ILOGA("Info : Save Meshlets.");
    }
#else
    if (!LoadResMeshlets("../../res/model/bunny.meshlets", meshlets))
    {
        ELOGA("Error : LoadMeshlets() Failed.");
        return false;
    }
#endif

    assert(meshlets.Positions .empty() == false);
    assert(meshlets.Normals   .empty() == false);
    assert(meshlets.TexCoords .empty() == false);
    assert(meshlets.Primitives.empty() == false);

    if (!m_PositionBuffer.Init(
        pCmdList,
        meshlets.Positions.size(),
        sizeof(asdx::Vector3),
        meshlets.Positions.data()))
    {
        ELOG("Error : PositionBuffer Init Failed.");
        return false;
    }
    m_PositionBuffer.GetResource()->SetName(L"PositionBuffer");

    if (!m_NormalBuffer.Init(
        pCmdList,
        meshlets.Normals.size(),
        sizeof(asdx::Vector3),
        meshlets.Normals.data()))
    {
        ELOG("Error : NormalBuffer Init Failed.");
        return false;
    }
    m_NormalBuffer.GetResource()->SetName(L"NormalBuffer");

    if (!m_TexCoordBuffer.Init(
        pCmdList,
        meshlets.TexCoords.size(),
        sizeof(asdx::Vector2),
        meshlets.TexCoords.data()))
    {
        ELOG("Error : TexCoordBuffer Init Failed.");
        return false;
    }
    m_TexCoordBuffer.GetResource()->SetName(L"TexCoordBuffer");

    {
        auto size = meshlets.Primitives.size() * 3 * sizeof(uint8_t);

        if (!m_PrimitiveBuffer.Init(
            pCmdList,
            size,
            meshlets.Primitives.data()))
        {
            ELOG("Error : PrimitiveIndexBuffer Init Failed.");
            return false;
        }
        m_PrimitiveBuffer.GetResource()->SetName(L"PrimitiveBuffer");

        std::vector<PrimitiveIndex> indices(meshlets.Primitives.size());
        for(size_t i=0; i<meshlets.Primitives.size(); ++i)
        {
            indices[i].x = meshlets.Primitives[i].x;
            indices[i].y = meshlets.Primitives[i].y;
            indices[i].z = meshlets.Primitives[i].z;
        }
    }

    if (!m_VertexIndexBuffer.Init(
        pCmdList,
        meshlets.VertexIndices.size(),
        sizeof(uint32_t),
        meshlets.VertexIndices.data()))
    {
        ELOG("Error : PrimitiveIndexBuffer2 Init Failed.");
        return false;
    }

    if (!m_MeshletBuffer.Init(
        pCmdList,
        meshlets.Meshlets.size(),
        sizeof(MeshletInfo),
        meshlets.Meshlets.data()))
    {
        ELOG("Error : MeshletBuffer Init Failed.");
        return false;
    }
    m_MeshletBuffer.GetResource()->SetName(L"MeshletBuffer");

    {
        std::vector<MeshInstanceParam> params;
        params.resize(kMaxInstanceCount);

        for(auto i=0u; i<kMaxInstanceCount; ++i)
        {
            params[i].CurrWorld = asdx::Matrix::CreateIdentity();
            params[i].PrevWorld = asdx::Matrix::CreateIdentity();
        }

        if (!m_MeshInstanceBuffer.Init(
            pCmdList,
            kMaxInstanceCount,
            sizeof(MeshInstanceParam),
            params.data()))
        {
            ELOG("Error : MeshInstanceBuffer Init Failed.");
            return false;
        }
    }

    if (!m_TransformBuffer.Init(sizeof(TransformParam)))
    {
        ELOG("Error : TransformBuffer Init Failed.");
        return false;
    }
    m_TransformBuffer.GetResource()->SetName(L"TransformBuffer");

    // ルートシグニチャ生成.
    {
        D3D12_DESCRIPTOR_RANGE ranges[7] = {};
        asdx::InitRangeAsSRV(ranges[0], 0);
        asdx::InitRangeAsSRV(ranges[1], 1);
        asdx::InitRangeAsSRV(ranges[2], 2);
        asdx::InitRangeAsSRV(ranges[3], 3);
        asdx::InitRangeAsSRV(ranges[4], 4);
        asdx::InitRangeAsSRV(ranges[5], 5);
        asdx::InitRangeAsSRV(ranges[6], 6);

        D3D12_ROOT_PARAMETER params[9] = {};

        auto ms = D3D12_SHADER_VISIBILITY_ALL;

        asdx::InitAsConstants(params[ROOT_B0], 0, 4, ms);
        asdx::InitAsCBV(params[ROOT_B1], 1, ms);
        asdx::InitAsTable(params[ROOT_T0], 1, &ranges[0], ms);
        asdx::InitAsTable(params[ROOT_T1], 1, &ranges[1], ms);
        asdx::InitAsTable(params[ROOT_T2], 1, &ranges[2], ms);
        asdx::InitAsTable(params[ROOT_T3], 1, &ranges[3], ms);
        asdx::InitAsTable(params[ROOT_T4], 1, &ranges[4], ms);
        asdx::InitAsTable(params[ROOT_T5], 1, &ranges[5], ms);
        asdx::InitAsTable(params[ROOT_T6], 1, &ranges[6], ms);

        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters      = _countof(params);
        desc.pParameters        = params;
        desc.NumStaticSamplers  = asdx::GetStaticSamplerCounts();
        desc.pStaticSamplers    = asdx::GetStaticSamplers();

        if (!asdx::InitRootSignature(pDevice, &desc, m_RootSigMS.GetAddress()))
        {
            ELOG("Error : RootSignature Init Failed.");
            return false;
        }
    }

    // メッシュシェーダパイプラインステート生成.
    {
        asdx::GEOMETRY_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature                 = m_RootSigMS.GetPtr();
        desc.AS                             = { MeshletCullingAS, sizeof(MeshletCullingAS) };
        desc.MS                             = { MeshletCullingMS, sizeof(MeshletCullingMS) };
        desc.PS                             = { SimplePS, sizeof(SimplePS) };
        desc.BlendState                     = asdx::Preset::Opaque;
        desc.SampleMask                     = D3D12_DEFAULT_SAMPLE_MASK;
        desc.RasterizerState                = asdx::Preset::CullBack;
        desc.DepthStencilState              = asdx::Preset::DepthDefault;
        desc.RTVFormats.NumRenderTargets    = 1;
        desc.RTVFormats.RTFormats[0]        = m_SwapChainFormat;
        desc.DSVFormat                      = m_DepthStencilFormat;
        desc.SampleDesc.Count               = 1;
        desc.SampleDesc.Quality             = 0;

        if (!m_PipelineStateMS.Init(pDevice, &desc))
        {
            ELOG("Error : PipelineState::Init() Failed.");
            return false;
        }
    }

    if (!m_ShapeStates.Init(pDevice, m_SwapChainFormat, m_DepthStencilFormat))
    {
        ELOG("Error : ShapeState Init Failed.");
        return false;
    }

    if (!m_MeshSphereShape.Init(pDevice, meshlets.BoundingSphere.w, 20))
    {
        ELOG("Error : MeshSphere Init Failed.");
        return false;
    }
    m_MeshSphereShape.SetWorld(
        asdx::Matrix::CreateTranslation(
            meshlets.BoundingSphere.x,
            meshlets.BoundingSphere.y,
            meshlets.BoundingSphere.z));
    m_MeshSphere = meshlets.BoundingSphere;

    if (!m_FrustumShape.Init(pDevice, 2.0f)) // [-1, 1] なのでサイズは2.0.
    {
        ELOG("Error : Frustum Shape Init Failed.");
    }
    m_FrustumShape.SetColor(asdx::Vector4(0.0f, 0.0f, 1.0f, 0.1f));

    m_MeshletSpheres.resize(meshlets.Meshlets.size());
    for(size_t i=0; i<meshlets.Meshlets.size(); ++i)
    {
        if (!m_MeshletSpheres[i].Init(pDevice, meshlets.Meshlets[i].BoundingSphere.w, 20))
        {
            ELOG("Error : Meshlet Sphere Init Failed.");
            return false;
        }

        m_MeshletSpheres[i].SetColor(asdx::Vector4(1.0f, 1.0f, 0.0f, 0.1f));
        m_MeshletSpheres[i].SetWorld(
            asdx::Matrix::CreateTranslation(
                meshlets.Meshlets[i].BoundingSphere.x,
                meshlets.Meshlets[i].BoundingSphere.y,
                meshlets.Meshlets[i].BoundingSphere.z));
    }

    // GUI初期化.
    if (!asdx::GuiMgr::Instance().Init(
        pCmdList,
        m_hWnd,
        m_Width,
        m_Height,
        m_SwapChainFormat,
        "../../res/font/07やさしさゴシック.ttf"))
    {
        ELOG("Error : asdx::GuiMgr::Init() Failed.");
        return false;
    }

    // コマンドリストへの記録を終了.
    pCmdList->Close();

    // コマンド実行と完了待機.
    {
        ID3D12CommandList* lists[] = { pCmdList };
        auto queue = asdx::GetGraphicsQueue();
        queue->Execute(1, lists);
        m_WaitPoint = queue->Signal();
        queue->Sync(m_WaitPoint);
    }

    const float radius = meshlets.BoundingSphere.w * 3.0f;

    m_Camera.Init(
        asdx::Vector3(0.0f, 0.0f, -radius),
        asdx::Vector3(0.0f, 0.0f, 0.0f),
        asdx::Vector3(0.0f, 1.0f, 0.0f),
        0.1f, 
        1000.0f);
    m_Camera.SetMoveGain(0.01f);
    m_Camera.SetDollyGain(0.1f);
    m_Camera.Present();

    m_DebugCamera.Init(
        asdx::Vector3(0.0f, 0.0f, -radius),
        asdx::Vector3(0.0f, 0.0f, 0.0f),
        asdx::Vector3(0.0f, 1.0f, 0.0f),
        0.1f, 
        1000.0f);
    m_DebugCamera.SetMoveGain(0.01f);
    m_DebugCamera.SetDollyGain(0.1f);
    m_DebugCamera.Present();

    m_MeshletInfos = std::move(meshlets.Meshlets);

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnTerm()
{
    // アイドル状態になるまで待機.
    asdx::SystemWaitIdle();

    m_ShapeStates.Term();
    m_MeshSphereShape .Term();

    m_PositionBuffer    .Term();
    m_NormalBuffer      .Term();
    m_TexCoordBuffer    .Term();
    m_PrimitiveBuffer   .Term();
    m_VertexIndexBuffer .Term();
    m_MeshletBuffer     .Term();
    m_MeshInstanceBuffer.Term();
    m_TransformBuffer   .Term();

    m_PipelineStateMS.Term();
    m_RootSigMS.Reset();

    // GUI破棄.
    asdx::GuiMgr::Instance().Term();
}

//-----------------------------------------------------------------------------
//      フレーム遷移処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnFrameMove(asdx::FrameEventArgs& args)
{
    auto aspectRatio = float(m_Width) / float(m_Height);

    TransformParam param = {};
    param.View  = m_Camera.GetView();
    param.Proj  = asdx::Matrix::CreatePerspectiveFieldOfView(
        asdx::ToRadian(37.5f),
        aspectRatio,
        m_Camera.GetNearClip(),
        m_Camera.GetFarClip());
    param.ViewProj = param.View * param.Proj;
    param.CameraPos = m_Camera.GetPosition();
    asdx::CalcFrustumPlanes(param.View, param.Proj, param.Planes);
    for(auto i=0; i<6; ++i)
    { m_MainFrustumPlanes[i] = param.Planes[i]; }
    param.RenderTargetSize.x = float(m_Width);
    param.RenderTargetSize.y = float(m_Height);
    param.RenderTargetSize.z = 1.0f / float(m_Width);
    param.RenderTargetSize.w = 1.0f / float(m_Height);

    param.DebugView  = m_DebugCamera.GetView();
    param.DebugProj  = asdx::Matrix::CreatePerspectiveFieldOfView(
        asdx::ToRadian(37.5f),
        aspectRatio,
        m_DebugCamera.GetNearClip(),
        m_DebugCamera.GetFarClip());
    param.DebugViewProj = param.View * param.Proj;
    param.DebugCameraPos = m_Camera.GetPosition();
    asdx::CalcFrustumPlanes(param.DebugView, param.DebugProj, param.DebugPlanes);
    for(auto i=0; i<6; ++i)
    { m_DebugFrustumPlanes[i] = param.DebugPlanes[i]; }

    m_TransformBuffer.SwapBuffer();
    m_TransformBuffer.Update(&param, sizeof(param));

    m_ShapeStates.SetMatrix(param.DebugView, param.DebugProj);

    auto invViewProj = asdx::Matrix::Invert(param.Proj) * asdx::Matrix::Invert(param.View);
    m_FrustumShape.SetWorld(invViewProj);

    asdx::GuiMgr::Instance().Update(m_Width, m_Height);
    ImGui::Begin("Debug Control");
    ImGui::Checkbox("Enable Debug Camera", &m_EnableDebugCamera);
    ImGui::Checkbox("Enable Shading", &m_EnableShading);
    ImGui::Checkbox("Enable Swap View", &m_EnableSwapView);
    ImGui::Checkbox("Draw Mesh Sphere", &m_DrawMeshSphere);
    ImGui::Checkbox("Draw Meshlet Sphere", &m_DrawMeshletSphere);
    ImGui::Checkbox("Draw Frustum", &m_DrawFrustum);
    ImGui::Text("Cull From MainCamera : %s", m_CullFromMain ? "Yes" : "No");
    ImGui::Text("Cull From DebugCamera : %s", m_CullFromDebug ? "Yes" : "No");
    ImGui::End();
}

//-----------------------------------------------------------------------------
//      描画処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnFrameRender(asdx::FrameEventArgs& args)
{
    auto idx = GetCurrentBackBufferIndex();

    // コマンド記録開始.
    auto pCmd = m_GfxCmdList.Reset();


    // カラーターゲットに描画.
    {
        // レンダーターゲットに遷移.
        m_ColorTarget[idx].ChangeState(pCmd, D3D12_RESOURCE_STATE_RENDER_TARGET);

        // レンダーターゲットをクリア.
        auto handleRTV = m_ColorTarget[idx].GetRTV()->GetHandleCPU();
        auto handleDSV = m_DepthTarget.GetDSV()->GetHandleCPU();
        pCmd->ClearRenderTargetView(handleRTV, m_ClearColor, 0, nullptr);
        pCmd->ClearDepthStencilView(handleDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        // レンダーターゲットを設定.
        pCmd->OMSetRenderTargets(1, &handleRTV, FALSE, &handleDSV);

        // ビューポートとシザー矩形を設定.
        pCmd->RSSetViewports(1, &m_Viewport);
        pCmd->RSSetScissorRects(1, &m_ScissorRect);

        bool debugView = m_EnableSwapView;
        DrawMeshlets(pCmd, m_MeshletInfos, 0, debugView);

        // デバッグシェイプ描画.
        if (debugView)
        {
            if (m_DrawMeshSphere)
            {
                m_ShapeStates.ApplyWireframeState(pCmd);
                m_MeshSphereShape.Draw(pCmd);
            }
            if (m_DrawFrustum)
            {
                m_ShapeStates.ApplyWireframeState(pCmd);
                m_FrustumShape.Draw(pCmd);
            }
            if (m_DrawMeshletSphere)
            {
                m_ShapeStates.ApplyTranslucentState(pCmd);
                for(size_t i=0; i<m_MeshletSpheres.size(); ++i)
                {
                    m_MeshletSpheres[i].Draw(pCmd);
                }
            }
        }

        auto w = m_Width / 4;
        auto h = m_Height / 4;
        D3D12_VIEWPORT viewport = {};
        viewport.TopLeftX   = FLOAT(m_Width - w);
        viewport.TopLeftY   = FLOAT(m_Height - h);
        viewport.Width      = FLOAT(w);
        viewport.Height     = FLOAT(h);
        viewport.MinDepth   = 0.0f;
        viewport.MaxDepth   = 1.0f;

        D3D12_RECT scissor = {};
        scissor.left    = m_Width - w;
        scissor.right   = m_Width;
        scissor.top     = m_Height - h;
        scissor.bottom  = m_Height;

        // ビューポートとシザー矩形を設定.
        pCmd->RSSetViewports(1, &viewport);
        pCmd->RSSetScissorRects(1, &scissor);

        DrawMeshlets(pCmd, m_MeshletInfos, 0, !debugView);

        // デバッグシェイプ描画.
        if (!debugView)
        {
            if (m_DrawMeshSphere)
            {
                m_ShapeStates.ApplyWireframeState(pCmd);
                m_MeshSphereShape.Draw(pCmd);
            }
            if (m_DrawFrustum)
            {
                m_ShapeStates.ApplyWireframeState(pCmd);
                m_FrustumShape.Draw(pCmd);
            }
            if (m_DrawMeshletSphere)
            {
                m_ShapeStates.ApplyTranslucentState(pCmd);
                for(size_t i=0; i<m_MeshletSpheres.size(); ++i)
                {
                    m_MeshletSpheres[i].Draw(pCmd);
                }
            }
        }

        asdx::GuiMgr::Instance().Draw(pCmd);

        // 表示用に遷移.
        m_ColorTarget[idx].ChangeState(pCmd, D3D12_RESOURCE_STATE_PRESENT);
    }

    // コマンド記録終了.
    pCmd->Close();

    {
        auto pQueue = asdx::GetGraphicsQueue();

        // 前フレームのコマンドが完了していなければ待機.
        pQueue->Sync(m_WaitPoint);

        // コマンドリストを実行.
        ID3D12CommandList* pLists[] = { pCmd };
        pQueue->Execute(_countof(pLists), pLists);

        // 待機点を取得.
        m_WaitPoint = pQueue->Signal();
    }

    // 画面に表示.
    Present(0);
}

//-----------------------------------------------------------------------------
//      リサイズ処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnResize(const asdx::ResizeEventArgs& args)
{
}

//-----------------------------------------------------------------------------
//      キー処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnKey(const asdx::KeyEventArgs& args)
{
    if (args.IsKeyDown)
    {
        switch(args.KeyCode)
        {
        case 'S':
            m_EnableShading = !m_EnableShading;
            break;

        case 'C':
            m_EnableDebugCamera = !m_EnableDebugCamera;
            break;

        default:
            break;
        }
    }

    m_Camera.OnKey(args.KeyCode, args.IsKeyDown, args.IsAltDown);
    m_DebugCamera.OnKey(args.KeyCode, args.IsKeyDown, args.IsAltDown);
    asdx::GuiMgr::Instance().OnKey(args.IsKeyDown, args.IsAltDown, args.KeyCode);
}

//-----------------------------------------------------------------------------
//      マウス処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnMouse(const asdx::MouseEventArgs& args)
{
    auto isAltDown = !!(GetAsyncKeyState(VK_MENU) & 0x1);
    if (isAltDown)
    {
        if (!m_EnableDebugCamera)
        {
            m_Camera.OnMouse(
                args.X, args.Y,
                args.WheelDelta,
                args.IsLeftButtonDown,
                args.IsRightButtonDown,
                args.IsMiddleButtonDown,
                args.IsSideButton1Down,
                args.IsSideButton2Down);
        }
        else
        {
            m_DebugCamera.OnMouse(
                args.X, args.Y,
                args.WheelDelta,
                args.IsLeftButtonDown,
                args.IsRightButtonDown,
                args.IsMiddleButtonDown,
                args.IsSideButton1Down,
                args.IsSideButton2Down);
        }
    }
    else
    {
        asdx::GuiMgr::Instance().OnMouse(
            args.X, args.Y,
            args.WheelDelta,
            args.IsLeftButtonDown,
            args.IsMiddleButtonDown,
            args.IsRightButtonDown);
    }
}

//-----------------------------------------------------------------------------
//      タイピング処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnTyping(uint32_t keyCode)
{
    asdx::GuiMgr::Instance().OnTyping(keyCode);
}

//-----------------------------------------------------------------------------
//      メッシュレットを描画します.
//-----------------------------------------------------------------------------
void SampleApp::DrawMeshlets
(
    ID3D12GraphicsCommandList6*     pCmd,
    const std::vector<MeshletInfo>& meshlets,
    uint32_t                        instanceId,
    bool                            debugView
)
{
    if (!Contains(m_MeshSphere, debugView ? m_DebugFrustumPlanes : m_MainFrustumPlanes))
    {
        if (debugView)
        { m_CullFromDebug = true; }
        else
        { m_CullFromMain = true; }
        return;
    }

    if (debugView)
    { m_CullFromDebug = false; }
    else
    { m_CullFromMain = false; }

    struct Constants
    {
        uint32_t MeshletCount;
        uint32_t InstanceId;
        float    MinContribution;
        uint32_t Flags;
    } constants = {};
       
    constants.MeshletCount    = uint32_t(meshlets.size());
    constants.InstanceId      = instanceId;
    constants.MinContribution = 1e-4f;
    constants.Flags           = (debugView) ? 1 : 0;

    constants.Flags |= (m_EnableShading) ? (0x1 << 1) : 0;

    uint32_t dispatchCount = (constants.MeshletCount + 31) / 32;

    pCmd->SetGraphicsRootSignature(m_RootSigMS.GetPtr());
    m_PipelineStateMS.SetState(pCmd);
    pCmd->SetGraphicsRoot32BitConstants(ROOT_B0, 4, &constants, 0);
    pCmd->SetGraphicsRootConstantBufferView(ROOT_B1, m_TransformBuffer.GetResource()->GetGPUVirtualAddress());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T0, m_PositionBuffer    .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T1, m_NormalBuffer      .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T2, m_TexCoordBuffer    .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T3, m_PrimitiveBuffer   .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T4, m_VertexIndexBuffer .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T5, m_MeshletBuffer     .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T6, m_MeshInstanceBuffer.GetSRV()->GetHandleGPU());
    pCmd->DispatchMesh(dispatchCount, 1, 1);
}

//-----------------------------------------------------------------------------
//      視錐台を描画します.
//-----------------------------------------------------------------------------
void SampleApp::DrawFrsutum(ID3D12GraphicsCommandList6* pCmd, const asdx::Vector4& color)
{
    auto VB = m_FrsutumVB.GetVBV();
    auto IB = m_FrustumIB.GetIBV();

    pCmd->SetGraphicsRootSignature(m_RootSigVS.GetPtr());
    m_PipelineStateVS.SetState(pCmd);
    pCmd->SetGraphicsRootConstantBufferView(0, m_TransformBuffer.GetResource()->GetGPUVirtualAddress());
    pCmd->SetGraphicsRoot32BitConstants(1, 4, &color, 0);
    pCmd->IASetVertexBuffers(0, 1, &VB);
    pCmd->IASetIndexBuffer(&IB);
    pCmd->DrawIndexedInstanced(m_FrustumIndexCount, 1, 0, 0, 0);
}