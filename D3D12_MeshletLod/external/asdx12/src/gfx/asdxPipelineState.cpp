//-----------------------------------------------------------------------------
// File : asdxPipelineState.cpp
// Desc : Pipeline State.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cassert>
#include <algorithm>
#include <d3d12shader.h>
#include <fnd/asdxLogger.h>
#include <fnd/asdxMisc.h>
#include <gfx/asdxPipelineState.h>
#include <gfx/asdxDevice.h>
#include <gfx/asdxShaderCompiler.h>


// ヘッダにあるとバグの原因となるので，ソース側に定義.
#ifdef __ID3D12GraphicsCommandList6_INTERFACE_DEFINED__
#define ASDX_ENABLE_MESH_SHADER
#endif//__ID3D12GraphicsCommandList6_INTERFACE_DEFINED__

namespace {

namespace internal {
#include "../res/shaders/Compiled/SpriteVS.inc"
#include "../res/shaders/Compiled/SpritePS.inc"
#include "../res/shaders/Compiled/FullScreenVS.inc"
#include "../res/shaders/Compiled/CopyPS.inc"
} // namespace internal

///////////////////////////////////////////////////////////////////////////////
// ROOT_PARAM_TYPE
///////////////////////////////////////////////////////////////////////////////
enum ROOT_PARAM_TYPE
{
    ROOT_PARAM_VAR = 0,
    ROOT_PARAM_CBV = 1,
    ROOT_PARAM_SRV = 2,
    ROOT_PARAM_UAV = 3,
    ROOT_PARAM_SMP = 4,
    ROOT_PARAM_AS  = 5,
};

///////////////////////////////////////////////////////////////////////////////
// PSSubObject class
///////////////////////////////////////////////////////////////////////////////
template<typename StructType, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type>
class alignas(void*) SubObject
{
public:
    SubObject() noexcept
    : m_Type    (Type)
    , m_Value   (StructType())
    { /* DO_NOTHING */ }

    SubObject(StructType const& value) noexcept
    : m_Type    (Type)
    , m_Value   (value)
    { /* DO_NOTHING */ }

    SubObject& operator = (StructType const& value) noexcept
    {
        m_Type  = Type;
        m_Value = value;
        return *this;
    }

    operator StructType const&() const noexcept 
    { return m_Value; }

    operator StructType&() noexcept 
    { return m_Value; }

    StructType* operator&() noexcept
    { return &m_Value; }

    StructType const* operator&() const noexcept
    { return &m_Value; }

private:
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_Type;
    StructType                          m_Value;
};

#define PSST(x) D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_##x

using PSS_ROOT_SIGNATURE = SubObject< ID3D12RootSignature*,        PSST(ROOT_SIGNATURE) >;
#ifdef ASDX_ENABLE_MESH_SHADER
using PSS_AS             = SubObject< D3D12_SHADER_BYTECODE,       PSST(AS) >;
using PSS_MS             = SubObject< D3D12_SHADER_BYTECODE,       PSST(MS) >;
#endif//ASDX_ENABLE_MESH_SHADER
using PSS_PS             = SubObject< D3D12_SHADER_BYTECODE,       PSST(PS) >;
using PSS_BLEND          = SubObject< D3D12_BLEND_DESC,            PSST(BLEND) >;
using PSS_SAMPLE_MASK    = SubObject< UINT,                        PSST(SAMPLE_MASK) >;
using PSS_RASTERIZER     = SubObject< D3D12_RASTERIZER_DESC,       PSST(RASTERIZER) >;
using PSS_DEPTH_STENCIL  = SubObject< D3D12_DEPTH_STENCIL_DESC,    PSST(DEPTH_STENCIL) >;
using PSS_RTV_FORMATS    = SubObject< D3D12_RT_FORMAT_ARRAY,       PSST(RENDER_TARGET_FORMATS) >;
using PSS_DSV_FORMAT     = SubObject< DXGI_FORMAT,                 PSST(DEPTH_STENCIL_FORMAT) >;
using PSS_SAMPLE_DESC    = SubObject< DXGI_SAMPLE_DESC,            PSST(SAMPLE_DESC) >;
using PSS_NODE_MASK      = SubObject< UINT,                        PSST(NODE_MASK) >;
using PSS_CACHED_PSO     = SubObject< D3D12_CACHED_PIPELINE_STATE, PSST(CACHED_PSO) >;
using PSS_FLAGS          = SubObject< D3D12_PIPELINE_STATE_FLAGS,  PSST(FLAGS) >;

#undef PSST


///////////////////////////////////////////////////////////////////////////////
// GPS_DESC structure
///////////////////////////////////////////////////////////////////////////////
struct GPS_DESC
{
    PSS_ROOT_SIGNATURE  RootSignature;
#ifdef ASDX_ENABLE_MESH_SHADER
    PSS_AS              AS;
    PSS_MS              MS;
#endif//ASDX_ENABLE_MESH_SHADER
    PSS_PS              PS;
    PSS_BLEND           BlendState;
    PSS_SAMPLE_MASK     SampleMask;
    PSS_RASTERIZER      RasterizerState;
    PSS_DEPTH_STENCIL   DepthStencilState;
    PSS_RTV_FORMATS     RTVFormats;
    PSS_DSV_FORMAT      DSVFormat;
    PSS_SAMPLE_DESC     SampleDesc;
    PSS_NODE_MASK       NodeMask;
    PSS_CACHED_PSO      CachedPSO;
    PSS_FLAGS           Flags;

