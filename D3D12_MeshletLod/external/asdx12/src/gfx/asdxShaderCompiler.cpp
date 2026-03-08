//-----------------------------------------------------------------------------
// File : asdxShaderCompiler.cpp
// Desc : Shader Compiler.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cstdio>
#include <cassert>
#include <atomic>
#include <fnd/asdxMisc.h>
#include <fnd/asdxLogger.h>
#include <gfx/asdxShaderCompiler.h>
#include <d3d12.h>
#include <d3d12shader.h>
#ifdef ASDX_ENABLE_DXC
    #include <dxcapi.h>
#else
    #include <d3dcompiler.h>
#endif//ASDX_ENABLE_DXC


#define ASDX_FOURCC(ch0, ch1, ch2, ch3) (                            \
  (uint32_t)(uint8_t)(ch0)        | (uint32_t)(uint8_t)(ch1) << 8  | \
  (uint32_t)(uint8_t)(ch2) << 16  | (uint32_t)(uint8_t)(ch3) << 24   \
)

namespace {

#ifdef ASDX_ENABLE_DXC
//-----------------------------------------------------------------------------
//      シェーダリフレクションを生成します.
//-----------------------------------------------------------------------------
HRESULT CreateShaderReflectionOld(const void* pData, size_t size, ID3D12ShaderReflection** ppResult)
{
    // DirectX ShaderCompiler before March 2020.

    const uint32_t kDFCC_DXIL  = ASDX_FOURCC('D', 'X', 'I', 'L');

    asdx::RefPtr<IDxcLibrary> pLibrary;
    auto hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(pLibrary.GetAddress()));
    if (FAILED(hr))
    {
        ELOG("Error : DxcCreateInstance() Failed. errcode = 0x%x", hr);
        return hr;
    }

    asdx::RefPtr<IDxcBlobEncoding> blobEncoding;
    hr = pLibrary->CreateBlobWithEncodingOnHeapCopy(pData, UINT32(size), CP_ACP, blobEncoding.GetAddress());
    if (FAILED(hr))
    {
        ELOG("Error : IDxcLibrary::CreateBlobWithEncodingOnHeapCopy() Faield. errcode = 0x%x", hr);
        return hr;
    }

    asdx::RefPtr<IDxcContainerReflection> containerReflection;
    hr = DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(containerReflection.GetAddress()));
    if (FAILED(hr))
    {
        ELOG("Error : DxcCreateInstance() Failed. errcode = 0x%x", hr);
        return hr;
    }

    uint32_t shaderIdx = 0;
    hr = containerReflection->Load(blobEncoding.GetPtr());
    if (FAILED(hr))
    {
        ELOG("Error : IDxcContainerReflection::Load() Failed. errcode = 0x%x", hr);
        return hr;
    }

    hr = containerReflection->FindFirstPartKind(kDFCC_DXIL, &shaderIdx);
    if (FAILED(hr))
    {
        ELOG("Error : IDxcContainerReflection::FindFirstPartKind() Failed. errcode = 0x%x", hr);
        return hr;
    }

    return containerReflection->GetPartReflection(shaderIdx, IID_PPV_ARGS(ppResult));
}
#endif

} // namespace


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// Blob class
///////////////////////////////////////////////////////////////////////////////
class Blob : public IBlob
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    /* NOTHING */

public:
    //=========================================================================
    // public variables.
    //=========================================================================
    size_t m_Size = 0;
    void*  m_Buffer = nullptr;

    //=========================================================================
    // public methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //      コンストラクタです.
    //-------------------------------------------------------------------------
    Blob()
    : m_Count(1)
    { /* DO_NOTHING */ }

    //-------------------------------------------------------------------------
    //      デストラクタです.
    //-------------------------------------------------------------------------
    ~Blob()
    {
        if (m_Buffer)
        {
            free(m_Buffer);
            m_Buffer = nullptr;
        }
        m_Size = 0;
    }

    //-------------------------------------------------------------------------
    //      メモリを確保します.
    //-------------------------------------------------------------------------
    bool Alloc(size_t size)
    {
        m_Buffer = malloc(size);
        if (m_Buffer == nullptr)
        { return false; }

        m_Size = size;
        return true;
    }

    //-------------------------------------------------------------------------
    //      参照カウントを上げます.
    //-------------------------------------------------------------------------
    void AddRef() override
    { m_Count++; }

    //-------------------------------------------------------------------------
    //      参照カウントを下げます.
    //-------------------------------------------------------------------------
    void Release() override
    {
        m_Count--;
        if (m_Count == 0)
        { delete this; }
    }

    //-------------------------------------------------------------------------
    //      参照カウントを取得します.
    //-------------------------------------------------------------------------
    uint32_t GetCount() const override
    { return m_Count; }

    //-------------------------------------------------------------------------
    //      バッファサイズを取得します.
    //-------------------------------------------------------------------------
    size_t GetBufferSize() override
    { return m_Size; }

    //-------------------------------------------------------------------------
    //      バッファポインタを取得します.
    //-------------------------------------------------------------------------
    void* GetBufferPointer() override
    { return m_Buffer; }

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    std::atomic<uint32_t> m_Count;

    //=========================================================================
    // private methods.
    //=========================================================================
    /* NOTHING */
};

