//-----------------------------------------------------------------------------
// File : asdxRayTracing.cpp
// Desc : DXR Helper.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <fnd/asdxLogger.h>
#include <fnd/asdxMisc.h>
#include <gfx/asdxRayTracing.h>
#include <gfx/asdxDevice.h>
#include <gfx/asdxShaderCompiler.h>


namespace asdx {

//-----------------------------------------------------------------------------
//      DXRがサポートされているかどうかチェックします.
//-----------------------------------------------------------------------------
bool IsSupportDXR(ID3D12Device6* pDevice)
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options = {};
    auto hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options, sizeof(options));
    if (FAILED(hr))
    { return false; }

    return options.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}

//-----------------------------------------------------------------------------
//      バッファUAVを生成します.
//-----------------------------------------------------------------------------
bool CreateBufferUAV
(
    ID3D12Device*           pDevice,
    UINT64                  bufferSize,
    ID3D12Resource**        ppResource,
    D3D12_RESOURCE_STATES   initialResourceState
)
{
    D3D12_HEAP_PROPERTIES props = {};
    props.Type                  = D3D12_HEAP_TYPE_DEFAULT;
    props.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    props.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
    props.CreationNodeMask      = 1;
    props.VisibleNodeMask       = 1;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment          = 0;
    desc.Width              = bufferSize;
    desc.Height             = 1;
    desc.DepthOrArraySize   = 1;
    desc.MipLevels          = 1;
    desc.Format             = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    auto hr = pDevice->CreateCommittedResource(
        &props,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        initialResourceState,
        nullptr,
        IID_PPV_ARGS(ppResource));
    if (FAILED(hr))
    {
        ELOGA("Error : ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr);
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      バッファSRVを生成します.
//-----------------------------------------------------------------------------
bool CreateBufferSRV
(
    ID3D12Device*           pDevice,
    ID3D12Resource*         pResource,
    UINT                    elementCount,
    UINT                    elementSize,
    IShaderResourceView**   ppView
)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.ViewDimension           = D3D12_SRV_DIMENSION_BUFFER;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Buffer.NumElements      = elementCount;
    if (elementSize == 0)
    {
        desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
        desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_RAW;
        desc.Buffer.StructureByteStride = 0;
    }
    else
    {
        desc.Format                     = DXGI_FORMAT_UNKNOWN;
        desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
        desc.Buffer.StructureByteStride = elementSize;
    }

    return asdx::CreateShaderResourceView(pResource, &desc, ppView);
}

//-----------------------------------------------------------------------------
//      アップロードバッファを生成します.
//-----------------------------------------------------------------------------
bool CreateUploadBuffer
(
    ID3D12Device*           pDevice,
    UINT64                  bufferSize,
    ID3D12Resource**        ppResource
)
{
    D3D12_HEAP_PROPERTIES props = {};
    props.Type                  = D3D12_HEAP_TYPE_UPLOAD;
    props.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    props.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
    props.CreationNodeMask      = 1;
    props.VisibleNodeMask       = 1;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment          = 0;
    desc.Width              = bufferSize;
    desc.Height             = 1;
    desc.DepthOrArraySize   = 1;
    desc.MipLevels          = 1;
    desc.Format             = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    auto hr = pDevice->CreateCommittedResource(
        &props,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(ppResource));
    if (FAILED(hr))
    {
        ELOGA("Error : ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr);
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// AsScratchBuffer class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
AsScratchBuffer::~AsScratchBuffer()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool AsScratchBuffer::Init(ID3D12Device* pDevice, size_t size)
{ return CreateBufferUAV(pDevice, size, m_Scratch.GetAddress()); }

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void AsScratchBuffer::Term()
{
    auto buffer = m_Scratch.Detach();
    Dispose(buffer);
}

//-----------------------------------------------------------------------------
//      GPU仮想アドレスを取得します.
//-----------------------------------------------------------------------------
D3D12_GPU_VIRTUAL_ADDRESS AsScratchBuffer::GetGpuAddress() const
{ return m_Scratch->GetGPUVirtualAddress(); }

//-----------------------------------------------------------------------------
//      デバッグ名を設定します.
//-----------------------------------------------------------------------------
void AsScratchBuffer::SetName(LPCWSTR name)
{ m_Scratch->SetName(name); }


///////////////////////////////////////////////////////////////////////////////
// Blas class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
Blas::~Blas()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool Blas::Init
(
    ID3D12Device6*              pDevice,
    uint32_t                    count,
    const DXR_GEOMETRY_DESC*    pDescs,
    DXR_BUILD_FLAGS             flags
)
{
    if (pDevice == nullptr)
    { return false; }

    // 設定をコピっておく.
    m_GeometryDesc.resize(count);
    if (pDescs != nullptr)
    {
        for(auto i=0u; i<count; ++i)
        { m_GeometryDesc[i] = pDescs[i]; }
    }

    // 高速化機構の入力設定.
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.DescsLayout      = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.Flags            = flags;
    inputs.NumDescs         = count;
    inputs.pGeometryDescs   = m_GeometryDesc.data();
    inputs.Type             = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

    // ビルド前情報を取得.
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);
    if (prebuildInfo.ResultDataMaxSizeInBytes == 0)
    { return false; }

    // スクラッチバッファサイズを取得.
    m_ScratchBufferSize = Max(prebuildInfo.ScratchDataSizeInBytes, prebuildInfo.UpdateScratchDataSizeInBytes);

    // 高速化機構用バッファ生成.
    if (!CreateBufferUAV(
        pDevice,
        prebuildInfo.ResultDataMaxSizeInBytes,
        m_Structure.GetAddress(),
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE))
    {
        ELOGA("Error : CreateUAVBuffer() Failed.");
        return false;
    }

    // ビルド設定.
    memset(&m_BuildDesc, 0, sizeof(m_BuildDesc));
    m_BuildDesc.Inputs                          = inputs;
    m_BuildDesc.DestAccelerationStructureData   = m_Structure->GetGPUVirtualAddress();

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void Blas::Term()
{
    m_GeometryDesc.clear();
    auto sturcture = m_Structure.Detach();

    Dispose(sturcture);
    m_ScratchBufferSize = 0;
}

//-----------------------------------------------------------------------------
//      スクラッチバッファサイズを取得します.
//-----------------------------------------------------------------------------
size_t Blas::GetScratchBufferSize() const
{ return m_ScratchBufferSize; }

//-----------------------------------------------------------------------------
//      ジオメトリ数を取得します.
//-----------------------------------------------------------------------------
uint32_t Blas::GetGeometryCount() const
{ return uint32_t(m_GeometryDesc.size()); }

//-----------------------------------------------------------------------------
//      ジオメトリ構成を取得します.
//-----------------------------------------------------------------------------
const DXR_GEOMETRY_DESC& Blas::GetGeometry(uint32_t index) const
{
    assert(index < uint32_t(m_GeometryDesc.size()));
    return m_GeometryDesc[index];
}

//-----------------------------------------------------------------------------
//      ジオメトリ構成を設定します.
//-----------------------------------------------------------------------------
void Blas::SetGeometry(uint32_t index, const DXR_GEOMETRY_DESC& desc)
{
    assert(index < uint32_t(m_GeometryDesc.size()));
    m_GeometryDesc[index] = desc;
}

//-----------------------------------------------------------------------------
//      リソースを取得します.
//-----------------------------------------------------------------------------
ID3D12Resource* Blas::GetResource() const
{ return m_Structure.GetPtr(); }

//-----------------------------------------------------------------------------
//      デバッグ名を設定します.
//-----------------------------------------------------------------------------
void Blas::SetName(LPCWSTR name)
{ m_Structure->SetName(name); }

//-----------------------------------------------------------------------------
//      ビルドします.
//-----------------------------------------------------------------------------
void Blas::Build(ID3D12GraphicsCommandList4* pCmd, D3D12_GPU_VIRTUAL_ADDRESS scratchAddress)
{
    auto desc = m_BuildDesc;
    desc.ScratchAccelerationStructureData = scratchAddress;

    // 高速化機構を構築.
    pCmd->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);

    // バリアを張っておく.
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type            = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource   = m_Structure.GetPtr();
    pCmd->ResourceBarrier(1, &barrier);
}


///////////////////////////////////////////////////////////////////////////////
// Tlas class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
Tlas::~Tlas()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool Tlas::Init
(
    ID3D12Device6*              pDevice,
    uint32_t                    instanceDescCount,
    const DXR_INSTANCE_DESC*    pInstanceDescs,
    DXR_BUILD_FLAGS             flags
)
{
    // アップロードバッファ生成.
    if (!CreateUploadBuffer(
        pDevice, sizeof(DXR_INSTANCE_DESC) * instanceDescCount, 
        m_Instances.GetAddress()))
    {
        ELOGA("Error : CreateUploadBuffer() Failed.");
        return false;
    }

    // インスタンス設定をコピー.
    {
        DXR_INSTANCE_DESC* ptr = nullptr;
        auto hr = m_Instances->Map(0, nullptr, reinterpret_cast<void**>(&ptr));
        if (FAILED(hr))
        {
            ELOGA("Error : ID3D12Resource::Map() Failed. errcode = 0x%x", hr);
            return false;
        }

        memcpy(ptr, pInstanceDescs, sizeof(DXR_INSTANCE_DESC) * instanceDescCount);

        m_Instances->Unmap(0, nullptr);
    }

    // 高速化機構の入力設定.
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.DescsLayout      = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.Flags            = flags;
    inputs.NumDescs         = instanceDescCount;
    inputs.InstanceDescs    = m_Instances->GetGPUVirtualAddress();
    inputs.Type             = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    // ビルド前情報を取得.
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);
    if (prebuildInfo.ResultDataMaxSizeInBytes == 0)
    { return false; }

    // スクラッチバッファサイズを取得.
    m_ScratchBufferSize = Max(prebuildInfo.ScratchDataSizeInBytes, prebuildInfo.UpdateScratchDataSizeInBytes);

    // 高速化機構用バッファ生成.
    if (!CreateBufferUAV(
        pDevice,
        prebuildInfo.ResultDataMaxSizeInBytes,
        m_Structure.GetAddress(),
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE))
    {
        ELOGA("Error : CreateUAVBuffer() Failed.");
        return false;
    }

    // ビルド設定.
    memset(&m_BuildDesc, 0, sizeof(m_BuildDesc));
    m_BuildDesc.Inputs                          = inputs;
    m_BuildDesc.DestAccelerationStructureData   = m_Structure->GetGPUVirtualAddress();

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void Tlas::Term()
{
    auto instance  = m_Instances.Detach();
    auto structure = m_Structure.Detach();

    Dispose(instance);
    Dispose(structure);

    m_ScratchBufferSize = 0;
}

//-----------------------------------------------------------------------------
//      スクラッチバッファサイズを取得します.
//-----------------------------------------------------------------------------
size_t Tlas::GetScratchBufferSize() const 
{ return m_ScratchBufferSize; }

//-----------------------------------------------------------------------------
//      メモリマッピングを行います.
//-----------------------------------------------------------------------------
DXR_INSTANCE_DESC* Tlas::Map()
{
    DXR_INSTANCE_DESC* ptr = nullptr;
    auto hr = m_Instances->Map(0, nullptr, reinterpret_cast<void**>(&ptr));
    if (FAILED(hr))
    { return nullptr; }

    return ptr;
}

//-----------------------------------------------------------------------------
//      メモリマッピングを解除します.
//-----------------------------------------------------------------------------
void Tlas::Unmap()
{ m_Instances->Unmap(0, nullptr); }

//-----------------------------------------------------------------------------
//      リソースを取得します.
//-----------------------------------------------------------------------------
ID3D12Resource* Tlas::GetResource() const
{ return m_Structure.GetPtr(); }

//-----------------------------------------------------------------------------
//      デバッグ名を設定します.
//-----------------------------------------------------------------------------
void Tlas::SetName(LPCWSTR name)
{ m_Structure->SetName(name); }

//-----------------------------------------------------------------------------
//      ビルドします.
//-----------------------------------------------------------------------------
void Tlas::Build(ID3D12GraphicsCommandList4* pCmd, D3D12_GPU_VIRTUAL_ADDRESS scratchAddress)
{
    auto desc = m_BuildDesc;
    desc.ScratchAccelerationStructureData = scratchAddress;

    // 高速化機構を構築.
    pCmd->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);

    // バリアを張っておく.
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type            = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource   = m_Structure.GetPtr();
    pCmd->ResourceBarrier(1, &barrier);
}


///////////////////////////////////////////////////////////////////////////////
// RayTracingPipelineState class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
RayTracingPipelineState::RayTracingPipelineState()
: m_pDefaultObject (nullptr)
, m_pDefaultProps  (nullptr)
, m_pReloadObject  (nullptr)
, m_pReloadProps   (nullptr)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
RayTracingPipelineState::~RayTracingPipelineState()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool RayTracingPipelineState::Init(ID3D12Device5* pDevice, const RayTracingPipelineStateDesc& desc)
{
    uint32_t objCount = 5u + uint32_t(desc.HitGroups.size());

    m_Desc = desc;

    std::vector<D3D12_STATE_SUBOBJECT> objDesc;
    objDesc.resize(objCount);

    std::vector<D3D12_EXPORT_DESC> exports;
    {
        exports.push_back({ desc.RayGeneration.c_str(), nullptr, D3D12_EXPORT_FLAG_NONE });
        for(auto& hit : desc.HitGroups)
        {
            if (hit.AnyHitShaderImport)
            {
                exports.push_back({hit.AnyHitShaderImport, nullptr, D3D12_EXPORT_FLAG_NONE});
            }
            if (hit.ClosestHitShaderImport)
            {
                exports.push_back({hit.ClosestHitShaderImport, nullptr, D3D12_EXPORT_FLAG_NONE});
            }
            if (hit.IntersectionShaderImport)
            {
                exports.push_back({hit.IntersectionShaderImport, nullptr, D3D12_EXPORT_FLAG_NONE});
            }
        }
        for(auto& miss : desc.MissTable)
        {
            exports.push_back({miss.c_str(), nullptr, D3D12_EXPORT_FLAG_NONE});
        }
    }

    auto index = 0;

    D3D12_GLOBAL_ROOT_SIGNATURE globalRootSignature = {};
    globalRootSignature.pGlobalRootSignature = desc.pGlobalRootSignature;

    objDesc[index].Type  = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    objDesc[index].pDesc = &globalRootSignature;
    index++;

    D3D12_DXIL_LIBRARY_DESC libDesc = {};
    libDesc.DXILLibrary = desc.DXILLibrary;
    libDesc.NumExports  = UINT(exports.size());
    libDesc.pExports    = exports.data();

    objDesc[index].Type     = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    objDesc[index].pDesc    = &libDesc;
    index++;

    D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
    shaderConfig.MaxAttributeSizeInBytes = desc.MaxAttributeSize;
    shaderConfig.MaxPayloadSizeInBytes   = desc.MaxPayloadSize;

    objDesc[index].Type     = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    objDesc[index].pDesc    = &shaderConfig;
    index++;

    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
    pipelineConfig.MaxTraceRecursionDepth = desc.MaxTraceRecursionDepth;

    objDesc[index].Type     = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    objDesc[index].pDesc    = &pipelineConfig;
    index++;

    for(auto i=0u; i<desc.HitGroups.size(); ++i)
    {
        objDesc[index].Type  = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
        objDesc[index].pDesc = &desc.HitGroups[i];
        index++;
    }

    D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
    stateObjectDesc.Type            = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    stateObjectDesc.NumSubobjects   = index;
    stateObjectDesc.pSubobjects     = objDesc.data();

    auto hr = pDevice->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(m_pDefaultObject.GetAddress()));

    // メモリ解放.
    objDesc.clear();

    if (FAILED(hr))
    {
        ELOG("Error : ID3D12Device5::CreateStateObject() Failed. errcode = 0x%x", hr);
        return false;
    }

    hr = m_pDefaultObject->QueryInterface(IID_PPV_ARGS(m_pDefaultProps.GetAddress()));
    if (FAILED(hr))
    {
        ELOG("Error : ID3D12StateObject::QueryInterface() Failed. errcode = 0x%x", hr);
        return false;
    }

    // レイ生成テーブル.
    {
        asdx::ShaderRecord record = {};
        record.ShaderIdentifier = m_pDefaultProps->GetShaderIdentifier(desc.RayGeneration.c_str());

        asdx::ShaderTable::Desc desc = {};
        desc.RecordCount = 1;
        desc.pRecords = &record;

        if (!m_DefaultRayGeneration.Init(pDevice, &desc))
        {
            ELOGA("Error : RayGenenerationTable Init Failed.");
            return false;
        }
    }

    // ミステーブル.
    {
        std::vector<asdx::ShaderRecord> record;
        record.resize(desc.MissTable.size());
        for(size_t i=0; i<desc.MissTable.size(); ++i)
        {
            record[i].ShaderIdentifier = m_pDefaultProps->GetShaderIdentifier(desc.MissTable[i].c_str());
        }

        asdx::ShaderTable::Desc desc = {};
        desc.RecordCount = uint32_t(record.size());
        desc.pRecords    = record.data();

        if (!m_DefaultMiss.Init(pDevice, &desc))
        {
            ELOGA("Error : MissTable Init Failed.");
            return false;
        }
    }

    // ヒットグループ.
    {
        std::vector<asdx::ShaderRecord> record;
        record.resize(desc.HitGroups.size());
        for(size_t i=0; i<desc.HitGroups.size(); ++i)
        {
            record[i].ShaderIdentifier = m_pDefaultProps->GetShaderIdentifier(desc.HitGroups[i].HitGroupExport);
        }

        asdx::ShaderTable::Desc desc = {};
        desc.RecordCount = uint32_t(record.size());
        desc.pRecords = record.data();

        if (!m_DefaultHitGroup.Init(pDevice, &desc))
        {
            ELOGA("Error : HitGroup Init Failed.");
            return false;
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void RayTracingPipelineState::Term()
{
    {
        auto object = m_pDefaultObject.Detach();
        auto props  = m_pDefaultProps .Detach();

        Dispose(object);
        Dispose(props);

        m_DefaultRayGeneration.Term();
        m_DefaultMiss         .Term();
        m_DefaultHitGroup     .Term();
    }

    {
        auto object = m_pReloadProps.Detach();
        auto props  = m_pReloadProps.Detach();

        Dispose(object);
        Dispose(props);

        m_ReloadRayGeneration.Term();
        m_ReloadMiss         .Term();
        m_ReloadtHitGroup    .Term();
    }

    m_ReloadPathLib .clear();
    m_ShaderModelLib.clear();
    m_IncludeDirs   .clear();
    m_Dependencies  .clear();
}

//-----------------------------------------------------------------------------
//      シェーダライブラリのリロードパスを設定します.
//-----------------------------------------------------------------------------
void RayTracingPipelineState::SetReloadPathLib(const char* path, const char* shaderModel)
{
    m_ReloadPathLib  = ToFullPathA(path);
    m_ShaderModelLib = shaderModel;
}

//-----------------------------------------------------------------------------
//      インクルードディレクトリを設定します.
//-----------------------------------------------------------------------------
void RayTracingPipelineState::SetIncludeDirs(const std::vector<std::string>& dirs)
{
    m_IncludeDirs.resize(dirs.size());
    for(auto i=0; i<dirs.size(); ++i)
    { m_IncludeDirs[i] = ToFullPathA(dirs[i].c_str()); }
}

//-----------------------------------------------------------------------------
//      依存ファイルを設定します.
//-----------------------------------------------------------------------------
void RayTracingPipelineState::SetDependencies(const std::vector<std::string>& dependencies)
{
    m_Dependencies.resize(dependencies.size());
    for(auto i=0; i<dependencies.size(); ++i)
    { m_Dependencies[i] = ToFullPathA(dependencies[i].c_str()); }
}

//-----------------------------------------------------------------------------
//      ファイル更新時の処理です.
//-----------------------------------------------------------------------------
void RayTracingPipelineState::OnUpdate(const FileUpdateEventArgs& args)
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
            if (!m_ReloadPathLib.empty() && m_ReloadPathLib == path)
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
bool RayTracingPipelineState::ReloadShader(const char* path, const char* shaderModel, std::vector<uint8_t>& result)
{
    RefPtr<IBlob> blob;
    // シェーダコンパイル.
    if (!CompileFromFileA(path, m_IncludeDirs, "", shaderModel, blob.GetAddress()))
    { return false; }

    result.clear();
    result.resize(blob->GetBufferSize());
    memcpy(result.data(), blob->GetBufferPointer(), blob->GetBufferSize());

    return true;
}

//-----------------------------------------------------------------------------
//      リビルドを行います.
//-----------------------------------------------------------------------------
void RayTracingPipelineState::Rebuild()
{
    if (!m_Dirty)
        return;

    m_Dirty = false;

    // シェーダリロード.
    if (!ReloadShader(m_ReloadPathLib.c_str(), m_ShaderModelLib.c_str(), m_Lib))
    {
        ELOGA("Error : Shader Reload Failed. File=%s, ShaderModel=%s", m_ReloadPathLib.c_str(), m_ShaderModelLib.c_str());
        return;
    }

    // オブジェクトを遅延解放.
    {
        auto object = m_pReloadObject.Detach();
        auto props  = m_pReloadProps .Detach();
        Dispose(object);
        Dispose(props);

        m_ReloadRayGeneration.Term();
        m_ReloadMiss         .Term();
        m_ReloadtHitGroup    .Term();
    }

    auto pDevice = GetD3D12Device();

    uint32_t objCount = 5u + uint32_t(m_Desc.HitGroups.size());

    std::vector<D3D12_STATE_SUBOBJECT> objDesc;
    objDesc.resize(objCount);

    uint32_t hitExports = 0;

    std::vector<D3D12_EXPORT_DESC> exports;
    {
        exports.push_back({ m_Desc.RayGeneration.c_str(), nullptr, D3D12_EXPORT_FLAG_NONE });
        for(auto& hit : m_Desc.HitGroups)
        {
            if (hit.AnyHitShaderImport)
            {
                exports.push_back({hit.AnyHitShaderImport, nullptr, D3D12_EXPORT_FLAG_NONE});
                hitExports++;
            }
            if (hit.ClosestHitShaderImport)
            {
                exports.push_back({hit.ClosestHitShaderImport, nullptr, D3D12_EXPORT_FLAG_NONE});
                hitExports++;
            }
            if (hit.IntersectionShaderImport)
            {
                exports.push_back({hit.IntersectionShaderImport, nullptr, D3D12_EXPORT_FLAG_NONE});
                hitExports++;
            }
        }
        for(auto& miss : m_Desc.MissTable)
        {
            exports.push_back({miss.c_str(), nullptr, D3D12_EXPORT_FLAG_NONE});
        }
    }

    auto index = 0;

    D3D12_GLOBAL_ROOT_SIGNATURE globalRootSignature = {};
    globalRootSignature.pGlobalRootSignature = m_Desc.pGlobalRootSignature;

    objDesc[index].Type  = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    objDesc[index].pDesc = &globalRootSignature;
    index++;

    D3D12_DXIL_LIBRARY_DESC libDesc = {};
    libDesc.DXILLibrary.BytecodeLength  = m_Lib.size();
    libDesc.DXILLibrary.pShaderBytecode = m_Lib.data();
    libDesc.NumExports  = UINT(exports.size());
    libDesc.pExports    = exports.data();

    objDesc[index].Type     = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    objDesc[index].pDesc    = &libDesc;
    index++;

    D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
    shaderConfig.MaxAttributeSizeInBytes = m_Desc.MaxAttributeSize;
    shaderConfig.MaxPayloadSizeInBytes   = m_Desc.MaxPayloadSize;

    objDesc[index].Type     = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    objDesc[index].pDesc    = &shaderConfig;
    index++;

    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
    pipelineConfig.MaxTraceRecursionDepth = m_Desc.MaxTraceRecursionDepth;

    objDesc[index].Type     = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    objDesc[index].pDesc    = &pipelineConfig;
    index++;

    for(auto i=0u; i<m_Desc.HitGroups.size(); ++i)
    {
        objDesc[index].Type  = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
        objDesc[index].pDesc = &m_Desc.HitGroups[i];
        index++;
    }

    D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
    stateObjectDesc.Type            = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    stateObjectDesc.NumSubobjects   = index;
    stateObjectDesc.pSubobjects     = objDesc.data();


    auto hr = pDevice->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(m_pReloadObject.GetAddress()));

    // メモリ解放.
    objDesc.clear();

    if (FAILED(hr))
    {
        ELOG("Error : ID3D12Device5::CreateStateObject() Failed. errcode = 0x%x", hr);
        return;
    }

    hr = m_pReloadObject->QueryInterface(IID_PPV_ARGS(m_pReloadProps.GetAddress()));
    if (FAILED(hr))
    {
        ELOG("Error : ID3D12StateObject::QueryInterface() Failed. errcode = 0x%x", hr);
        return;
    }

    // レイ生成テーブル.
    {
        asdx::ShaderRecord record = {};
        record.ShaderIdentifier = m_pReloadProps->GetShaderIdentifier(m_Desc.RayGeneration.c_str());

        asdx::ShaderTable::Desc desc = {};
        desc.RecordCount = 1;
        desc.pRecords = &record;

        if (!m_ReloadRayGeneration.Init(pDevice, &desc))
        {
            ELOGA("Error : RayGenenerationTable Init Failed.");
            return;
        }
    }

    // ミステーブル.
    {
        std::vector<asdx::ShaderRecord> record;
        record.resize(m_Desc.MissTable.size());
        for(size_t i=0; i<m_Desc.MissTable.size(); ++i)
        {
            record[i].ShaderIdentifier = m_pReloadProps->GetShaderIdentifier(m_Desc.MissTable[i].c_str());
        }

        asdx::ShaderTable::Desc desc = {};
        desc.RecordCount = uint32_t(record.size());
        desc.pRecords    = record.data();

        if (!m_ReloadMiss.Init(pDevice, &desc))
        {
            ELOGA("Error : MissTable Init Failed.");
            return;
        }
    }

    // ヒットグループ.
    {
        std::vector<asdx::ShaderRecord> record;
        record.resize(m_Desc.HitGroups.size());
        for(size_t i=0; i<m_Desc.HitGroups.size(); ++i)
        {
            record[i].ShaderIdentifier = m_pReloadProps->GetShaderIdentifier(m_Desc.HitGroups[i].HitGroupExport);
        }

        asdx::ShaderTable::Desc desc = {};
        desc.RecordCount = uint32_t(record.size());
        desc.pRecords = record.data();

        if (!m_ReloadtHitGroup.Init(pDevice, &desc))
        {
            ELOGA("Error : HitGroup Init Failed.");
            return;
        }
    }

    ILOGA("Info : Shader Reloaded! Lib=%s(%s)", m_ReloadPathLib.c_str(), m_ShaderModelLib.c_str());
}

//-----------------------------------------------------------------------------
//      レイトレーシングパイプラインを起動します.
//-----------------------------------------------------------------------------
void RayTracingPipelineState::DispatchRays(ID3D12GraphicsCommandList4* pCmd, uint32_t width, uint32_t height)
{
    assert(pCmd != nullptr);
    assert(width > 0);
    assert(height > 0);

    Rebuild();

    D3D12_DISPATCH_RAYS_DESC desc = {};

    if (m_pReloadObject.GetPtr() != nullptr)
    {
        auto stateObject = m_pReloadObject.GetPtr();
        auto rayGenTable = m_ReloadRayGeneration.GetRecordView();
        auto missTable   = m_ReloadMiss.GetTableView();
        auto hitGroup    = m_ReloadtHitGroup.GetTableView();

        desc.RayGenerationShaderRecord  = rayGenTable;
        desc.MissShaderTable            = missTable;
        desc.HitGroupTable              = hitGroup;
        desc.Width                      = width;
        desc.Height                     = height;
        desc.Depth                      = 1;

        pCmd->SetPipelineState1(stateObject);
        pCmd->DispatchRays(&desc);
        return;
    }
    else
    {
        auto stateObject = m_pDefaultObject.GetPtr();
        auto rayGenTable = m_DefaultRayGeneration.GetRecordView();
        auto missTable   = m_DefaultMiss.GetTableView();
        auto hitGroup    = m_DefaultHitGroup.GetTableView();

        desc.RayGenerationShaderRecord  = rayGenTable;
        desc.MissShaderTable            = missTable;
        desc.HitGroupTable              = hitGroup;
        desc.Width                      = width;
        desc.Height                     = height;
        desc.Depth                      = 1;

        pCmd->SetPipelineState1(stateObject);
        pCmd->DispatchRays(&desc);
    }
}


///////////////////////////////////////////////////////////////////////////////
// ShaderTable class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
ShaderTable::ShaderTable()
: m_RecordSize(0)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
ShaderTable::~ShaderTable()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理です.
//-----------------------------------------------------------------------------
bool ShaderTable::Init(ID3D12Device* pDevice, const Desc* pDesc)
{
    // アライメントを揃える.
    m_RecordSize = RoundUp(
        D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + pDesc->LocalRootArgumentSize,
        D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

    auto bufferSize = m_RecordSize * pDesc->RecordCount;

    // アップロードバッファ生成.
    if (!CreateUploadBuffer(pDevice, bufferSize, m_Resource.GetAddress()))
    {
        ELOGA("Error : CreateUploadBuffer() Failed.");
        return false;
    }

    // メモリマッピング.
    uint8_t* ptr = nullptr;
    auto hr = m_Resource->Map(0, nullptr, reinterpret_cast<void**>(&ptr));
    if (FAILED(hr))
    {
        ELOGA("Error : ID3D12Resource::Map() Failed. errcode = 0x%x", hr);
        return false;
    }

    // 大きく分岐して高速化.
    if (pDesc->LocalRootArgumentSize == 0)
    {
        for(auto i=0u; i<pDesc->RecordCount; ++i)
        {
            auto record = pDesc->pRecords[i];
            memcpy(ptr, record.ShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            ptr += m_RecordSize;
        }
    }
    else
    {
        for(auto i=0u; i<pDesc->RecordCount; ++i)
        {
            auto record = pDesc->pRecords[i];
            memcpy(ptr, record.ShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            memcpy(ptr + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, record.LocalRootArguments, pDesc->LocalRootArgumentSize);
            ptr += m_RecordSize;
        }
    }

    // メモリマッピング解除.
    m_Resource->Unmap(0, nullptr);

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      終了処理です.
//-----------------------------------------------------------------------------
void ShaderTable::Term()
{
    auto resource = m_Resource.Detach();
    Dispose(resource);
}

//-----------------------------------------------------------------------------
//      GPU仮想アドレスと範囲を取得します.
//-----------------------------------------------------------------------------
D3D12_GPU_VIRTUAL_ADDRESS_RANGE ShaderTable::GetRecordView() const
{
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE result = {};
    result.StartAddress = m_Resource->GetGPUVirtualAddress();
    result.SizeInBytes  = m_Resource->GetDesc().Width;
    return result;
}

//-----------------------------------------------------------------------------
//      GPU仮想アドレスと範囲とストライドを取得します.
//-----------------------------------------------------------------------------
D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE ShaderTable::GetTableView() const
{
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE result = {};
    result.StartAddress     = m_Resource->GetGPUVirtualAddress();
    result.SizeInBytes      = m_Resource->GetDesc().Width;
    result.StrideInBytes    = m_RecordSize;
    return result;
}

} // namespace asdx