    GPS_DESC()
    { /* DO_NOTHING */ }

    GPS_DESC(const asdx::GEOMETRY_PIPELINE_STATE_DESC* pValue)
    {
        RootSignature       = pValue->pRootSignature;
    #ifdef ASDX_ENABLE_MESH_SHADER
        AS                  = pValue->AS;
        MS                  = pValue->MS;
    #endif
        PS                  = pValue->PS;
        BlendState          = pValue->BlendState;
        SampleMask          = pValue->SampleMask;
        RasterizerState     = pValue->RasterizerState;
        DepthStencilState   = pValue->DepthStencilState;
        RTVFormats          = pValue->RTVFormats;
        DSVFormat           = pValue->DSVFormat;
        SampleDesc          = pValue->SampleDesc;
        NodeMask            = pValue->NodeMask;
        CachedPSO           = pValue->CachedPSO;
        Flags               = pValue->Flags;
    }
};

//-----------------------------------------------------------------------------
//      DynamicResourcesをサポートしているかどうかチェックします.
//-----------------------------------------------------------------------------
bool CheckSupportDynamicResources(ID3D12Device8* pDevice)
{
    // D3D12_RESOURCE_BINDING_TIER3 と D3D_SHADER_MODEL_6_6 以上であることが必須.
    // また、シェーダコンパイルする側で
    // D3D_SHADER_REQUIRES_RESOURCE_HEAP_INDEXING(0x02000000)
    // D3D_SHADER_REQUIRES_SAMPLER_HEAP_INDEXING(0x04000000)
    // のフラグを設定しておく必要がある.

    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    if (FAILED(pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options))))
    { return false; }

    if (options.ResourceBindingTier != D3D12_RESOURCE_BINDING_TIER_3)
    { return false; }

    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {};
    if (FAILED(pDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))))
    { return false; }

    if (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_6)
    { return false; }

    return true;
}

} // namespace


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// Preset class
///////////////////////////////////////////////////////////////////////////////
const D3D12_RASTERIZER_DESC Preset::CullNone = {
    D3D12_FILL_MODE_SOLID,
    D3D12_CULL_MODE_NONE,
    FALSE,
    D3D12_DEFAULT_DEPTH_BIAS,
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
    D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
    TRUE,
    FALSE,
    FALSE,
    0,
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
};
const D3D12_RASTERIZER_DESC Preset::CullBack = {
    D3D12_FILL_MODE_SOLID,
    D3D12_CULL_MODE_BACK,
    FALSE,
    D3D12_DEFAULT_DEPTH_BIAS,
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
    D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
    TRUE,
    FALSE,
    FALSE,
    0,
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
};
const D3D12_RASTERIZER_DESC Preset::CullFront = {
    D3D12_FILL_MODE_SOLID,
    D3D12_CULL_MODE_FRONT,
    FALSE,
    D3D12_DEFAULT_DEPTH_BIAS,
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
    D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
    TRUE,
    FALSE,
    FALSE,
    0,
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
};
const D3D12_RASTERIZER_DESC Preset::Wireframe = {
    D3D12_FILL_MODE_WIREFRAME,
    D3D12_CULL_MODE_NONE,
    FALSE,
    D3D12_DEFAULT_DEPTH_BIAS,
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
    D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
    TRUE,
    FALSE,
    FALSE,
    0,
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
};

const D3D12_DEPTH_STENCILOP_DESC Preset::StencilDefault = {
    D3D12_STENCIL_OP_KEEP,
    D3D12_STENCIL_OP_KEEP,
    D3D12_STENCIL_OP_KEEP,
    D3D12_COMPARISON_FUNC_ALWAYS
};