//-----------------------------------------------------------------------------
//      バイナリラージオブジェクトを生成します.
//-----------------------------------------------------------------------------
bool CreateBlob(size_t size, IBlob** ppResult)
{
    auto blob = new Blob();
    if (!blob->Alloc(size))
    {
        delete blob;
        return false;
    }

    *ppResult = blob;
    return true;
}

//-----------------------------------------------------------------------------
//      バイナリラージオブジェクトに読み込みます.
//-----------------------------------------------------------------------------
bool ReadFileToBlob(const char* filename, IBlob** ppResult)
{
    FILE* pFile;
    auto err = fopen_s(&pFile, filename, "rb");
    if (err != 0 || pFile == nullptr)
    {
        return false;
    }

    auto prevpos = ftell(pFile);
    fseek(pFile, 0, SEEK_END);
    auto currpos = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    auto size = uint64_t(currpos) - uint64_t(prevpos);
    auto ptr = malloc(size);
    if (ptr == nullptr)
    {
        fclose(pFile);
        return false;
    }

    fread(ptr, size, 1, pFile);
    fclose(pFile);

    auto blob = new Blob();
    blob->m_Buffer = ptr;
    blob->m_Size = size;

    *ppResult = blob;
    return true;
}

//-----------------------------------------------------------------------------
//      バイナリラージオブジェクトを書き出します.
//-----------------------------------------------------------------------------
bool WriteBlobToFile(IBlob* pBlob, const char* filename)
{
    FILE* pFile;
    auto err = fopen_s(&pFile, filename, "wb");
    if (err != 0)
    {
        return false;
    }

    fwrite(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), 1, pFile);
    fclose(pFile);

    return true;
}


//-----------------------------------------------------------------------------
//      シェーダコンパイルします.
//-----------------------------------------------------------------------------
bool CompileFromFileA
(
    const char*                 filename,
    std::vector<std::string>    includeDirs,
    const char*                 entryPoint,
    const char*                 shaderModel,
    IBlob**                     ppResult
)
{
#ifdef ASDX_ENABLE_DXC
    FILE* pFile = nullptr;
    std::vector<uint8_t> buffer;
    auto err = fopen_s(&pFile, filename, "rb");
    if (err != 0 || pFile == nullptr)
    {
        return false;
    }

    auto cur = ftell(pFile);
    fseek(pFile, 0, SEEK_END);
    auto end = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);
    auto size = end - cur;
    buffer.resize(end - cur + 1);
    fread(buffer.data(), size, 1, pFile);
    fclose(pFile);

    HRESULT hr = S_OK;
    RefPtr<IDxcUtils> pUtils;
    RefPtr<IDxcCompiler> pCompiler;
    RefPtr<IDxcLibrary> pLibrary;
    RefPtr<IDxcIncludeHandler> pIncludeHandler;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(pUtils.GetAddress()));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(pCompiler.GetAddress()));
    DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(pLibrary.GetAddress()));

    RefPtr<IDxcBlobEncoding> pSource;
    hr = pLibrary->CreateBlobWithEncodingFromPinned(buffer.data(), UINT32(buffer.size()), DXC_CP_UTF8, pSource.GetAddress());
    if (FAILED(hr))
    {
        ELOGA("Error : IDxcLibrary::CreateBlobWithEncodingFromPinned() Failed. errcode = 0x%x", hr);
        return false;
    }

    hr = pUtils->CreateDefaultIncludeHandler(pIncludeHandler.GetAddress());
    if (FAILED(hr))
    {
        ELOG("Error : IDxcUtils::CreateDefaultIncludeHandler() Failed. errcode = 0x%x", hr);
        return false;
    }

    std::vector<LPCWSTR> args;
    std::vector<std::wstring> wpaths;
