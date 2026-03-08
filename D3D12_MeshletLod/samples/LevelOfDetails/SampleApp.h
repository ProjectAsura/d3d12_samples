//-----------------------------------------------------------------------------
// File : SampleApp.h
// Desc : Sample Application.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <array>
#include <fw/asdxApp.h>
#include <fw/asdxAppCamera.h>
#include <gfx/asdxCommandQueue.h>
#include <gfx/asdxBuffer.h>
#include <gfx/asdxPipelineState.h>
#include <gfx/asdxShape.h>
#include <gfx/asdxTarget.h>
#include <LodGenerator.h>


///////////////////////////////////////////////////////////////////////////////
// MyMeshlet structure
///////////////////////////////////////////////////////////////////////////////
struct MyMeshlet
{
    uint32_t        VertexOffset;
    uint32_t        VertexCount;
    uint32_t        PrimitiveOffset;
    uint32_t        PrimitiveCount;
    uint8_t4        NormalCone;
    asdx::Vector4   BoundingSphere;
    uint32_t        MaterialId;
    uint32_t        Lod;
    float           GroupError;
    float           ParentError;
    asdx::Vector4   GroupBoundingSphere;
    asdx::Vector4   ParentBoundingSphere;
};

///////////////////////////////////////////////////////////////////////////////
// SampleApp class
///////////////////////////////////////////////////////////////////////////////
class SampleApp : public asdx::Application
{
public:
    SampleApp();
    ~SampleApp();

private:
    asdx::WaitPoint     m_WaitPoint;
    asdx::AppCamera     m_Camera;
    asdx::AppCamera     m_DebugCamera;

    asdx::StructuredBuffer  m_PositionBuffer;
    asdx::StructuredBuffer  m_NormalBuffer;
    asdx::StructuredBuffer  m_TexCoordBuffer;
    asdx::StructuredBuffer  m_VertexIndexBuffer;
    asdx::StructuredBuffer  m_PrimitiveBuffer;
    asdx::StructuredBuffer  m_MeshletBuffer;
    asdx::StructuredBuffer  m_InstanceBuffer;
    asdx::StructuredBuffer  m_LodRangeBuffer;
    asdx::ConstantBuffer    m_TransformBuffer;

    asdx::RefPtr<ID3D12RootSignature>   m_RootSigMS;
    asdx::PipelineState                 m_ManualPipelineStateMS;
    asdx::PipelineState                 m_ManualWireframeStateMS;
    asdx::PipelineState                 m_AutoPipelineStateMS;
    asdx::PipelineState                 m_AutoWireframeStateMS;

    asdx::RefPtr<ID3D12RootSignature>   m_RootSigVS;
    asdx::PipelineState                 m_PipelineStateVS;

    asdx::ShapeStates               m_ShapeStates;
    asdx::SphereShape               m_MeshSphereShape;
    asdx::BoxShape                  m_FrustumShape;
    std::vector<asdx::SphereShape>  m_MeshletSpheres;
    asdx::Vector4                   m_MeshSphere;
    std::vector<LodMeshletInfo>     m_Meshlets;
    std::vector<ResLodRange>        m_LodRanges;
    std::vector<ResLodSubset>       m_Subsets;

    std::array<asdx::Vector4, 6> m_MainFrustumPlanes;
    std::array<asdx::Vector4, 6> m_DebugFrustumPlanes;

    asdx::ComputeTarget         m_DebugWriteBuffer;

    int m_ColorMode             = 0;
    int m_CurrLodIndex          = 0;
    int m_LodMode               = 0;
    bool m_EnableWireframe      = false;
    bool m_DrawMeshletSphere    = false;
    int m_MaxLodLevel           = 0;

    bool OnInit() override;
    void OnTerm() override;
    void OnFrameMove(asdx::FrameEventArgs& args) override;
    void OnFrameRender(asdx::FrameEventArgs& args) override;
    void OnResize(const asdx::ResizeEventArgs& args) override;
    void OnKey(const asdx::KeyEventArgs& args) override;
    void OnMouse(const asdx::MouseEventArgs& args) override;
    void OnTyping(uint32_t keyCode) override;

    void DrawMeshletsManualControl(ID3D12GraphicsCommandList6* pCmd, uint32_t instanceId);
    void DrawMeshletsAutoControl(ID3D12GraphicsCommandList6* pCmd, uint32_t instanceId);
};