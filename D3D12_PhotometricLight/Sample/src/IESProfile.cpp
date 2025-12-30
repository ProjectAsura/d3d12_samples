//-----------------------------------------------------------------------------
// File : IESProfile.cpp
// Desc : IES Profile.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <IESProfile.h>
#include <vector>
#include <fstream>
#include <Logger.h>
#include <DirectXMath.h>


namespace {

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
constexpr int TypeC = 1;        // C-Plane
constexpr int TypeB = 2;        // B-Plane
constexpr int TypeA = 3;        // A-Plane

constexpr int UnitFeet  = 1;    // フィート単位[ft]
constexpr int UnitMeter = 2;    // メートル単位[m]


///////////////////////////////////////////////////////////////////////////////
// Lamp structure
///////////////////////////////////////////////////////////////////////////////
struct Lamp
{
    float               Lumen;          //!< 光束
    float               Multiplier;     //!< 乗算係数.
    int                 PhotometricType;//!< フォトメトリックタイプ(1:C-Plane, 2:B-Plane, 3:A-Plane).
    int                 UnitType;       //!< 単位(1:フィート, 2:メートル).
    float               ShapeWidth;     //!< 形状横幅.
    float               ShapeLength;    //!< 形状奥行き.
    float               ShapeHeight;    //!< 形状高さ.
    float               BallastFactor;  //!< 安定器光出力係数.
    float               InputWatts;     //!< 入力ワット数.
    std::vector<float>  AngleV;         //!< 垂直角.
    std::vector<float>  AngleH;         //!< 水平角.
    std::vector<float>  Candera;        //!< カンデラ値.
    float               AveCandera;     //!< カンデラの平均値.
};

//-----------------------------------------------------------------------------
//      線形補間を行います.
//-----------------------------------------------------------------------------
inline float Lerp(float a, float b, float t)
{ return a + (b - a) * t; }

//-----------------------------------------------------------------------------
//      インデックスを浮動小数で求めます.
//-----------------------------------------------------------------------------
float GetPos(float value, const std::vector<float>& container)
{
    if (container.size() == 1)
    { return container.front(); }

    if (value < container.front())
    { return -1.0f; }

    if (value > container.back())
    { return -1.0f; }

    size_t lhs = 0;
    size_t rhs = container.size() - 1;

    // 2分探索.
    while (lhs < rhs)
    {
        auto pivot = (lhs + rhs + 1) / 2;
        auto temp = container[pivot];

        if (value >= temp)
        { lhs = pivot; }
        else
        { rhs = pivot - 1; }
    }

    auto t = 0.0f;
    if (lhs + 1 < container.size())
    {
        auto left  = container[lhs + 0];
        auto right = container[lhs + 1];
        auto delta = right - left;

        if (delta > 1e-5f)
        { t = (value - left) / delta; }
    }

    return float(lhs) + t;
}

//-----------------------------------------------------------------------------
//      IESプロファイルをロードします.
//-----------------------------------------------------------------------------
bool LoadIESProfile(const char* path, Lamp& lamp)
{
    std::ifstream stream;

    stream.open( path, std::ios::in );

    if (!stream.is_open())
    {
        ELOG("Error : File Open Failed. path = %s", path);
        return false;
    }

    const std::streamsize BufferSize = 2048;
    char buf[BufferSize] = {};

    stream >> buf;

    // 判定文字列になっているかどうか確認.
    if (0 != strcmp(buf, "IESNA:LM-63-2002") &&
        0 != strcmp(buf, "IESNA:LM-63-1995"))
    {
        ELOG("Error : Invalid IES Profile.");
        stream.close();
        return false;
    }


    for(;;)
    {
        if (!stream)
        { break; }

        if (stream.eof())
        { break; }

        stream >> buf;

        // チルト角情報が出てきたら解析する.
        if (0 == strcmp(buf, "TILT=NONE"))
        {
            // TILEで始まる行を読み飛ばす.
            stream.ignore( BufferSize, '\n' );
            
            // 光源数を取得.
            int lampCount = 0;
            stream >> lampCount;

            // このサンプルでは１つの光源のみ対応.
            if (lampCount != 1)
            {
                ELOG("Error : Lamp count is %d.", lampCount);
                stream.close();
                return false;
            }

            int angleCountV = 0;
            int angleCountH = 0;
            int futureUse   = 0;

            stream >> lamp.Lumen;       // 光度値
            stream >> lamp.Multiplier;  // 光度の種類.
            stream >> angleCountV;      // 垂直角の数.
            stream >> angleCountH;      // 水平角の数.

            // メモリを予約.
            lamp.AngleV .reserve(angleCountV);
            lamp.AngleH .reserve(angleCountH);
            lamp.Candera.reserve(angleCountV * angleCountH);

            stream >> lamp.PhotometricType;     // 測定座標系.
            if (lamp.PhotometricType != TypeC)
            {
                ELOG("Error : Out of support.");
                stream.close();
                return false;
            }

            stream >> lamp.UnitType;            // 器具形状の単位.
            stream >> lamp.ShapeWidth;          // 器具の幅.
            stream >> lamp.ShapeLength;         // 器具の長さ.
            stream >> lamp.ShapeHeight;         // 器具の高さ.
            stream >> lamp.BallastFactor;       // 安定器光出力係数.
            stream >> futureUse;                // 予約領域.
            stream >> lamp.InputWatts;          // 定格消費電力.

            // 改行まで読み飛ばす.
            stream.ignore( BufferSize, '\n' );

            float value = 0.0f;

            // 垂直角の角度目盛.
            for(auto i=0; i<angleCountV; ++i)
            {
                stream >> value;
                lamp.AngleV.push_back(value);
            }

            // 水平角の角度目盛.
            for(auto i=0; i<angleCountH; ++i)
            {
                stream >> value;
                lamp.AngleH.push_back(value);
            }

            // 平均値を初期化.
            lamp.AveCandera = 0.0f;
            auto count = 0;

            // 光度のデータ.
            for(auto i=0; i<angleCountH; ++i)
            {
                for(auto j=0; j<angleCountV; ++j)
                {
                    stream >> value;
                    auto candera = value * lamp.Multiplier;
                    lamp.Candera.push_back(candera);
                    lamp.AveCandera += candera;
                    count++;
                }
            }

            lamp.AveCandera /= float(count);
        }

        // 改行まで読み飛ばす.
        stream.ignore( BufferSize, '\n' );
    }

    // ストリームを閉じる.
    stream.close();

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      カンデラ値を取得します.
//-----------------------------------------------------------------------------
float GetCandera(int x, int y, const Lamp& lamp)
{
    auto w = int(lamp.AngleV.size());
    auto h = int(lamp.AngleH.size());

    x %= w;
    y %= h;

    auto idx = w * y + x;
    assert(idx < lamp.Candera.size());

    return lamp.Candera[idx];
}

//-----------------------------------------------------------------------------
//      バイリニアサンプリングします.
//-----------------------------------------------------------------------------
float BilinearSample(float x, float y, const Lamp& lamp)
{
    auto x0 = int(floor(x));
    auto y0 = int(floor(y));

    auto x1 = x0 + 1;
    auto y1 = y0 + 1;

    return (x1 - x) * ( (y1 - y) * GetCandera(x0, y0, lamp) + (y - y0) * GetCandera(x0, y1, lamp) )
         + (x - x0) * ( (y1 - y) * GetCandera(x1, y0, lamp) + (y - y0) * GetCandera(x1, y1, lamp) );
}

//-----------------------------------------------------------------------------
//      補間したカンデラ値を取得します.
//-----------------------------------------------------------------------------
float Interpolate(float angleV, float angleH, const Lamp& lamp)
{
    // 最大範囲でチェック.
    assert(0 <= angleV && angleV <= 180.0f);
    assert(0 <= angleH && angleH <= 360.0f);

    // インデックスを浮動小数で取得.
    auto s = GetPos(angleV, lamp.AngleV);
    auto t = GetPos(angleH, lamp.AngleH);

    if (s < 0.0f || t < 0.0f)
    { return 0.0f; }

    // バイリニアサンプリング.
    return BilinearSample(s, t, lamp);
}

} // namespace 


///////////////////////////////////////////////////////////////////////////////
// IESProfile class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
IESProfile::IESProfile()
: m_pHandle (nullptr)
, m_pPool   (nullptr)
, m_Lumen   (0.0f)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
IESProfile::~IESProfile()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool IESProfile::Init
(
    ID3D12Device*                   pDevice,
    DescriptorPool*                 pPool,
    const char*                     filePath,
    DirectX::ResourceUploadBatch&   batch
)
{
    if (pDevice == nullptr || pPool == nullptr || filePath == nullptr)
    {
        ELOG("Error : Invalid Argument.");
        return false;
    }

    Lamp lamp;
    if (!LoadIESProfile(filePath, lamp))
    {
        ELOG("Error : IES Profile Load Failed.");
        return false;
    }

    auto size = 128;
    size = DirectX::XMMax(size, int(lamp.AngleV.size()));
    size = DirectX::XMMax(size, int(lamp.AngleH.size()));

    auto find = false;

    // 128pxから2次元テクスチャ最大値までの範囲で近いべき乗を求める.
    for (int i=7; i<=D3D12_MAX_TEXTURE_DIMENSION_2_TO_EXP; ++i)
    {
        auto lhs = exp2(i - 1);
        auto rhs = exp2(i);

        // 範囲内に収まっているかチェック.
        if (lhs < size && size <= rhs)
        {
            size = int(rhs);
            find = true;
            break;
        }
    }

    // サポート範囲外のサイズはエラーとする.
    if (!find)
    {
        ELOG("Error : Out of support.");
        return false;
    }

    auto w = size;
    auto h = size;

    m_Candera.clear();
    m_Candera.shrink_to_fit();
    m_Candera.reserve(w * h);

    auto invW = 1.0f / float(w);
    auto invH = 1.0f / float(h);
    auto invA = 1.0f / lamp.AveCandera;

    auto lastH = lamp.AngleH.back();
    auto lastV = int(lamp.AngleV.back());

    for(auto j=0; j<h; ++j)
    {
        // Φは[0, 2π]の範囲.
        auto angleH = 0.0f;

        if (lastH > 0.0f)
        {
            angleH = j * invH * 360.0f;
            angleH = fmod(angleH, 2.0f * lastH);
            if (angleH > lastH)
            { angleH = lastH * 2.0f - angleH; }
        }

        for(auto i=0; i<w; ++i)
        {
            // θは[0, π]の範囲.
            auto rad = i * invW * 2.0f - 1.0f;
            auto angleV = DirectX::XMConvertToDegrees(acos((rad)));

            auto cd = invA * Interpolate(angleV, angleH, lamp);

            // データ格納.
            m_Candera.push_back(cd);
        }
    }

    m_Lumen = lamp.Lumen;

    // テクスチャ生成.
    m_pPool = pPool;
    m_pPool->AddRef();

    m_pHandle = pPool->AllocHandle();
    if ( m_pHandle == nullptr )
    {
        ELOG("Error : DescriptorHandle Allocate Failed.");
        return false;
    }

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width              = w;
    desc.Height             = h;
    desc.DepthOrArraySize   = 1;
    desc.MipLevels          = 1;
    desc.Format             = DXGI_FORMAT_R32_FLOAT;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    D3D12_HEAP_PROPERTIES prop = {};
    prop.Type                   = D3D12_HEAP_TYPE_DEFAULT;
    prop.MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN;
    prop.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    prop.CreationNodeMask       = 0;
    prop.VisibleNodeMask        = 0;

    auto hr = pDevice->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(m_Resource.GetAddressOf()));
    if (FAILED(hr))
    {
        ELOG("Error : ID3D12Device::CreateCommittedResource() Failed. retcode = 0x%x", hr );
        return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
    viewDesc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2D;
    viewDesc.Format                         = DXGI_FORMAT_R32_FLOAT;
    viewDesc.Shader4ComponentMapping        = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    viewDesc.Texture2D.MipLevels            = 1;
    viewDesc.Texture2D.MostDetailedMip      = 0;
    viewDesc.Texture2D.PlaneSlice           = 0;
    viewDesc.Texture2D.ResourceMinLODClamp  = 0;

    pDevice->CreateShaderResourceView(m_Resource.Get(), &viewDesc, m_pHandle->HandleCPU);

    D3D12_SUBRESOURCE_DATA subRes = {};
    subRes.RowPitch   = w * sizeof(float);
    subRes.SlicePitch = h * subRes.RowPitch;
    subRes.pData      = m_Candera.data();

    batch.Upload(m_Resource.Get(), 0, &subRes, 1);

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void IESProfile::Term()
{
    m_Resource.Reset();

    if (m_pHandle != nullptr && m_pPool != nullptr)
    {
        m_pPool->FreeHandle(m_pHandle);
        m_pHandle = nullptr;
    }

    if (m_pPool != nullptr)
    {
        m_pPool->Release();
        m_pPool = nullptr;
    }

    m_Candera.clear();
    m_Candera.shrink_to_fit();

    m_Lumen = 0.0f;
}

//-----------------------------------------------------------------------------
//      CPUディスクリプタハンドルを取得します.
//-----------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE IESProfile::GetHandleCPU() const
{
    if (m_pHandle != nullptr)
    { return m_pHandle->HandleCPU; }

    return D3D12_CPU_DESCRIPTOR_HANDLE();
}

//-----------------------------------------------------------------------------
//      GPUディスクリプタハンドルを取得します.
//-----------------------------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE IESProfile::GetHandleGPU() const
{
    if (m_pHandle != nullptr)
    { return m_pHandle->HandleGPU; }

    return D3D12_GPU_DESCRIPTOR_HANDLE();
}

//-----------------------------------------------------------------------------
//      リソースを取得します.
//-----------------------------------------------------------------------------
ID3D12Resource* IESProfile::GetResource() const
{ return m_Resource.Get(); }

//-----------------------------------------------------------------------------
//      光束を取得します.
//-----------------------------------------------------------------------------
float IESProfile::GetLumen() const
{ return m_Lumen; }