#if defined(DEBUG) || defined(_DEBUG)
    args.push_back(L"-Zi");
    args.push_back(L"-O0");
    args.push_back(L"-Qembed_debug");
#else
    args.push_back(L"-O2");
#endif
    if (!includeDirs.empty())
    {
        wpaths.resize(includeDirs.size());
        for(size_t i=0; i<includeDirs.size(); ++i)
        {
            wpaths[i] = asdx::ToStringW(includeDirs[i].c_str());
            args.push_back(L"-I");
            args.push_back(wpaths[i].c_str());
        }
    }

    auto ep = asdx::ToStringW(entryPoint);
    auto sm = asdx::ToStringW(shaderModel);

    RefPtr<IDxcOperationResult> pResults;
    pCompiler->Compile(
        pSource.GetPtr(),
        ToStringW(filename).c_str(),
        ep.c_str(),
        sm.c_str(),
        args.data(),
        UINT(args.size()),
        nullptr,
        0,
        pIncludeHandler.GetPtr(),
        pResults.GetAddress());

    HRESULT ret;
    pResults->GetStatus(&ret);
    if (FAILED(ret))
    {
        RefPtr<IDxcBlobEncoding> pErrorBlob;
        pResults->GetErrorBuffer(pErrorBlob.GetAddress());
        ELOGA("Compilation Failed. errcode = 0x%x, msg = %s.",
            ret, 
            (pErrorBlob->GetBufferSize() > 0)
            ? reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer())
            : "Unknown");
        
        return false;
    }

    RefPtr<IDxcBlob> pShader;
    pResults->GetResult(pShader.GetAddress());

    auto blob = new Blob();
    blob->m_Buffer = malloc(pShader->GetBufferSize());
    blob->m_Size   = pShader->GetBufferSize();

    memcpy(blob->m_Buffer, pShader->GetBufferPointer(), pShader->GetBufferSize());
    *ppResult = blob;

    return true;
#else
    RefPtr<ID3DBlob> pShader;
    RefPtr<ID3DBlob> pError;
    auto hr = D3DCompileFromFile(
        filename,
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint,
        shaderModel,
        flags,
        0,
        pShader.GetAddress(),
        pError.GetAddress());
    if (FAILED(hr))
    {
        return false;
    }

    auto blob = new Blob();
    blob->m_Buffer = malloc(pShader->GetBufferSize());
    blob->m_Size = pShader->GetBufferSize();
    assert(blob->m_Buffer != nullptr);
    memcpy(blob->m_Buffer, pShader->GetBufferPointer(), pShader->GetBufferSize());
    *ppResult = blob;

    return true;
#endif
}


//-----------------------------------------------------------------------------
//      シェーダコンパイルします.
//-----------------------------------------------------------------------------
bool CompileFromFileW
(
    const wchar_t*            filename,
    std::vector<std::wstring> includeDirs,
    const char*               entryPoint,
    const char*               shaderModel,
    IBlob**                   ppResult
)
{
#ifdef ASDX_ENABLE_DXC
    FILE* pFile = nullptr;
    std::vector<uint8_t> buffer;
    auto err = _wfopen_s(&pFile, filename, L"rb");
    if (err != 0 || pFile == nullptr)
    {
        return false;
    }

    auto cur = ftell(pFile);
    fseek(pFile, 0, SEEK_END);
    auto end = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);
    auto size = end - cur;
    buffer.resize(end - cur + 1);
    fread(buffer.data(), size, 1, pFile);
    fclose(pFile);

    HRESULT hr = S_OK;
    RefPtr<IDxcUtils> pUtils;
    RefPtr<IDxcCompiler> pCompiler;
    RefPtr<IDxcLibrary> pLibrary;
    RefPtr<IDxcIncludeHandler> pIncludeHandler;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(pUtils.GetAddress()));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(pCompiler.GetAddress()));
    DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(pLibrary.GetAddress()));

    RefPtr<IDxcBlobEncoding> pSource;
    hr = pLibrary->CreateBlobWithEncodingFromPinned(buffer.data(), UINT32(buffer.size()), DXC_CP_UTF8, pSource.GetAddress());
    if (FAILED(hr))
    {
        ELOGA("Error : IDxcLibrary::CreateBlobWithEncodingFromPinned() Failed. errcode = 0x%x", hr);
        return false;
    }

    hr = pUtils->CreateDefaultIncludeHandler(pIncludeHandler.GetAddress());
    if (FAILED(hr))
    {
        ELOG("Error : IDxcUtils::CreateDefaultIncludeHandler() Failed. errcode = 0x%x", hr);
        return false;
    }

    std::vector<LPCWSTR> args;