const D3D12_DEPTH_STENCIL_DESC Preset::DepthDefault = {
    TRUE,
    D3D12_DEPTH_WRITE_MASK_ALL,
    D3D12_COMPARISON_FUNC_LESS_EQUAL,
    FALSE,
    D3D12_DEFAULT_STENCIL_READ_MASK,
    D3D12_DEFAULT_STENCIL_WRITE_MASK,
    StencilDefault,
    StencilDefault
};
const D3D12_DEPTH_STENCIL_DESC Preset::DepthNone = {
    FALSE,
    D3D12_DEPTH_WRITE_MASK_ZERO,
    D3D12_COMPARISON_FUNC_ALWAYS,
    FALSE,
    D3D12_DEFAULT_STENCIL_READ_MASK,
    D3D12_DEFAULT_STENCIL_WRITE_MASK,
    StencilDefault,
    StencilDefault
};
const D3D12_DEPTH_STENCIL_DESC Preset::DepthReadOnly = {
    TRUE,
    D3D12_DEPTH_WRITE_MASK_ZERO,
    D3D12_COMPARISON_FUNC_LESS_EQUAL,
    FALSE,
    D3D12_DEFAULT_STENCIL_READ_MASK,
    D3D12_DEFAULT_STENCIL_WRITE_MASK,
    StencilDefault,
    StencilDefault
};
const D3D12_DEPTH_STENCIL_DESC Preset::DepthWriteOnly = {
    FALSE,
    D3D12_DEPTH_WRITE_MASK_ALL,
    D3D12_COMPARISON_FUNC_LESS_EQUAL,
    FALSE,
    D3D12_DEFAULT_STENCIL_READ_MASK,
    D3D12_DEFAULT_STENCIL_WRITE_MASK,
    StencilDefault,
    StencilDefault
};

const D3D12_RENDER_TARGET_BLEND_DESC Preset::RTB_Opaque = {
    FALSE,
    FALSE,
    D3D12_BLEND_ONE,
    D3D12_BLEND_ZERO,
    D3D12_BLEND_OP_ADD,
    D3D12_BLEND_ONE,
    D3D12_BLEND_ZERO,
    D3D12_BLEND_OP_ADD,
    D3D12_LOGIC_OP_NOOP,
    D3D12_COLOR_WRITE_ENABLE_ALL
};
const D3D12_RENDER_TARGET_BLEND_DESC Preset::RTB_AlphaBlend = {
    TRUE,
    FALSE,
    D3D12_BLEND_SRC_ALPHA,
    D3D12_BLEND_INV_SRC_ALPHA,
    D3D12_BLEND_OP_ADD,
    D3D12_BLEND_SRC_ALPHA,
    D3D12_BLEND_INV_SRC_ALPHA,
    D3D12_BLEND_OP_ADD,
    D3D12_LOGIC_OP_NOOP,
    D3D12_COLOR_WRITE_ENABLE_ALL
};
const D3D12_RENDER_TARGET_BLEND_DESC Preset::RTB_Additive = {
    TRUE,
    FALSE,
    D3D12_BLEND_SRC_ALPHA,
    D3D12_BLEND_ONE,
    D3D12_BLEND_OP_ADD,
    D3D12_BLEND_SRC_ALPHA,
    D3D12_BLEND_ONE,
    D3D12_BLEND_OP_ADD,
    D3D12_LOGIC_OP_NOOP,
    D3D12_COLOR_WRITE_ENABLE_ALL
};
const D3D12_RENDER_TARGET_BLEND_DESC Preset::RTB_Subtract = {
    TRUE,
    FALSE,
    D3D12_BLEND_SRC_ALPHA,
    D3D12_BLEND_ONE,
    D3D12_BLEND_OP_REV_SUBTRACT,
    D3D12_BLEND_SRC_ALPHA,
    D3D12_BLEND_ONE,
    D3D12_BLEND_OP_REV_SUBTRACT,
    D3D12_LOGIC_OP_NOOP,
    D3D12_COLOR_WRITE_ENABLE_ALL
};
const D3D12_RENDER_TARGET_BLEND_DESC Preset::RTB_Premultiplied = {
    TRUE,
    FALSE,
    D3D12_BLEND_ONE,
    D3D12_BLEND_INV_SRC_ALPHA,
    D3D12_BLEND_OP_ADD,
    D3D12_BLEND_ONE,
    D3D12_BLEND_INV_SRC_ALPHA,
    D3D12_BLEND_OP_ADD,
    D3D12_LOGIC_OP_NOOP,
    D3D12_COLOR_WRITE_ENABLE_ALL
};
const D3D12_RENDER_TARGET_BLEND_DESC Preset::RTB_Multiply = {
    TRUE,
    FALSE,
    D3D12_BLEND_ZERO,
    D3D12_BLEND_SRC_COLOR,
    D3D12_BLEND_OP_ADD,
    D3D12_BLEND_ZERO,
    D3D12_BLEND_SRC_ALPHA,
    D3D12_BLEND_OP_ADD,
    D3D12_LOGIC_OP_NOOP,
    D3D12_COLOR_WRITE_ENABLE_ALL
};

