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
#include <gfx/asdxDevice.h>
#include <edit/asdxGuiMgr.h>
#include <Meshlet.h>
#include <MeshOBJ.h>
#include <imgui.h>
#include <LodGenerator.h>


namespace {

//-----------------------------------------------------------------------------
// Shader Binaries.
//-----------------------------------------------------------------------------
#include "../../res/shader/Compiled/LodMeshletAS.inc"
#include "../../res/shader/Compiled/LodMeshletMS.inc"
#include "../../res/shader/Compiled/LodMeshletPS.inc"
#include "../../res/shader/Compiled/AutoLodMeshletAS.inc"

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
    ROOT_T7,
    ROOT_U0,
};

///////////////////////////////////////////////////////////////////////////////
// LOD_MODE enum
///////////////////////////////////////////////////////////////////////////////
enum LOD_MODE
{
    LOD_MODE_MANUAL,    //!< 手動制御.
    LOD_MODE_AUTO,      //!< 自動制御.
};

// カラーモード.
static const char* kColorModes[] = {
    "Shading",
    "Normal",
    "TexCoord",
    "MeshletID",
    "PrimitiveID",
    "LODIndex",
    "MaterialID",
};

// LODモード.
static const char* kLodModes[] = {
    "Manual",
    "Auto",
};

// 最大インスタンス数.
static const uint32_t kMaxInstanceCount = 512;

///////////////////////////////////////////////////////////////////////////////
// TransformParam structure
///////////////////////////////////////////////////////////////////////////////
struct alignas(256) TransformParam
{
    // Main Camera.
    asdx::Matrix    View;               //!< ビュー行列.
    asdx::Matrix    Proj;               //!< 射影行列.
    asdx::Matrix    ViewProj;           //!< ビュー射影行列.
    asdx::Vector3   CameraPos;          //!< カメラ位置.
    float           FieldOfView;        //!< 垂直画角.
    asdx::Vector4   Planes[6];          //!< 視錐台平面.
    asdx::Vector4   RenderTargetSize;   //!< レンダーターゲットサイズ.

    // Debug Camera.
    asdx::Matrix    DebugView;          //!< ビュー行列.
    asdx::Matrix    DebugProj;          //!< 射影行列.
    asdx::Matrix    DebugViewProj;      //!< ビュー射影行列.
    asdx::Vector3   DebugCameraPos;     //!< カメラ位置.
    float           DebugFieldOfView;   //!< 垂直画角.
    asdx::Vector4   DebugPlanes[6];     //!< 視錐台平面.
};

///////////////////////////////////////////////////////////////////////////////
// MeshInstanceParam structure
///////////////////////////////////////////////////////////////////////////////
struct MeshInstanceParam
{
    asdx::Matrix    CurrWorld;      //!< 現在フレームのワールド行列.
    asdx::Matrix    PrevWorld;      //!< 前フレームのワールド行列.
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
            return false;   // カリングする.
    }

    return true;    // カリングしない.
}

//-----------------------------------------------------------------------------
//      小数部の値を取得します.
//-----------------------------------------------------------------------------
inline float frac(float value)
{ return value - floor(value); }

//-----------------------------------------------------------------------------
//      色相からRGBを求めます.
//-----------------------------------------------------------------------------
inline asdx::Vector3 HueToRGB(float hue)
{
    hue = frac(hue);
    float r = -1.0f + abs(hue * 6.0f - 3.0f);
    float g =  2.0f - abs(hue * 6.0f - 2.0f);
    float b =  2.0f - abs(hue * 6.0f - 4.0f);
    return asdx::Vector3::Saturate(asdx::Vector3(r, g, b));
}

//-----------------------------------------------------------------------------
//      IDからRGBを求めます.
//-----------------------------------------------------------------------------
inline asdx::Vector3 IdToRGB(uint32_t id)
{
    const auto kOffset = 0.53115f;      // 適当に色を変えるためのオフセット.
    return HueToRGB(id * 1.71f + kOffset);
}