#if defined(DEBUG) || defined(_DEBUG)
    args.push_back(L"-Zi");
    args.push_back(L"-O0");
    args.push_back(L"-Qembed_debug");
#else
    args.push_back(L"-O2");
#endif
    if (!includeDirs.empty())
    {
        for(size_t i=0; i<includeDirs.size(); ++i)
        {
            args.push_back(L"-I");
            args.push_back(includeDirs[i].c_str());
        }
    }

    auto ep = asdx::ToStringW(entryPoint);
    auto sm = asdx::ToStringW(shaderModel);

    RefPtr<IDxcOperationResult> pResults;
    pCompiler->Compile(
        pSource.GetPtr(),
        filename,
        ep.c_str(),
        sm.c_str(),
        args.data(),
        UINT(args.size()),
        nullptr,
        0,
        pIncludeHandler.GetPtr(),
        pResults.GetAddress());

    HRESULT ret;
    pResults->GetStatus(&ret);
    if (FAILED(ret))
    {
        RefPtr<IDxcBlobEncoding> pErrorBlob;
        pResults->GetErrorBuffer(pErrorBlob.GetAddress());
        ELOGA("Compilation Failed. errcode = 0x%x, msg = %s.",
            ret, 
            (pErrorBlob->GetBufferSize() > 0)
            ? reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer())
            : "Unknown");
        
        return false;
    }

    RefPtr<IDxcBlob> pShader;
    pResults->GetResult(pShader.GetAddress());

    auto blob = new Blob();
    blob->m_Buffer = malloc(pShader->GetBufferSize());
    blob->m_Size   = pShader->GetBufferSize();

    memcpy(blob->m_Buffer, pShader->GetBufferPointer(), pShader->GetBufferSize());
    *ppResult = blob;

    return true;
#else
    RefPtr<ID3DBlob> pShader;
    RefPtr<ID3DBlob> pError;
    auto hr = D3DCompileFromFile(
        filename,
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint,
        shaderModel,
        flags,
        0,
        pShader.GetAddress(),
        pError.GetAddress());
    if (FAILED(hr))
    {
        return false;
    }

    auto blob = new Blob();
    blob->m_Buffer = malloc(pShader->GetBufferSize());
    blob->m_Size = pShader->GetBufferSize();
    assert(blob->m_Buffer != nullptr);
    memcpy(blob->m_Buffer, pShader->GetBufferPointer(), pShader->GetBufferSize());
    *ppResult = blob;

    return true;
#endif
}

//-----------------------------------------------------------------------------
//      シェーダコンパイルします.
//-----------------------------------------------------------------------------
bool CompileFromFile
(
    const wchar_t*  filename,
    const wchar_t** compileArgs,
    uint32_t        countCompileArgs,
    IBlob**         ppResult
)
{
#ifdef ASDX_ENABLE_DXC
    HRESULT hr = S_OK;
    RefPtr<IDxcUtils> pUtils;
    RefPtr<IDxcCompiler3> pCompiler;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(pUtils.GetAddress()));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(pCompiler.GetAddress()));

    RefPtr<IDxcIncludeHandler> pIncludeHandler;
    hr = pUtils->CreateDefaultIncludeHandler(pIncludeHandler.GetAddress());
    if (FAILED(hr))
    { return false; }

    RefPtr<IDxcBlobEncoding> pSource;
    pUtils->LoadFile(filename, nullptr, pSource.GetAddress());

    BOOL known;
    UINT32 codePage;
    pSource->GetEncoding(&known, &codePage);

    DxcBuffer source;
    source.Ptr = pSource->GetBufferPointer();
    source.Size = pSource->GetBufferSize();
    source.Encoding = (known == TRUE) ? codePage : DXC_CP_UTF8;

    RefPtr<IDxcResult> pResults;
    pCompiler->Compile(
        &source,
        compileArgs,
        countCompileArgs,
        pIncludeHandler.GetPtr(),
        IID_PPV_ARGS(pResults.GetAddress()));

    RefPtr<IDxcBlobUtf8> pErrors;
    pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.GetAddress()), nullptr);
    if (pErrors.GetPtr() != nullptr && pErrors->GetStringLength() != 0)
    {
        ELOGA("Warnings and Errors:\n%s", pErrors->GetStringPointer());
    }

    HRESULT ret;
    pResults->GetStatus(&ret);
    if (FAILED(ret))
    {
        ELOGA("Compilation Failed. errcode = 0x%x", ret);
        return false;
    }

    RefPtr<IDxcBlob> pShader;
    RefPtr<IDxcBlobUtf16> pShaderName;
    pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(pShader.GetAddress()), pShaderName.GetAddress());

    auto blob = new Blob();
    blob->m_Buffer = malloc(pShader->GetBufferSize());
    blob->m_Size = pShader->GetBufferSize();

    memcpy(blob->m_Buffer, pShader->GetBufferPointer(), pShader->GetBufferSize());
    *ppResult = blob;

    return true;