const D3D12_RENDER_TARGET_BLEND_DESC Preset::RTB_Screen = {
    TRUE,
    FALSE,
    D3D12_BLEND_DEST_COLOR,
    D3D12_BLEND_ONE,
    D3D12_BLEND_OP_ADD,
    D3D12_BLEND_DEST_ALPHA,
    D3D12_BLEND_ONE,
    D3D12_BLEND_OP_ADD,
    D3D12_LOGIC_OP_NOOP,
    D3D12_COLOR_WRITE_ENABLE_ALL
};

const D3D12_BLEND_DESC Preset::Opaque = {
    FALSE,
    FALSE,
    { RTB_Opaque, RTB_Opaque, RTB_Opaque, RTB_Opaque, RTB_Opaque, RTB_Opaque, RTB_Opaque, RTB_Opaque }
};
const D3D12_BLEND_DESC Preset::AlphaBlend = {
    FALSE,
    FALSE,
    { RTB_AlphaBlend, RTB_AlphaBlend, RTB_AlphaBlend, RTB_AlphaBlend, RTB_AlphaBlend, RTB_AlphaBlend, RTB_AlphaBlend, RTB_AlphaBlend }
};
const D3D12_BLEND_DESC Preset::Additive = {
    FALSE,
    FALSE,
    { RTB_Additive, RTB_Additive, RTB_Additive, RTB_Additive, RTB_Additive, RTB_Additive, RTB_Additive, RTB_Additive }
};
const D3D12_BLEND_DESC Preset::Subtract = {
    FALSE,
    FALSE,
    { RTB_Subtract, RTB_Subtract, RTB_Subtract, RTB_Subtract, RTB_Subtract, RTB_Subtract, RTB_Subtract, RTB_Subtract }
};
const D3D12_BLEND_DESC Preset::Premultiplied = {
    FALSE,
    FALSE,
    { RTB_Premultiplied, RTB_Premultiplied, RTB_Premultiplied, RTB_Premultiplied, RTB_Premultiplied, RTB_Premultiplied, RTB_Premultiplied, RTB_Premultiplied }
};
const D3D12_BLEND_DESC Preset::Multiply = {
    FALSE,
    FALSE,
    { RTB_Multiply, RTB_Multiply, RTB_Multiply, RTB_Multiply, RTB_Multiply, RTB_Multiply, RTB_Multiply, RTB_Multiply }
};
const D3D12_BLEND_DESC Preset::Screen = {
    FALSE,
    FALSE,
    { RTB_Screen, RTB_Screen, RTB_Screen, RTB_Screen, RTB_Screen, RTB_Screen, RTB_Screen, RTB_Screen }
};

const D3D12_SHADER_BYTECODE Preset::FullScreenVS = { internal::FullScreenVS, sizeof(internal::FullScreenVS) };
const D3D12_SHADER_BYTECODE Preset::CopyPS       = { internal::CopyPS,       sizeof(internal::CopyPS) };
const D3D12_SHADER_BYTECODE Preset::SpriteVS     = { internal::SpriteVS,     sizeof(internal::SpriteVS) };
const D3D12_SHADER_BYTECODE Preset::SpritePS     = { internal::SpritePS,     sizeof(internal::SpritePS) };


///////////////////////////////////////////////////////////////////////////////
// PipelineState class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
PipelineState::PipelineState()
: m_Type(PIPELINE_TYPE_GRAPHICS)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
PipelineState::~PipelineState()
{ Term(); }