//-----------------------------------------------------------------------------
//      バウンディングスフィアを変換します.
//-----------------------------------------------------------------------------
asdx::Vector4 TransformSphere(const asdx::Vector4& sphere, const asdx::Matrix& transform)
{
    asdx::Vector3 center = asdx::Vector3::Transform(asdx::Vector3(sphere.x, sphere.y, sphere.z), transform);

    const auto xAxis = asdx::Vector3(transform._11, transform._12, transform._13);
    const auto yAxis = asdx::Vector3(transform._21, transform._22, transform._23);
    const auto zAxis = asdx::Vector3(transform._31, transform._32, transform._33);
    auto sx = asdx::Vector3::Dot(xAxis, xAxis);
    auto sy = asdx::Vector3::Dot(yAxis, yAxis);
    auto sz = asdx::Vector3::Dot(zAxis, zAxis);
    auto scale = std::sqrt(asdx::Max(sx, asdx::Max(sy, sz)));
    return asdx::Vector4(center, sphere.w * scale);
}

//-----------------------------------------------------------------------------
//      指定されたメッシュレットが視認可能かどうかチェックします.
//-----------------------------------------------------------------------------
bool IsVisibleLod
(
    const asdx::Vector4&    sphere,
    const asdx::Vector4&    parentSphere,
    float                   clusterError,
    float                   parentError,
    float                   height,
    float                   fov,
    const asdx::Matrix&     world,
    const asdx::Matrix&     view,
    float                   errorThreshold
)
{
    auto projectErrorToScreen = [&](const asdx::Vector4& sphere)
    {
        if (std::isinf(sphere.w))
            return sphere.w;

        const auto center = asdx::Vector3(sphere.x, sphere.y, sphere.z);
        const auto d2 = asdx::Vector3::Dot(center, center);
        const auto r2 = sphere.w * sphere.w;
        return height * 0.5f * (1.0f / tan(fov * 0.5f)) * sphere.w / sqrt(d2 - r2);
    };

    const auto transform = world * view;

    const auto childCenter = asdx::Vector3::Transform(asdx::Vector3(sphere.x, sphere.y, sphere.z), transform);
    const auto childBounds = asdx::Vector4(childCenter, asdx::Max(clusterError, 1e-8f));
    const auto errorC = projectErrorToScreen(childBounds);

    const auto parentCenter = asdx::Vector3::Transform(asdx::Vector3(parentSphere.x, parentSphere.y, parentSphere.z), transform);
    const auto parentBounds = asdx::Vector4(parentCenter, asdx::Max(parentError, 1e-8f));
    const auto errorP = projectErrorToScreen(parentBounds);

    return errorC <= errorThreshold && errorThreshold < errorP;

}

} // namespace