#else
    std::string entryPoint = "main";
    std::string shaderModel = "vs_5_0";
    UINT flags = 0;

    for(auto i=0u; i<countCompileArgs; ++i)
    {
        // エントリーポイント.
        if (wcscmp(compileArgs[i], L"-E") == 0)
        {
            i++;
            entryPoint = asdx::ToStringA(compileArgs[i]);
        }
        // シェーダプロファイル.
        else if (wcscmp(compileArgs[i], L"-T") == 0)
        {
            i++;
            shaderModel = asdx::ToStringA(compileArgs[i]);
        }
    }

    return CompileFromFile(filename, entryPoint.c_str(), shaderModel.c_str(), ppResult);
#endif
}


///////////////////////////////////////////////////////////////////////////////
// ShaderReflection class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
ShaderReflection::ShaderReflection()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
ShaderReflection::~ShaderReflection()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool ShaderReflection::Init(const void* pData, size_t size)
{
#ifdef ASDX_ENABLE_DXC

#if 0 // 新しいやつ.
    asdx::RefPtr<IDxcUtils> pUtil;
    auto hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(pUtil.GetAddress()));
    if (FAILED(hr))
    {
        ELOG("Error : DxcCreateInstance() Failed. errcode = 0x%x", hr);
        return false;
    }

    DxcBuffer buf = {};
    buf.Ptr      = pData;
    buf.Size     = size;
    buf.Encoding = CP_ACP;

    hr = pUtil->CreateReflection(&buf, IID_PPV_ARGS(m_pReflection.GetAddress()));
    if (FAILED(hr))
    {
        if (hr == E_NOINTERFACE)
        {
            hr = CreateShaderReflectionOld(pData, size, m_pReflection.GetAddress());
            if (FAILED(hr))
            {
                ELOG("Error : CreateShaderReflectionOld() Failed.");
                return false;
            }
        }
        else
        {
            ELOG("Error : IDxcutils::CreateReflection() Failed. errcode = 0x%x", hr);
            return false;
        }
    }
#else
    // コンパイルエラーが出る場合は古いリフレクションに変更.
    auto hr = CreateShaderReflectionOld(pData, size, m_pReflection.GetAddress());
    if (FAILED(hr))
    {
        ELOG("Error : CreateShaderReflectionOld() Failed.");
        return false;
    }
#endif

    return true;
#else
    auto hr = D3DReflect(pData, size, IID_PPV_ARGS(m_pReflection.GetAddress()));
    if (FAILED(hr))
    {
        ELOG("Error : D3DReflect() Failed. errcode = 0x%x", hr);
        return false;
    }

    return true;
#endif
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void ShaderReflection::Term()
{ m_pReflection.Reset(); }

//-----------------------------------------------------------------------------
//      シェーダリフレクションを取得します.
//-----------------------------------------------------------------------------
ID3D12ShaderReflection* ShaderReflection::GetPtr() const
{ return m_pReflection.GetPtr(); }

//-----------------------------------------------------------------------------
//      アロー演算子です.
//-----------------------------------------------------------------------------
ID3D12ShaderReflection* ShaderReflection::operator ->()
{ return m_pReflection.GetPtr(); }

} // namespace asdx