//-----------------------------------------------------------------------------
//      グラフィックスパイプラインとして初期化します.
//-----------------------------------------------------------------------------
bool PipelineState::Init(ID3D12Device* pDevice, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc)
{
    if (pDevice == nullptr || pDesc == nullptr)
    {
        ELOG("Error : Inavlid Argument.");
        return false;
    }

    if (pDesc->pRootSignature == nullptr)
    {
        ELOG("Error : Invalid Argument.");
        return false;
    }

    m_VS.resize(pDesc->VS.BytecodeLength);
    memcpy(m_VS.data(), pDesc->VS.pShaderBytecode, m_VS.size());

    m_PS.resize(pDesc->PS.BytecodeLength);
    memcpy(m_PS.data(), pDesc->PS.pShaderBytecode, m_PS.size());

    m_Type = PIPELINE_TYPE_GRAPHICS;
    m_Desc.Graphics                     = *pDesc;
    m_Desc.Graphics.VS.pShaderBytecode  = m_VS.data();
    m_Desc.Graphics.PS.pShaderBytecode  = m_PS.data();

    // パイプラインステート生成.
    {
        auto hr = pDevice->CreateGraphicsPipelineState(&m_Desc.Graphics, IID_PPV_ARGS(m_DefaultPSO.GetAddress()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. errcode = 0x%x", hr);
            return false;
        }

        m_DefaultPSO->SetName(L"asdxGraphicsPipelineState");
    }

    return true;
}

//-----------------------------------------------------------------------------
//      コンピュートパイプラインとして初期化します.
//-----------------------------------------------------------------------------
bool PipelineState::Init(ID3D12Device* pDevice, const D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc)
{
    if (pDesc == nullptr)
    {
        ELOG("Error : Invalid Argument.");
        return false;
    }

    if (pDesc->pRootSignature == nullptr)
    {
        ELOG("Error : Invalid Argument.");
        return false;
    }

    m_CS.resize(pDesc->CS.BytecodeLength);
    memcpy(m_CS.data(), pDesc->CS.pShaderBytecode, m_CS.size());

    m_Type = PIPELINE_TYPE_COMPUTE;
    m_Desc.Compute                      = *pDesc;
    m_Desc.Compute.CS.pShaderBytecode   = m_CS.data();

    // パイプラインステート生成.
    {
        auto hr = pDevice->CreateComputePipelineState(&m_Desc.Compute, IID_PPV_ARGS(m_DefaultPSO.GetAddress()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateComputePipelineState() Failed. errcode = 0x%x", hr);
            return false;
        }

        m_DefaultPSO->SetName(L"asdxComputePipelineState");
    }

    return true;
}

//-----------------------------------------------------------------------------
//      ジオメトリパイプラインとして初期化します.
//-----------------------------------------------------------------------------
bool PipelineState::Init(ID3D12Device2* pDevice, const GEOMETRY_PIPELINE_STATE_DESC* pDesc)
{
    if (pDevice == nullptr || pDesc == nullptr)
    {
        ELOG("Error : Invalid Argument.");
        return false;
    }

    if (pDesc->pRootSignature == nullptr)
    {
        ELOG("Error : Invalid Argument.");
        return false;
    }

#ifdef ASDX_ENABLE_MESH_SHADER
    // シェーダモデルをチェック.
    {
        D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_5 };
        auto hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
        if (FAILED(hr) || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_5))
        {
            ELOG("Error : Shader Model 6.5 is not supported.");
            return false;
        }
    }

    // メッシュシェーダをサポートしているかどうかチェック.
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 features = {};
        auto hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &features, sizeof(features));
        if (FAILED(hr) || (features.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED))
        {
            ELOG("Error : Mesh Shaders aren't supported.");
            return false;
        }
    }

    m_MS.resize(pDesc->MS.BytecodeLength);
    memcpy(m_MS.data(), pDesc->MS.pShaderBytecode, m_MS.size());

    if (pDesc->AS.BytecodeLength > 0)
    {
        m_AS.resize(pDesc->AS.BytecodeLength);
        memcpy(m_AS.data(), pDesc->AS.pShaderBytecode, m_AS.size());
    }
    else
    {
        m_AS.clear();
    }

    m_PS.resize(pDesc->PS.BytecodeLength);
    memcpy(m_PS.data(), pDesc->PS.pShaderBytecode, m_PS.size());

    m_Type = PIPELINE_TYPE_GEOMETRY;
    m_Desc.Geometry = *pDesc;
    m_Desc.Geometry.MS.pShaderBytecode = m_MS.data();
    m_Desc.Geometry.PS.pShaderBytecode = m_PS.data();

    if (pDesc->AS.BytecodeLength > 0)
    { m_Desc.Geometry.AS.pShaderBytecode = m_AS.data(); }

    // ジオメトリパイプラインステートを生成.
    {
        GPS_DESC gpsDesc(&m_Desc.Geometry);

        D3D12_PIPELINE_STATE_STREAM_DESC pssDesc = {};
        pssDesc.SizeInBytes = sizeof(gpsDesc);
        pssDesc.pPipelineStateSubobjectStream = &gpsDesc;

        // パイプラインステート生成.
        auto hr = pDevice->CreatePipelineState(&pssDesc, IID_PPV_ARGS(m_DefaultPSO.GetAddress()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. errcode = 0x%x", hr);
            return false;
        }

        m_DefaultPSO->SetName(L"asdxGeometryPipelineState");
    }

    return true;
#else
    ELOG("Error : Not Support Geometry Pipeline.");
    return false;
#endif
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void PipelineState::Term()
{
    m_DefaultPSO .Reset();
    m_ReloadedPSO.Reset();

    m_ReloadPathVS.clear();
    m_ReloadPathPS.clear();
    m_ReloadPathCS.clear();
    m_ReloadPathAS.clear();
    m_ReloadPathMS.clear();

    m_ShaderModelVS.clear();
    m_ShaderModelPS.clear();
    m_ShaderModelCS.clear();
    m_ShaderModelAS.clear();
    m_ShaderModelMS.clear();

    m_VS.clear();
    m_PS.clear();
    m_CS.clear();
    m_MS.clear();
    m_AS.clear();
}

//-----------------------------------------------------------------------------
//      パイプラインステートを設定します.
//-----------------------------------------------------------------------------
void PipelineState::SetState(ID3D12GraphicsCommandList* pCmdList)
{
    Rebuild();

    auto pso = (m_ReloadedPSO.GetPtr() != nullptr) 
        ? m_ReloadedPSO.GetPtr()
        : m_DefaultPSO.GetPtr();
    pCmdList->SetPipelineState(pso);
}

//-----------------------------------------------------------------------------
//      パイプラインタイプを取得します.
//-----------------------------------------------------------------------------
PIPELINE_TYPE PipelineState::GetType() const
{ return m_Type; }

//-----------------------------------------------------------------------------
//      頂点シェーダのリロードパスを設定します.
//-----------------------------------------------------------------------------
void PipelineState::SetReloadPathVS(const char* path, const char* shaderModel)
{
    m_ReloadPathVS  = ToFullPathA(path);
    m_ShaderModelVS = shaderModel;
}

//-----------------------------------------------------------------------------
//      ピクセルシェーダのリロードパスを設定します.
//-----------------------------------------------------------------------------
void PipelineState::SetReloadPathPS(const char* path, const char* shaderModel)
{
    m_ReloadPathPS  = ToFullPathA(path);
    m_ShaderModelPS = shaderModel;
}

//-----------------------------------------------------------------------------
//      コンピュートシェーダのリロードパスを設定します.
//-----------------------------------------------------------------------------
void PipelineState::SetReloadPathCS(const char* path, const char* shaderModel)
{
    m_ReloadPathCS  = ToFullPathA(path);
    m_ShaderModelCS = shaderModel;
}

//-----------------------------------------------------------------------------
//      増幅シェーダのリロードパスを設定します.
//-----------------------------------------------------------------------------
void PipelineState::SetReloadPathAS(const char* path, const char* shaderModel)
{
    m_ReloadPathAS  = ToFullPathA(path);
    m_ShaderModelAS = shaderModel;
}

//-----------------------------------------------------------------------------
//      メッシュシェーダのリロードパスを設定します.
//-----------------------------------------------------------------------------
void PipelineState::SetReloadPathMS(const char* path, const char* shaderModel)
{
    m_ReloadPathMS  = ToFullPathA(path);
    m_ShaderModelMS = shaderModel;
}

//-----------------------------------------------------------------------------
//      インクルードディレクトリを設定します.
//-----------------------------------------------------------------------------
void PipelineState::SetIncludeDirs(const std::vector<std::string>& dirs)
{ m_IncludeDirs = dirs; }

//-----------------------------------------------------------------------------
//      依存ファイル名を設定します.
//-----------------------------------------------------------------------------
void PipelineState::SetDependencies(const std::vector<std::string>& dependencies)
{
    m_Dependencies.resize(dependencies.size());
    for(auto i=0; i<dependencies.size(); ++i)
    { m_Dependencies[i] = ToFullPathA(dependencies[i].c_str()); }
}

//-----------------------------------------------------------------------------
//      ファイル更新時の処理です.
//-----------------------------------------------------------------------------
void PipelineState::OnUpdate(const FileUpdateEventArgs& args)
{
    std::string path = args.DirectoryPath;
    path += "/";
    path += args.RelativePath;

    path = ToFullPathA(path.c_str());

    switch(args.Type)
    {
    case ACTION_MODIFIED:
    case ACTION_RENAMED_NEW_NAME:
        {
            if (!m_ReloadPathVS.empty() && m_ReloadPathVS == path)
            { m_Dirty = true; }

            if (!m_ReloadPathPS.empty() && m_ReloadPathPS == path)
            { m_Dirty = true; }

            if (!m_ReloadPathCS.empty() && m_ReloadPathCS == path)
            { m_Dirty = true; }

            if (!m_ReloadPathAS.empty() && m_ReloadPathAS == path)
            { m_Dirty = true; }

            if (!m_ReloadPathMS.empty() && m_ReloadPathMS == path)
            { m_Dirty = true; }

            for(auto& dep : m_Dependencies)
            {
                if (!dep.empty() && dep == path)
                {
                    m_Dirty = true;
                    break;
                }
            }
        }
        break;

    default:
        break;
    }
}

//-----------------------------------------------------------------------------
//      シェーダをリロードします.
//-----------------------------------------------------------------------------
bool PipelineState::ReloadShader
(
    const char*             path,
    const char*             shaderModel,
    std::vector<uint8_t>&   result
)
{
    RefPtr<IBlob> blob;
    // シェーダコンパイル.
    if (!CompileFromFileA(path, m_IncludeDirs, "main", shaderModel, blob.GetAddress()))
    { return false; }

    result.clear();
    result.resize(blob->GetBufferSize());
    memcpy(result.data(), blob->GetBufferPointer(), blob->GetBufferSize());

    return true;
}

//-----------------------------------------------------------------------------
//      パイプラインステートを再生成します.
//-----------------------------------------------------------------------------
void PipelineState::Rebuild()
{
    if (!m_Dirty)
    { return; }

    m_Dirty = false;

    // シェーダリロード.
    bool reloaded = false;
    {
        // 頂点シェーダ.
        if (!m_ReloadPathVS.empty())
        { reloaded = ReloadShader(m_ReloadPathVS.c_str(), m_ShaderModelVS.c_str(), m_VS); }

        // ピクセルシェーダ.
        if (!m_ReloadPathPS.empty())
        { reloaded = ReloadShader(m_ReloadPathPS.c_str(), m_ShaderModelPS.c_str(), m_PS); }

        // コンピュートシェーダ.
        if (!m_ReloadPathCS.empty())
        { reloaded = ReloadShader(m_ReloadPathCS.c_str(), m_ShaderModelCS.c_str(), m_CS); }

        // 増幅シェーダ.
        if (!m_ReloadPathAS.empty())
        { reloaded = ReloadShader(m_ReloadPathAS.c_str(), m_ShaderModelAS.c_str(), m_AS); }

        // メッシュシェーダ.
        if (!m_ReloadPathMS.empty())
        { reloaded = ReloadShader(m_ReloadPathMS.c_str(), m_ShaderModelMS.c_str(), m_MS); }
    }

    if (!reloaded)
        return;

    if (!m_ReloadedPSO.GetPtr())
    {
        auto pso = m_ReloadedPSO.Detach();
        Dispose(pso);
    }

    if (m_Type == PIPELINE_TYPE_GRAPHICS)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = m_Desc.Graphics;
        desc.VS.pShaderBytecode = m_VS.data();
        desc.VS.BytecodeLength  = m_VS.size();
        desc.PS.pShaderBytecode = m_PS.data();
        desc.PS.BytecodeLength  = m_PS.size();

        auto hr = GetD3D12Device()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_ReloadedPSO.GetAddress()));
        if (FAILED(hr))
        {
            ELOGA("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. errcode = 0x%x", hr);
            return;
        }

         m_ReloadedPSO->SetName(L"asdxGraphicsPipelineState_Reload");
         ILOGA("Info : Shader Reloaded! VS=%s(%s), PS=%s(%s)",
             m_ReloadPathVS.c_str(), m_ShaderModelVS.c_str(),
             m_ReloadPathPS.c_str(), m_ShaderModelPS.c_str());
    }
    else if (m_Type == PIPELINE_TYPE_COMPUTE)
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = m_Desc.Compute;
        desc.CS.pShaderBytecode = m_CS.data();
        desc.CS.BytecodeLength  = m_CS.size();

        auto hr = GetD3D12Device()->CreateComputePipelineState(&desc, IID_PPV_ARGS(m_ReloadedPSO.GetAddress()));
        if (FAILED(hr))
        {
            ELOGA("Error : ID3D12Device::CreateComputePipelineState() Failed. errcode = 0x%x", hr);
            return;
        }

        m_ReloadedPSO->SetName(L"asdxComputePipelineState_Reload");
        ILOGA("Info : Shader Reloaded! CS=%s(%s)", m_ReloadPathCS.c_str(), m_ShaderModelCS.c_str());
    }
    else
    {
        GEOMETRY_PIPELINE_STATE_DESC desc = m_Desc.Geometry;
        desc.AS.pShaderBytecode = m_AS.data();
        desc.AS.BytecodeLength  = m_AS.size();
        desc.MS.pShaderBytecode = m_MS.data();
        desc.MS.BytecodeLength  = m_MS.size();
        desc.PS.pShaderBytecode = m_PS.data();
        desc.PS.BytecodeLength  = m_PS.size();

        GPS_DESC gpsDesc(&desc);

        D3D12_PIPELINE_STATE_STREAM_DESC pssDesc = {};
        pssDesc.SizeInBytes = sizeof(gpsDesc);
        pssDesc.pPipelineStateSubobjectStream = &gpsDesc;

        // パイプラインステート生成.
        auto hr = GetD3D12Device()->CreatePipelineState(&pssDesc, IID_PPV_ARGS(m_ReloadedPSO.GetAddress()));
        if (FAILED(hr))
        {
            ELOGA("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. errcode = 0x%x", hr);
            return;
        }

        m_ReloadedPSO->SetName(L"asdxGeometryPipelineState_Reload");
        ILOGA("Info : Shader Reloaded! AS=%s(%s), MS=%s(%s), PS=%s(%s)",
            m_ReloadPathAS.c_str(), m_ShaderModelAS.c_str(),
            m_ReloadPathMS.c_str(), m_ShaderModelMS.c_str(),
            m_ReloadPathPS.c_str(), m_ShaderModelPS.c_str());
    }
}

