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
#include <Meshlet.h>

#define ENABLE_VERTEX_SHADER (0)

///////////////////////////////////////////////////////////////////////////////
// SampleApp class
///////////////////////////////////////////////////////////////////////////////
class SampleApp : public asdx::Application
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
    // pubilc methods.
    //=========================================================================
    SampleApp();
    ~SampleApp();

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    asdx::WaitPoint                     m_WaitPoint;
    asdx::AppCamera                     m_Camera;
    asdx::AppCamera                     m_DebugCamera;
    asdx::StructuredBuffer              m_PositionBuffer;
    asdx::StructuredBuffer              m_NormalBuffer;
    asdx::StructuredBuffer              m_TexCoordBuffer;
    asdx::ByteAddressBuffer             m_PrimitiveBuffer;
    asdx::StructuredBuffer              m_VertexIndexBuffer;
    asdx::StructuredBuffer              m_MeshletBuffer;
    asdx::StructuredBuffer              m_MeshInstanceBuffer;
    asdx::ConstantBuffer                m_TransformBuffer;
    std::vector<MeshletInfo>            m_MeshletInfos;

    asdx::RefPtr<ID3D12RootSignature>   m_RootSigMS;
    asdx::PipelineState                 m_PipelineStateMS;

    asdx::RefPtr<ID3D12RootSignature>   m_RootSigVS;
    asdx::PipelineState                 m_PipelineStateVS;

    asdx::VertexBuffer                  m_FrsutumVB;
    asdx::IndexBuffer                   m_FrustumIB;
    uint32_t                            m_FrustumIndexCount = 0;

    asdx::VertexBuffer                  m_DebugFrsutumVB;
    asdx::IndexBuffer                   m_DebugFrustumIB;
    uint32_t                            m_DebugFrustumIndexCount = 0;

    asdx::ShapeStates                   m_ShapeStates;
    asdx::SphereShape                   m_MeshSphereShape;
    asdx::BoxShape                      m_FrustumShape;
    std::vector<asdx::SphereShape>      m_MeshletSpheres;
    asdx::Vector4                       m_MeshSphere;

    std::array<asdx::Vector4, 6>        m_MainFrustumPlanes;
    std::array<asdx::Vector4, 6>        m_DebugFrustumPlanes;

    bool m_CullFromMain      = false;
    bool m_CullFromDebug     = false;

    bool m_EnableShading     = false;
    bool m_EnableDebugCamera = false;
    bool m_EnableSwapView    = false;
    bool m_DrawMeshSphere    = false;
    bool m_DrawMeshletSphere = false;
    bool m_DrawFrustum       = false;


    //=========================================================================
    // private methods.
    //=========================================================================
    bool OnInit() override;
    void OnTerm() override;
    void OnFrameMove(asdx::FrameEventArgs& args) override;
    void OnFrameRender(asdx::FrameEventArgs& args) override;
    void OnResize(const asdx::ResizeEventArgs& args) override;
    void OnKey(const asdx::KeyEventArgs& args) override;
    void OnMouse(const asdx::MouseEventArgs& args) override;
    void OnTyping(uint32_t keyCode) override;

    void DrawMeshlets(
        ID3D12GraphicsCommandList6* pCmd,
        const std::vector<MeshletInfo>& meshlets,
        uint32_t instanceId,
        bool debugView);

    void DrawFrsutum(
        ID3D12GraphicsCommandList6* pCmd,
        const asdx::Vector4& color);
};