///////////////////////////////////////////////////////////////////////////////
// SampleApp class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
SampleApp::SampleApp()
: asdx::Application(L"Level of details", 1920, 1080, nullptr, nullptr, nullptr)
{
    m_SwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_ClearDepth      = 0.0f;

    m_DeviceDesc.MaxShaderResourceCount = 4096;
    m_DeviceDesc.MaxSamplerCount        = 256;
    m_DeviceDesc.MaxColorTargetCount    = 256;
    m_DeviceDesc.MaxDepthTargetCount    = 256;

    m_DeviceDesc.EnableDebug = true;

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

    ResLodMeshlets lods;
    {
        ResMeshlets meshlets;
#if 1
        if (!CreateMeshlets("../../res/model/bunny.obj", meshlets))
        {
            ELOGA("Error : CreateMeshlets() Failed.");
            return false;
        }

        if (SaveResMeshlets("../../res/model/bunny.meshlets", meshlets))
        {
            ILOGA("Info : Save Meshlets.");
        }
#else
        if (!LoadResMeshlets("../../res/model/bunny.meshlets", meshlets))
        {
            ELOGA("Error : LoadResMeshlets() Failed.");
            return false;
        }
#endif

        if (!CreateLodMeshlets(meshlets, lods))
        {
            ELOGA("Error : CreateLodMeshlets(). Failed.");
            return false;
        }
    }

    if (!m_PositionBuffer.Init(
        pCmdList,
        lods.Positions.size(),
        sizeof(lods.Positions[0]),
        lods.Positions.data()))
    {
        ELOG("Error : PositionBuffer Init Failed.");
        return false;
    }
    m_PositionBuffer.GetResource()->SetName(L"PositionBuffer");

    if (!m_NormalBuffer.Init(
        pCmdList,
        lods.Normals.size(),
        sizeof(lods.Normals[0]),
        lods.Normals.data()))
    {
        ELOG("Error : NormalBuffer Init Failed.");
        return false;
    }
    m_NormalBuffer.GetResource()->SetName(L"NormalBuffer");

    if (!m_TexCoordBuffer.Init(
        pCmdList,
        lods.TexCoords.size(),
        sizeof(lods.TexCoords[0]),
        lods.TexCoords.data()))
    {
        ELOG("Error : TexCoordBuffer Init Failed.");
        return false;
    }
    m_TexCoordBuffer.GetResource()->SetName(L"TexCoordBuffer");

    {
        std::vector<uint8_t3> prims;
        for(size_t i=0; i<lods.Meshlets.size(); ++i)
        {
            for(size_t j=0; j<lods.Meshlets[i].Primitives.size(); ++j)
            {
                prims.push_back(lods.Meshlets[i].Primitives[j]);
            }
        }

        if (!m_PrimitiveBuffer.Init(pCmdList, prims.size(), sizeof(uint8_t3), prims.data()))
        {
            ELOG("Error : PrimitiveBuffer Init Failed.");
            return false;
        }
    }
    
    {
        std::vector<uint32_t> vertIndices;
        for(size_t i=0; i<lods.Meshlets.size(); ++i)
        {
            for(size_t j=0; j<lods.Meshlets[i].VertIndices.size(); ++j)
            {
                vertIndices.push_back(lods.Meshlets[i].VertIndices[j]);
            }
        }

        if (!m_VertexIndexBuffer.Init(pCmdList, vertIndices.size(), sizeof(uint32_t), vertIndices.data()))
        {
            ELOG("Error : VertexIndexBuffer Init Failed.");
            return false;
        }
    }

    {
        auto count = lods.Meshlets.size();
        std::vector<MyMeshlet> meshlets;
        meshlets.resize(count);

        uint32_t vertexOffset    = 0u;
        uint32_t primitiveOffset = 0u;
        for(size_t i=0; i<count; ++i)
        {
            meshlets[i].VertexOffset            = vertexOffset;
            meshlets[i].VertexCount             = uint32_t(lods.Meshlets[i].VertIndices.size());
            meshlets[i].PrimitiveOffset         = primitiveOffset;
            meshlets[i].PrimitiveCount          = uint32_t(lods.Meshlets[i].Primitives.size());
            meshlets[i].NormalCone              = lods.Meshlets[i].NormalCone;
            meshlets[i].BoundingSphere          = lods.Meshlets[i].BoundingSphere;
            meshlets[i].GroupError              = lods.Meshlets[i].GroupError;
            meshlets[i].ParentError             = lods.Meshlets[i].ParentError;
            meshlets[i].MaterialId              = lods.Meshlets[i].MaterialId;
            meshlets[i].Lod                     = lods.Meshlets[i].Lod;
            meshlets[i].GroupBoundingSphere     = lods.Meshlets[i].GroupBounds;
            meshlets[i].ParentBoundingSphere    = lods.Meshlets[i].ParentBounds;

            vertexOffset    += meshlets[i].VertexCount;
            primitiveOffset += meshlets[i].PrimitiveCount;
        }

        if (!m_MeshletBuffer.Init(pCmdList, count, sizeof(MyMeshlet), meshlets.data()))
        {
            ELOG("Error : MeshletBuffer Init Failed.");
            return false;
        }
    }

    if (!m_LodRangeBuffer.Init(pCmdList, lods.LodRanges.size(), sizeof(lods.LodRanges[0]), lods.LodRanges.data()))
    {
        ELOG("Error : LodRangeBuffer Init Failed.");
        return false;
    }

    // ルートシグニチャ生成.
    {
        D3D12_DESCRIPTOR_RANGE ranges[9] = {};
        asdx::InitRangeAsSRV(ranges[0], 0);
        asdx::InitRangeAsSRV(ranges[1], 1);
        asdx::InitRangeAsSRV(ranges[2], 2);
        asdx::InitRangeAsSRV(ranges[3], 3);
        asdx::InitRangeAsSRV(ranges[4], 4);
        asdx::InitRangeAsSRV(ranges[5], 5);
        asdx::InitRangeAsSRV(ranges[6], 6);
        asdx::InitRangeAsSRV(ranges[7], 7);
        asdx::InitRangeAsUAV(ranges[8], 0);

        D3D12_ROOT_PARAMETER params[11] = {};

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
        asdx::InitAsTable(params[ROOT_T7], 1, &ranges[7], ms);
        asdx::InitAsTable(params[ROOT_U0], 1, &ranges[8], ms);

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
        desc.AS                             = { LodMeshletAS, sizeof(LodMeshletAS) };
        desc.MS                             = { LodMeshletMS, sizeof(LodMeshletMS) };
        desc.PS                             = { LodMeshletPS, sizeof(LodMeshletPS) };
        desc.BlendState                     = asdx::Preset::Opaque;
        desc.SampleMask                     = D3D12_DEFAULT_SAMPLE_MASK;
        desc.RasterizerState                = asdx::Preset::CullBack;
        desc.DepthStencilState              = asdx::Preset::DepthDefault;
        desc.RTVFormats.NumRenderTargets    = 1;
        desc.RTVFormats.RTFormats[0]        = m_SwapChainFormat;
        desc.DSVFormat                      = m_DepthStencilFormat;
        desc.SampleDesc.Count               = 1;
        desc.SampleDesc.Quality             = 0;

        if (!m_ManualPipelineStateMS.Init(pDevice, &desc))
        {
            ELOG("Error : PipelineStateMS Init Failed.");
            return false;
        }

        desc.RasterizerState = asdx::Preset::Wireframe;
        if (!m_ManualWireframeStateMS.Init(pDevice, &desc))
        {
            ELOG("Error : WireframeStateMS Init Failed.");
            return false;
        }

        desc.RasterizerState = asdx::Preset::CullBack;
        desc.AS = { AutoLodMeshletAS, sizeof(AutoLodMeshletAS) };
        if (!m_AutoPipelineStateMS.Init(pDevice, &desc))
        {
            ELOG("Error : AutoPipelineStateMS Init Failed.");
            return false;
        }

        desc.RasterizerState = asdx::Preset::Wireframe;
        if (!m_AutoWireframeStateMS.Init(pDevice, &desc))
        {
            ELOG("Error : AutoWireframeStateMS Init Failed.");
            return false;
        }
    }

    if (!m_ShapeStates.Init(pDevice, m_SwapChainFormat, m_DepthStencilFormat))
    {
        ELOG("Error : ShapeState Init Failed.");
        return false;
    }

    if (!m_MeshSphereShape.Init(pDevice, lods.BoundingSphere.w, 20))
    {
        ELOG("Error : MeshSphere Init Failed.");
        return false;
    }
    m_MeshSphereShape.SetWorld(
        asdx::Matrix::CreateTranslation(
            lods.BoundingSphere.x,
            lods.BoundingSphere.y,
            lods.BoundingSphere.z));
    m_MeshSphere = lods.BoundingSphere;

    m_MeshletSpheres.resize(lods.Meshlets.size());
    for(size_t i=0; i<lods.LodRanges.size(); ++i)
    {
        for(size_t j=0; j<lods.LodRanges[i].Count; ++j)
        {
            auto  meshletId = lods.LodRanges[i].Offset + j;
            auto& sphere    = lods.Meshlets[meshletId].BoundingSphere;

            if (!m_MeshletSpheres[meshletId].Init(pDevice, sphere.w, 20))
            {
                ELOG("Error : Meshlet Sphere Init Failed.");
                return false;
            }

            m_MeshletSpheres[meshletId].SetColor(asdx::Vector4(IdToRGB(uint32_t(i)), 0.1f));
            m_MeshletSpheres[meshletId].SetWorld(
                asdx::Matrix::CreateTranslation(
                    sphere.x,
                    sphere.y,
                    sphere.z));
        }
    }

    {
        std::vector<MeshInstanceParam> params;
        params.resize(kMaxInstanceCount);

        for(auto i=0u; i<kMaxInstanceCount; ++i)
        {
            params[i].CurrWorld = asdx::Matrix::CreateIdentity();
            params[i].PrevWorld = asdx::Matrix::CreateIdentity();
        }



        if (!m_InstanceBuffer.Init(
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

    {
        struct DebugWrite
        {
            float projClusterError;
            float projParentError;
            asdx::Vector4 groupBounds;
            asdx::Vector4 parentBounds;
        };

        asdx::TargetDesc desc;
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width              = lods.Meshlets.size() * sizeof(DebugWrite);
        desc.Height             = 1;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.Format             = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.InitState          = D3D12_RESOURCE_STATE_COMMON;

        if (!m_DebugWriteBuffer.Init(&desc, sizeof(DebugWrite)))
        {
            ELOG("Error : DebugWriteIndexBuffer Init Failed.");
            return false;
        }
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

    const float radius = lods.BoundingSphere.w * 3.0f;

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

    m_LodRanges  = std::move(lods.LodRanges);
    m_MeshSphere = lods.BoundingSphere;
    m_Meshlets   = std::move(lods.Meshlets);
    m_Subsets    = std::move(lods.Subsets);
    m_MaxLodLevel = uint32_t(lods.MaxLodLevel);

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnTerm()
{
    // アイドル状態になるまで待機.
    asdx::SystemWaitIdle();

    m_ShapeStates     .Term();
    m_MeshSphereShape .Term();

    m_PositionBuffer    .Term();
    m_NormalBuffer      .Term();
    m_TexCoordBuffer    .Term();
    m_PrimitiveBuffer   .Term();
    m_VertexIndexBuffer .Term();
    m_MeshletBuffer     .Term();
    m_InstanceBuffer    .Term();
    m_LodRangeBuffer    .Term();
    m_TransformBuffer   .Term();

    m_ManualWireframeStateMS.Term();
    m_ManualPipelineStateMS .Term();
    m_AutoWireframeStateMS  .Term();
    m_AutoPipelineStateMS   .Term();
    m_RootSigMS.Reset();

    m_PipelineStateVS.Term();
    m_RootSigVS.Reset();

    m_DebugWriteBuffer.Term();

    // GUI破棄.
    asdx::GuiMgr::Instance().Term();
}

//-----------------------------------------------------------------------------
//      フレーム遷移処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnFrameMove(asdx::FrameEventArgs& args)
{
    auto aspectRatio = float(m_Width) / float(m_Height);
    constexpr auto fov = asdx::ToRadian(37.5f);

    TransformParam param = {};
    param.View  = m_Camera.GetView();
    param.Proj  = asdx::Matrix::CreatePerspectiveFieldOfView(
        fov,
        aspectRatio,
        m_Camera.GetNearClip(),
        m_Camera.GetFarClip());
    param.ViewProj = param.View * param.Proj;
    param.CameraPos = m_Camera.GetPosition();
    param.FieldOfView = fov;
    asdx::CalcFrustumPlanes(param.View, param.Proj, param.Planes);
    param.RenderTargetSize.x = float(m_Width);
    param.RenderTargetSize.y = float(m_Height);
    param.RenderTargetSize.z = 1.0f / float(m_Width);
    param.RenderTargetSize.w = 1.0f / float(m_Height);

    param.DebugView  = m_DebugCamera.GetView();
    param.DebugProj  = asdx::Matrix::CreatePerspectiveFieldOfView(
        fov,
        aspectRatio,
        m_DebugCamera.GetNearClip(),
        m_DebugCamera.GetFarClip());
    param.DebugViewProj = param.View * param.Proj;
    param.DebugCameraPos = m_Camera.GetPosition();
    param.DebugFieldOfView = fov;
    asdx::CalcFrustumPlanes(param.DebugView, param.DebugProj, param.DebugPlanes);

    for(auto i=0; i<6; ++i)
    { m_MainFrustumPlanes[i] = param.Planes[i]; }

    for(auto i=0; i<6; ++i)
    { m_DebugFrustumPlanes[i] = param.DebugPlanes[i]; }

    m_TransformBuffer.SwapBuffer();
    m_TransformBuffer.Update(&param, sizeof(param));

    m_ShapeStates.SetMatrix(param.View, param.Proj);


    asdx::GuiMgr::Instance().Update(m_Width, m_Height);
    ImGui::Begin("Debug Control");
    ImGui::Combo("Color Mode", &m_ColorMode, kColorModes, _countof(kColorModes));
    ImGui::Combo("LOD Mode", &m_LodMode, kLodModes, _countof(kLodModes));
    ImGui::Text("Max LOD : %d", m_MaxLodLevel);
    if (m_LodMode == LOD_MODE_MANUAL)
    {
        ImGui::SliderInt(u8"LOD Index", &m_CurrLodIndex, 0, int(m_MaxLodLevel - 1));

        if (m_CurrLodIndex >= m_MaxLodLevel)
        { m_CurrLodIndex = int(m_MaxLodLevel - 1); }
        else if (m_CurrLodIndex < 0)
        { m_CurrLodIndex = 0; }
    }

    const auto& lod = m_LodRanges[m_CurrLodIndex];

    ImGui::Text("Meshlet Offset : %u", lod.Offset);
    ImGui::Text("Meshlet Count  : %u", lod.Count);

    size_t vertexCount   = 0;
    size_t triangleCount = 0;
    for(size_t i=0; i<lod.Count; ++i)
    {
        const auto& meshlet = m_Meshlets[i + lod.Offset];
        vertexCount   += meshlet.VertIndices.size();
        triangleCount += meshlet.Primitives .size();
    }

    ImGui::Text("Vertex Count   : %zu", vertexCount);
    ImGui::Text("Polygon Count  : %zu", triangleCount);

    ImGui::Checkbox("Wireframe", &m_EnableWireframe);
    ImGui::Checkbox("Draw Sphere", &m_DrawMeshletSphere);

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

        if (m_LodMode == LOD_MODE_MANUAL)
        {
            DrawMeshletsManualControl(pCmd, 0);
        }
        else
        {
            DrawMeshletsAutoControl(pCmd, 0);
        }

        if (m_DrawMeshletSphere)
        {
            auto lodIndex = uint32_t(m_CurrLodIndex);
            const auto& range = m_LodRanges[lodIndex];
            m_ShapeStates.ApplyTranslucentState(pCmd);
            for(auto i=0u; i<range.Count; ++i)
            {
                m_MeshletSpheres[i + range.Offset].Draw(pCmd);
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
void SampleApp::DrawMeshletsManualControl
(
    ID3D12GraphicsCommandList6*     pCmd,
    uint32_t                        instanceId
)
{
    if (!Contains(m_MeshSphere, m_MainFrustumPlanes))
    { return; }

    pCmd->SetGraphicsRootSignature(m_RootSigMS.GetPtr());
    if (m_EnableWireframe)
    {
        m_ManualWireframeStateMS.SetState(pCmd);
    }
    else
    {
        m_ManualPipelineStateMS.SetState(pCmd);
    }
    pCmd->SetGraphicsRootConstantBufferView(ROOT_B1, m_TransformBuffer.GetResource()->GetGPUVirtualAddress());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T0, m_PositionBuffer    .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T1, m_NormalBuffer      .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T2, m_TexCoordBuffer    .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T3, m_PrimitiveBuffer   .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T4, m_VertexIndexBuffer .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T5, m_MeshletBuffer     .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T6, m_InstanceBuffer    .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T7, m_LodRangeBuffer    .GetSRV()->GetHandleGPU());

    struct Constants
    {
        uint32_t LodIndex;
        uint32_t Reserved;
        uint32_t InstanceId;
        uint32_t Flags;
    } constants = {};

    for(size_t i=0; i<m_Subsets.size(); ++i)
    {
        // LOD数を超える場合は描画しない.
        if (uint32_t(m_CurrLodIndex) >= m_Subsets[i].LodRangeCount)
            continue;

        // 範囲チェック.
        auto lodIndex = m_Subsets[i].LodRangeOffset + m_CurrLodIndex;
        if (lodIndex >= m_LodRanges.size())
            continue;

        const auto& range = m_LodRanges[lodIndex];
       
        constants.LodIndex        = lodIndex;
        constants.InstanceId      = instanceId;
        constants.Flags           = 0;

        constants.Flags |= (m_ColorMode & 0xF) << 1;

        uint32_t dispatchCount = (range.Count + 31) / 32;
        pCmd->SetGraphicsRoot32BitConstants(ROOT_B0, 4, &constants, 0);
        pCmd->DispatchMesh(dispatchCount, 1, 1);
    }
}

//-----------------------------------------------------------------------------
//      メッシュレットを描画します.
//-----------------------------------------------------------------------------
void SampleApp::DrawMeshletsAutoControl
(
    ID3D12GraphicsCommandList6*     pCmd,
    uint32_t                        instanceId
)
{
    if (!Contains(m_MeshSphere, m_MainFrustumPlanes))
    { return; }

    struct Constants
    {
        uint32_t MeshletOffset;
        uint32_t MeshletCount;
        uint32_t InstanceId;
        uint32_t Flags;
    } constants = {};

    pCmd->SetGraphicsRootSignature(m_RootSigMS.GetPtr());
    if (m_EnableWireframe)
    {
        m_AutoWireframeStateMS.SetState(pCmd);
    }
    else
    {
        m_AutoPipelineStateMS.SetState(pCmd);
    }
    pCmd->SetGraphicsRootConstantBufferView(ROOT_B1, m_TransformBuffer.GetResource()->GetGPUVirtualAddress());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T0, m_PositionBuffer    .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T1, m_NormalBuffer      .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T2, m_TexCoordBuffer    .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T3, m_PrimitiveBuffer   .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T4, m_VertexIndexBuffer .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T5, m_MeshletBuffer     .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T6, m_InstanceBuffer    .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_T7, m_LodRangeBuffer    .GetSRV()->GetHandleGPU());
    pCmd->SetGraphicsRootDescriptorTable(ROOT_U0, m_DebugWriteBuffer  .GetUAV()->GetHandleGPU());

    for(size_t i=0; i<m_Subsets.size(); ++i)
    {
        constants.MeshletOffset   = m_Subsets[i].MeshletOffset;
        constants.MeshletCount    = m_Subsets[i].MeshletCount;
        constants.InstanceId      = instanceId;
        constants.Flags           = 0;

        constants.Flags |= (m_ColorMode & 0xF) << 1; 

        uint32_t dispatchCount = (constants.MeshletCount + 31) / 32;

        pCmd->SetGraphicsRoot32BitConstants(ROOT_B0, 4, &constants, 0);
        pCmd->DispatchMesh(dispatchCount, 1, 1);
    }
}