//-----------------------------------------------------------------------------
//      SRVレンジとして初期化します.
//-----------------------------------------------------------------------------
void InitRangeAsSRV(D3D12_DESCRIPTOR_RANGE& range, UINT registerIndex, UINT count, UINT registerSpace)
{
    range.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors                    = count;
    range.BaseShaderRegister                = registerIndex;
    range.RegisterSpace                     = registerSpace;
    range.OffsetInDescriptorsFromTableStart = 0;
}

//-----------------------------------------------------------------------------
//      UAVレンジとして初期化します.
//-----------------------------------------------------------------------------
void InitRangeAsUAV(D3D12_DESCRIPTOR_RANGE& range, UINT registerIndex, UINT count, UINT registerSpace)
{
    range.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    range.NumDescriptors                    = count;
    range.BaseShaderRegister                = registerIndex;
    range.RegisterSpace                     = registerSpace;
    range.OffsetInDescriptorsFromTableStart = 0;
}

//-----------------------------------------------------------------------------
//      ルート定数として初期化します.
//-----------------------------------------------------------------------------
void InitAsConstants(D3D12_ROOT_PARAMETER& param, UINT registerIndex, UINT count, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace)
{
    param.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    param.Constants.Num32BitValues  = count;
    param.Constants.ShaderRegister  = registerIndex;
    param.Constants.RegisterSpace   = registerSpace;
    param.ShaderVisibility          = visibility;
}

