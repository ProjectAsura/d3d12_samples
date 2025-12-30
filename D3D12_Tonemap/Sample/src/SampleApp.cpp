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

//-----------------------------------------------------------------------------
//      色度を取得する.
//-----------------------------------------------------------------------------
UINT16 inline GetChromaticityCoord(double value)
{
    return UINT16(value * 50000);
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
{ /* DO_NOTHING */ }

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
    // ルートシグニチャの生成.
    {
        auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
        flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
        flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        // ディスクリプタレンジを設定.
        D3D12_DESCRIPTOR_RANGE range = {};
        range.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors                    = 1;
        range.BaseShaderRegister                = 0;
        range.RegisterSpace                     = 0;
        range.OffsetInDescriptorsFromTableStart = 0;

        // スタティックサンプラーの設定.
        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter              = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU            = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressV            = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressW            = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.MipLODBias          = D3D12_DEFAULT_MIP_LOD_BIAS;
        sampler.MaxAnisotropy       = 1;
        sampler.ComparisonFunc      = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor         = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD              = -D3D12_FLOAT32_MAX;
        sampler.MaxLOD              = +D3D12_FLOAT32_MAX;
        sampler.ShaderRegister      = 0;
        sampler.RegisterSpace       = 0;
        sampler.ShaderVisibility    = D3D12_SHADER_VISIBILITY_PIXEL;

        // ルートパラメータの設定.
        D3D12_ROOT_PARAMETER param[2] = {};
        param[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
        param[0].Descriptor.ShaderRegister = 0;
        param[0].Descriptor.RegisterSpace  = 0;
        param[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_PIXEL;

        param[1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param[1].DescriptorTable.NumDescriptorRanges = 1;
        param[1].DescriptorTable.pDescriptorRanges   = &range;
        param[1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;

        // ルートシグニチャの設定.
        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters      = 2;
        desc.NumStaticSamplers  = 1;
        desc.pParameters        = param;
        desc.pStaticSamplers    = &sampler;
        desc.Flags              = flag;

        ComPtr<ID3DBlob> pBlob;
        ComPtr<ID3DBlob> pErrorBlob;

        // シリアライズ
        auto hr = D3D12SerializeRootSignature(
            &desc,
            D3D_ROOT_SIGNATURE_VERSION_1,
            pBlob.GetAddressOf(),
            pErrorBlob.GetAddressOf());
        if ( FAILED(hr) )
        { return false; }

        // ルートシグニチャを生成.
        hr = m_pDevice->CreateRootSignature(
            0,
            pBlob->GetBufferPointer(),
            pBlob->GetBufferSize(),
            IID_PPV_ARGS(m_pRootSig.GetAddressOf()));
        if ( FAILED(hr) )
        {
            ELOG( "Error : Root Signature Create Failed. retcode = 0x%x", hr );
            return false;
        }
    }

    // パイプラインステートの生成.
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
        desc.pRootSignature         = m_pRootSig.Get();
        desc.VS                     = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
        desc.PS                     = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
        desc.RasterizerState        = DirectX::CommonStates::CullNone;
        desc.BlendState             = DirectX::CommonStates::Opaque;
        desc.DepthStencilState      = DirectX::CommonStates::DepthDefault;
        desc.SampleMask             = UINT_MAX;
        desc.PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets       = 1;
        desc.RTVFormats[0]          = m_ColorTarget[0].GetViewDesc().Format;
        desc.DSVFormat              = m_DepthTarget.GetViewDesc().Format;
        desc.SampleDesc.Count       = 1;
        desc.SampleDesc.Quality     = 0;

        // パイプラインステートを生成.
        hr = m_pDevice->CreateGraphicsPipelineState( &desc, IID_PPV_ARGS(m_pPSO.GetAddressOf()) );
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
        if (!m_CB[i].Init(m_pDevice.Get(), m_pPool[POOL_TYPE_RES], sizeof(CbTonemap)))
        {
            ELOG("Error : ConstantBuffer::Init() Failed.");
            return false;
        }
    }

    // テクスチャロード.
    {
        std::wstring path;
        if (!SearchFilePathW(L"./res/hdr014.dds", path))
        {
            ELOG("Error : Texture Not Found.");
            return false;
        }

        DirectX::ResourceUploadBatch batch(m_pDevice.Get());

        // バッチ開始.
        batch.Begin();

        // テクスチャ初期化.
        if (!m_Texture.Init(
            m_pDevice.Get(),
            m_pPool[POOL_TYPE_RES],
            path.c_str(),
            false,
            batch))
        {
            ELOG("Error : Texture Initialize Failed.");
            return false;
        }

        // バッチ終了.
        auto future = batch.End(m_pQueue.Get());

        // 完了を待機.
        future.wait();
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了時の処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnTerm()
{
    m_pRootSig.Reset();
    m_pPSO.Reset();
    m_QuadVB.Term();
    for(auto i=0; i<FrameCount; ++i)
    { m_CB[i].Term(); }
    m_Texture.Term();
}

//-----------------------------------------------------------------------------
//      描画時の処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnRender()
{
    // 定数バッファ更新
    {
        auto ptr = m_CB[m_FrameIndex].GetPtr<CbTonemap>();
        ptr->Type          = m_TonemapType;
        ptr->ColorSpace    = m_ColorSpace;
        ptr->BaseLuminance = m_BaseLuminance;
        ptr->MaxLuminance  = m_MaxLuminance;
    }

    // コマンドリストの記録を開始.
    auto pCmd = m_CommandList.Reset();

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

    // クリアカラー.
    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    // レンダーターゲットをクリア.
    pCmd->ClearRenderTargetView(handleRTV->HandleCPU, clearColor, 0, nullptr);

    // 深度ステンシルビューをクリア.
    pCmd->ClearDepthStencilView(handleDSV->HandleCPU, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // 描画処理.
    {
        ID3D12DescriptorHeap* const pHeaps[] = {
            m_pPool[POOL_TYPE_RES]->GetHeap(),
        };

        auto VBV = m_QuadVB.GetView();

        pCmd->SetGraphicsRootSignature( m_pRootSig.Get() );
        pCmd->SetDescriptorHeaps( 1, pHeaps );
        pCmd->SetGraphicsRootConstantBufferView( 0, m_CB[m_FrameIndex].GetAddress() );
        pCmd->SetGraphicsRootDescriptorTable( 1, m_Texture.GetHandleGPU() );

        pCmd->SetPipelineState( m_pPSO.Get() );
        pCmd->RSSetViewports( 1, &m_Viewport );
        pCmd->RSSetScissorRects( 1, &m_Scissor );

        pCmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pCmd->IASetVertexBuffers(0, 1, &VBV);
        pCmd->DrawInstanced(3, 1, 0, 0);
    }

    // 表示用リソースバリア設定.
    DirectX::TransitionResource(pCmd,
        m_ColorTarget[m_FrameIndex].GetResource(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);

    // コマンドリストの記録を終了.
    pCmd->Close();

    // コマンドリストを実行.
    ID3D12CommandList* pLists[] = { pCmd };
    m_pQueue->ExecuteCommandLists( 1, pLists );

    // 画面に表示.
    Present(1);
}

//-------------------------------------------------------------------------------------------------
//      ディスプレイモードを変更します.
//-------------------------------------------------------------------------------------------------
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

        hr = m_pSwapChain->SetHDRMetaData(
            DXGI_HDR_METADATA_TYPE_HDR10,
            sizeof(DXGI_HDR_METADATA_HDR10),
            &metaData);
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

        hr = m_pSwapChain->SetHDRMetaData(
            DXGI_HDR_METADATA_TYPE_HDR10,
            sizeof(DXGI_HDR_METADATA_HDR10),
            &metaData);
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

//-------------------------------------------------------------------------------------------------
//      メッセージプロシージャです.
//-------------------------------------------------------------------------------------------------
void SampleApp::OnMsgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    // キーボードの処理.
    if ( ( msg == WM_KEYDOWN )
      || ( msg == WM_SYSKEYDOWN )
      || ( msg == WM_KEYUP )
      || ( msg == WM_SYSKEYUP ) )
    {
        DWORD mask = ( 1 << 29 );

        auto isKeyDown = ( msg == WM_KEYDOWN  || msg == WM_SYSKEYDOWN );
        auto isAltDown = ( ( lp & mask ) != 0 );
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
            }
        }
    }
}