//-----------------------------------------------------------------------------
//      定数バッファとして初期化します.
//-----------------------------------------------------------------------------
void InitAsCBV(D3D12_ROOT_PARAMETER& param, UINT registerIndex, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace)
{
    param.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.Descriptor.ShaderRegister = registerIndex;
    param.Descriptor.RegisterSpace  = registerSpace;
    param.ShaderVisibility          = visibility;
}

//-----------------------------------------------------------------------------
//      SRVとして初期化します.
//-----------------------------------------------------------------------------
void InitAsSRV(D3D12_ROOT_PARAMETER& param, UINT registerIndex, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace)
{
    param.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_SRV;
    param.Descriptor.ShaderRegister = registerIndex;
    param.Descriptor.RegisterSpace  = registerSpace;
    param.ShaderVisibility          = visibility;
}

//-----------------------------------------------------------------------------
//      UAVとして初期化します.
//-----------------------------------------------------------------------------
void InitAsUAV(D3D12_ROOT_PARAMETER& param, UINT registerIndex, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace)
{
    param.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_UAV;
    param.Descriptor.ShaderRegister = registerIndex;
    param.Descriptor.RegisterSpace  = registerSpace;
    param.ShaderVisibility          = visibility;
}

//-----------------------------------------------------------------------------
//      ディスクリプタテーブルとして初期化します.
//-----------------------------------------------------------------------------
void InitAsTable(
    D3D12_ROOT_PARAMETER&           param,
    UINT                            count,
    const D3D12_DESCRIPTOR_RANGE*   range,
    D3D12_SHADER_VISIBILITY         visibility
)
{
    param.ParameterType                         = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.DescriptorTable.NumDescriptorRanges   = 1;
    param.DescriptorTable.pDescriptorRanges     = range;
    param.ShaderVisibility                      = visibility;
}

//-----------------------------------------------------------------------------
//      ルートシグニチャを初期化します.
//-----------------------------------------------------------------------------
bool InitRootSignature
(
    ID3D12Device*                       pDevice,
    const D3D12_ROOT_SIGNATURE_DESC*    pDesc,
    ID3D12RootSignature**               ppRootSig
)
{
    asdx::RefPtr<ID3DBlob> blob;
    asdx::RefPtr<ID3DBlob> errorBlob;
    auto hr = D3D12SerializeRootSignature(
        pDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, blob.GetAddress(), errorBlob.GetAddress());
    if (FAILED(hr))
    {
        ELOG("Error : D3D12SerializeRootSignature() Failed. errcode = 0x%x", hr);
        if (errorBlob) {
            ELOG("Error : msg = %s", reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
        }
        return false;
    }

    hr = pDevice->CreateRootSignature(
        0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(ppRootSig));
    if (FAILED(hr))
    {
        ELOG("Error : ID3D12Device::CreateRootSignature() Failed. errcode = 0x%x", hr);
        return false;
    }

    return true;
}

} // namespace asdx
