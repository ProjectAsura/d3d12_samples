//-------------------------------------------------------------------------------------------------
// File : asdxResTexture.cpp
// Desc : Resource Texture Module.
// Copyright(c) Project Asura. All right reserved.
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------
#include <cassert>
#include <memory>
#include <string>
#include <algorithm>
#include <dxgiformat.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <res/asdxResTexture.h>
#include <fnd/asdxLogger.h>
#include <fnd/asdxMath.h>


//-------------------------------------------------------------------------------------------------
// Linker
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) || defined(PROFILE)
#pragma comment(lib,"dxguid.lib")
#endif// defined(_DEBUG) || defined(PROFILE)


namespace /* aonymous */ {

//-------------------------------------------------------------------------------------------------
// Using Statements
//-------------------------------------------------------------------------------------------------
using Microsoft::WRL::ComPtr;


//-------------------------------------------------------------------------------------------------
// Constant Values.
//-------------------------------------------------------------------------------------------------
//static const uint32_t MAX_TEXTURE_SIZE = 4096;   // D3D_FEATURE_LEVEL_9_3
//static const uint32_t MAX_TEXTURE_SIZE = 8192;   // D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_10_1
static const uint32_t MAX_TEXTURE_SIZE = 16384;  // D3D_FEATURE_LEVEL_11_0

// dwFlags Value
static const unsigned int DDSD_CAPS         = 0x00000001;   // dwCaps/dwCaps2が有効.
static const unsigned int DDSD_HEIGHT       = 0x00000002;   // dwHeightが有効.
static const unsigned int DDSD_WIDTH        = 0x00000004;   // dwWidthが有効.
static const unsigned int DDSD_PITCH        = 0x00000008;   // dwPitchOrLinearSizeがPitchを表す.
static const unsigned int DDSD_PIXELFORMAT  = 0x00001000;   // dwPfSize/dwPfFlags/dwRGB～等の直接定義が有効.
static const unsigned int DDSD_MIPMAPCOUNT  = 0x00020000;   // dwMipMapCountが有効.
static const unsigned int DDSD_LINEARSIZE   = 0x00080000;   // dwPitchOrLinearSizeがLinerSizeを表す.
static const unsigned int DDSD_DEPTH        = 0x00800000;   // dwDepthが有効.

// dwPfFlags Value
static const unsigned int DDPF_ALPHAPIXELS      = 0x00000001;   // RGB以外にalphaが含まれている.
static const unsigned int DDPF_ALPHA            = 0x00000002;   // pixelはAlpha成分のみ.
static const unsigned int DDPF_FOURCC           = 0x00000004;   // dwFourCCが有効.
static const unsigned int DDPF_PALETTE_INDEXED4 = 0x00000008;   // Palette 16 colors.
static const unsigned int DDPF_PALETTE_INDEXED8 = 0x00000020;   // Palette 256 colors.
static const unsigned int DDPF_RGB              = 0x00000040;   // dwRGBBitCount/dwRBitMask/dwGBitMask/dwBBitMask/dwRGBAlphaBitMaskによってフォーマットが定義されていることを示す.
static const unsigned int DDPF_LUMINANCE        = 0x00020000;   // 1chのデータがRGB全てに展開される.
static const unsigned int DDPF_BUMPDUDV         = 0x00080000;   // pixelが符号付きであることを示す.

// dwCaps Value
static const unsigned int DDSCAPS_ALPHA     = 0x00000002;       // Alphaが含まれている場合.
static const unsigned int DDSCAPS_COMPLEX   = 0x00000008;       // 複数のデータが含まれている場合Palette/Mipmap/Cube/Volume等.
static const unsigned int DDSCAPS_TEXTURE   = 0x00001000;       // 常に1.
static const unsigned int DDSCAPS_MIPMAP    = 0x00400000;       // MipMapが存在する場合.

// dwCaps2 Value
static const unsigned int DDSCAPS2_CUBEMAP              = 0x00000200;   // CubeMapが存在する場合.
static const unsigned int DDSCAPS2_CUBEMAP_POSITIVE_X   = 0x00000400;   // CubeMap X+
static const unsigned int DDSCAPS2_CUBEMAP_NEGATIVE_X   = 0x00000800;   // CubeMap X-
static const unsigned int DDSCAPS2_CUBEMAP_POSITIVE_Y   = 0x00001000;   // CubeMap Y+
static const unsigned int DDSCAPS2_CUBEMAP_NEGATIVE_Y   = 0x00002000;   // CubeMap Y-
static const unsigned int DDSCAPS2_CUBEMAP_POSITIVE_Z   = 0x00004000;   // CubeMap Z+
static const unsigned int DDSCAPS2_CUBEMAP_NEGATIVE_Z   = 0x00008000;   // CubeMap Z-
static const unsigned int DDSCAPS2_VOLUME               = 0x00400000;   // VolumeTextureの場合.

// dwFourCC Value
static const unsigned int FOURCC_DXT1           = '1TXD';           // DXT1
static const unsigned int FOURCC_DXT2           = '2TXD';           // DXT2
static const unsigned int FOURCC_DXT3           = '3TXD';           // DXT3
static const unsigned int FOURCC_DXT4           = '4TXD';           // DXT4
static const unsigned int FOURCC_DXT5           = '5TXD';           // DXT5
static const unsigned int FOURCC_ATI1           = '1ITA';           // 3Dc ATI2
static const unsigned int FOURCC_ATI2           = '2ITA';           // 3Dc ATI2
static const unsigned int FOURCC_DX10           = '01XD';           // DX10
static const unsigned int FOURCC_BC4U           = 'U4CB';           // BC4U
static const unsigned int FOURCC_BC4S           = 'S4CB';           // BC4S
static const unsigned int FOURCC_BC5U           = 'U5CB';           // BC5U
static const unsigned int FOURCC_BC5S           = 'S5CB';           // BC5S
static const unsigned int FOURCC_A16B16G16R16   = 0x00000024;
static const unsigned int FOURCC_Q16W16V16U16   = 0x0000006e;
static const unsigned int FOURCC_R16F           = 0x0000006f;
static const unsigned int FOURCC_G16R16F        = 0x00000070;
static const unsigned int FOURCC_A16B16G16R16F  = 0x00000071;
static const unsigned int FOURCC_R32F           = 0x00000072;
static const unsigned int FOURCC_G32R32F        = 0x00000073;
static const unsigned int FOURCC_A32B32G32R32F  = 0x00000074;
static const unsigned int FOURCC_CxV8U8         = 0x00000075;
static const unsigned int FOURCC_Q8W8V8U8       = 0x0000003f;


///////////////////////////////////////////////////////////////////////////////////////////////////
// NATIVE_TEXTURE_FORMAT enum
///////////////////////////////////////////////////////////////////////////////////////////////////
enum NATIVE_TEXTURE_FORMAT
{
    NATIVE_TEXTURE_FORMAT_ARGB_8888 = 0,
    NATIVE_TEXTURE_FORMAT_ABGR_8888,
    NATIVE_TEXTURE_FORMAT_XRGB_8888,
    NATIVE_TEXTURE_FORMAT_XBGR_8888,
    NATIVE_TEXTURE_FORMAT_BC1,
    NATIVE_TEXTURE_FORMAT_BC2,
    NATIVE_TEXTURE_FORMAT_BC3,
    NATIVE_TEXTURE_FORMAT_A8,
    NATIVE_TEXTURE_FORMAT_R8,
    NATIVE_TEXTURE_FORMAT_L8A8,
    NATIVE_TEXTURE_FORMAT_BC4U,
    NATIVE_TEXTURE_FORMAT_BC4S,
    NATIVE_TEXTURE_FORMAT_BC5U,
    NATIVE_TEXTURE_FORMAT_BC5S,
    NATIVE_TEXTURE_FORMAT_R16_FLOAT,
    NATIVE_TEXTURE_FORMAT_G16R16_FLOAT,
    NATIVE_TEXTURE_FORMAT_A16B16G16R16_FLOAT,
    NATIVE_TEXTURE_FORMAT_R32_FLOAT,
    NATIVE_TEXTURE_FORMAT_G32R32_FLOAT,
    NATIVE_TEXTURE_FORMAT_A32B32G32R32_FLOAT,
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// TGA_FORMA_TYPE enum
////////////////////////////////////////////////////////////////////////////////////////////////////
enum TGA_FORMAT_TYPE
{
    TGA_FORMAT_NONE             = 0,        //!< イメージなし.
    TGA_FORMAT_INDEXCOLOR       = 1,        //!< インデックスカラー(256色).
    TGA_FORMAT_FULLCOLOR        = 2,        //!< フルカラー
    TGA_FORMAT_GRAYSCALE        = 3,        //!< 白黒.
    TGA_FORMAT_RLE_INDEXCOLOR   = 9,        //!< RLE圧縮インデックスカラー.
    TGA_FORMAT_RLE_FULLCOLOR    = 10,       //!< RLE圧縮フルカラー.
    TGA_FORMAT_RLE_GRAYSCALE    = 11,       //!< RLE圧縮白黒.
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// DDPixelFormat structure
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct __DDPixelFormat
{
    unsigned int    size;
    unsigned int    flags;
    unsigned int    fourCC;
    unsigned int    bpp;
    unsigned int    maskR;
    unsigned int    maskG;
    unsigned int    maskB;
    unsigned int    maskA;
} DDPixelFormat;


///////////////////////////////////////////////////////////////////////////////////////////////////
// DDColorKey structure
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct __DDColorKey
{
    unsigned int    low;
    unsigned int    hight;
} DDColorKey;


///////////////////////////////////////////////////////////////////////////////////////////////////
// DDSurfaceDesc structure
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct __DDSurfaceDesc
{
    unsigned int    size;
    unsigned int    flags;
    unsigned int    height;
    unsigned int    width;
    unsigned int    pitch;
    unsigned int    depth;
    unsigned int    mipMapLevels;
    unsigned int    alphaBitDepth;
    unsigned int    reserved;
    unsigned int    surface;

    DDColorKey      ckDestOverlay;
    DDColorKey      ckDestBit;
    DDColorKey      ckSrcOverlay;
    DDColorKey      ckSrcBit;

    DDPixelFormat   pixelFormat;
    unsigned int    caps;
    unsigned int    caps2;
    unsigned int    reservedCaps[ 2 ];

    unsigned int    textureStage;
} DDSurfaceDesc;


///////////////////////////////////////////////////////////////////////////////////////////////////
// WIC Pixel Format Translation Data
///////////////////////////////////////////////////////////////////////////////////////////////////
struct WICTranslate
{
    GUID                wic;
    DXGI_FORMAT         format;
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// WIC Pixel Format nearest conversion table
///////////////////////////////////////////////////////////////////////////////////////////////////
struct WICConvert
{
    GUID        source;
    GUID        target;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// TGA_HEADER structure
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma pack( push, 1 )
struct TGA_HEADER
{
    uint8_t  IdFieldLength;      // IDフィードのサイズ(範囲は0～255).
    uint8_t  HasColorMap;        // カラーマップ有無(0=なし, 1=あり)
    uint8_t  Format;             // 画像形式.
    uint16_t ColorMapEntry;      // カラーマップエントリー.
    uint16_t ColorMapLength;     // カラーマップのエントリーの総数.
    uint8_t  ColorMapEntrySize;  // カラーマップの1エントリー当たりのビット数.
    uint16_t OffsetX;            // 画像のX座標.
    uint16_t OffsetY;            // 画像のY座標.
    uint16_t Width;              // 画像の横幅.
    uint16_t Height;             // 画像の縦幅.
    uint8_t  BitPerPixel;        // ビットの深さ.
    uint8_t  ImageDescriptor;    // (0~3bit) : 属性, 4bit : 格納方向(0=左から右,1=右から左), 5bit : 格納方向(0=下から上, 1=上から下), 6~7bit : インタリーブ(使用不可).
};
#pragma pack( pop )

////////////////////////////////////////////////////////////////////////////////////////////////////
// TGA_FOOTER structure
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma pack( push, 1 )
struct TGA_FOOTER
{
    uint32_t    OffsetExt;      // 拡張データへのオフセット(byte数) [オフセットはファイルの先頭から].
    uint32_t    OffsetDev;      // ディベロッパーエリアへのオフセット(byte数)[オフセットはファイルの先頭から].
    char        Tag[18];        // 'TRUEVISION-XFILE.\0'
};
#pragma pack( pop )


///////////////////////////////////////////////////////////////////////////////////////////////////
// TGA_EXTENSION structure
///////////////////////////////////////////////////////////////////////////////////////////////////
#pragma pack( push, 1 )
struct TGA_EXTENSION
{
    uint16_t    Size;                       //!< サイズ.
    char        AuthorName[ 41 ];           //!< 著作者名.
    char        AuthorComment[ 324 ];       //!< 著作者コメント.
    uint16_t    StampMonth;                 //!< タイムスタンプ　月(1-12).
    uint16_t    StampDay;                   //!< タイムスタンプ　日(1-31).
    uint16_t    StampYear;                  //!< タイムスタンプ　年(4桁, 例1989).
    uint16_t    StampHour;                  //!< タイムスタンプ　時(0-23).
    uint16_t    StampMinute;                //!< タイムスタンプ　分(0-59).
    uint16_t    StampSecond;                //!< タイムスタンプ　秒(0-59).
    char        JobName[ 41 ];              //!< ジョブ名 (最後のバイトはゼロが必須).
    uint16_t    JobHour;                    //!< ジョブ時間  時(0-65535)
    uint16_t    JobMinute;                  //!< ジョブ時間　分(0-59)
    uint16_t    JobSecond;                  //!< ジョブ時間　秒(0-59)
    char        SoftwareId[ 41 ];           //!< ソフトウェアID (最後のバイトはゼロが必須).
    uint16_t    VersionNumber;              //!< ソフトウェアバージョン    VersionNumber * 100になる.
    uint8_t     VersionLetter;              //!< ソフトウェアバージョン
    uint32_t    KeyColor;                   //!< キーカラー.
    uint16_t    PixelNumerator;             //!< ピクセル比分子　ピクセル横幅.
    uint16_t    PixelDenominator;           //!< ピクセル比分母　ピクセル縦幅.
    uint16_t    GammaNumerator;             //!< ガンマ値分子.
    uint16_t    GammaDenominator;           //!< ガンマ値分母
    uint32_t    ColorCorrectionOffset;      //!< 色補正テーブルへのオフセット.
    uint32_t    StampOffset;                //!< ポステージスタンプ画像へのオフセット.
    uint32_t    ScanLineOffset;             //!< スキャンラインオフセット.
    uint8_t     AttributeType;              //!< アルファチャンネルデータのタイプ
};
#pragma pack( pop )

////////////////////////////////////////////////////////////////////////////////////////////
// RGBE structure
////////////////////////////////////////////////////////////////////////////////////////////
struct RGBE
{
    union
    {
        struct
        {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t e;
        };
        uint8_t v[4];
    };
};


//------------------------------------------------------------------------------------------
//      RGBE形式からVector3形式に変換します.
//------------------------------------------------------------------------------------------
inline asdx::Vector3 RGBEToVec3( const RGBE& val )
{
    asdx::Vector3 result;
    if ( val.e )
    {
        auto f = ldexp( 1.0, static_cast<int>(val.e - (128+8)) );
        result.x = static_cast<float>( val.r * f );
        result.y = static_cast<float>( val.g * f );
        result.z = static_cast<float>( val.b * f );
    }
    else 
    {
        result.x = 0.0f;
        result.y = 0.0f;
        result.z = 0.0f;
    }
    return result;
}

//------------------------------------------------------------------------------------------
//      Vector3形式からRGBE形式に変換します.
//------------------------------------------------------------------------------------------
inline RGBE Vec3ToRGBE( const asdx::Vector3& val )
{
    RGBE result = {};
    double d = asdx::Max( val.x, asdx::Max( val.y, val.z ) );

    if ( d <= DBL_EPSILON )
    {
        result.r = 0;
        result.g = 0;
        result.b = 0;
        result.e = 0;
        return result;
    }

    int e;
    double m = frexp(d, &e); // d = m * 2^e
    d = m * 256.0 / d;

    result.r = static_cast<uint32_t>(val.x * d);
    result.g = static_cast<uint32_t>(val.y * d);
    result.b = static_cast<uint32_t>(val.z * d);
    result.e = static_cast<uint32_t>(e + 128);
    return result;
}

//-------------------------------------------------------------------------------------------------
// Global Variables.
//-------------------------------------------------------------------------------------------------
static bool         g_WIC2               = false;
static WICTranslate g_WICFormats[]       = {
    { GUID_WICPixelFormat128bppRGBAFloat,       DXGI_FORMAT_R32G32B32A32_FLOAT },

    { GUID_WICPixelFormat64bppRGBAHalf,         DXGI_FORMAT_R16G16B16A16_FLOAT },
    { GUID_WICPixelFormat64bppRGBA,             DXGI_FORMAT_R16G16B16A16_UNORM },

    { GUID_WICPixelFormat32bppRGBA,             DXGI_FORMAT_R8G8B8A8_UNORM },
    { GUID_WICPixelFormat32bppBGRA,             DXGI_FORMAT_B8G8R8A8_UNORM }, // DXGI 1.1
    { GUID_WICPixelFormat32bppBGR,              DXGI_FORMAT_B8G8R8X8_UNORM }, // DXGI 1.1

    { GUID_WICPixelFormat32bppRGBA1010102XR,    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM }, // DXGI 1.1
    { GUID_WICPixelFormat32bppRGBA1010102,      DXGI_FORMAT_R10G10B10A2_UNORM },

    { GUID_WICPixelFormat16bppBGRA5551,         DXGI_FORMAT_B5G5R5A1_UNORM },
    { GUID_WICPixelFormat16bppBGR565,           DXGI_FORMAT_B5G6R5_UNORM },

    { GUID_WICPixelFormat32bppGrayFloat,        DXGI_FORMAT_R32_FLOAT },
    { GUID_WICPixelFormat16bppGrayHalf,         DXGI_FORMAT_R16_FLOAT },
    { GUID_WICPixelFormat16bppGray,             DXGI_FORMAT_R16_UNORM },
    { GUID_WICPixelFormat8bppGray,              DXGI_FORMAT_R8_UNORM },

    { GUID_WICPixelFormat8bppAlpha,             DXGI_FORMAT_A8_UNORM },
};

static WICConvert g_WICConvert[] = {
    // Note target GUID in this conversion table must be one of those directly supported formats (above).

    { GUID_WICPixelFormatBlackWhite,            GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM

    { GUID_WICPixelFormat1bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
    { GUID_WICPixelFormat2bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
    { GUID_WICPixelFormat4bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
    { GUID_WICPixelFormat8bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM

    { GUID_WICPixelFormat2bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM
    { GUID_WICPixelFormat4bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM

    { GUID_WICPixelFormat16bppGrayFixedPoint,   GUID_WICPixelFormat16bppGrayHalf }, // DXGI_FORMAT_R16_FLOAT
    { GUID_WICPixelFormat32bppGrayFixedPoint,   GUID_WICPixelFormat32bppGrayFloat }, // DXGI_FORMAT_R32_FLOAT

    { GUID_WICPixelFormat16bppBGR555,           GUID_WICPixelFormat16bppBGRA5551 }, // DXGI_FORMAT_B5G5R5A1_UNORM

    { GUID_WICPixelFormat32bppBGR101010,        GUID_WICPixelFormat32bppRGBA1010102 }, // DXGI_FORMAT_R10G10B10A2_UNORM

    { GUID_WICPixelFormat24bppBGR,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
    { GUID_WICPixelFormat24bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
    { GUID_WICPixelFormat32bppPBGRA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
    { GUID_WICPixelFormat32bppPRGBA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM

    { GUID_WICPixelFormat48bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat48bppBGR,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat64bppBGRA,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat64bppPRGBA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat64bppPBGRA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM

    { GUID_WICPixelFormat48bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
    { GUID_WICPixelFormat48bppBGRFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
    { GUID_WICPixelFormat64bppRGBAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
    { GUID_WICPixelFormat64bppBGRAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
    { GUID_WICPixelFormat64bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
    { GUID_WICPixelFormat64bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
    { GUID_WICPixelFormat48bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT

    { GUID_WICPixelFormat128bppPRGBAFloat,      GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
    { GUID_WICPixelFormat128bppRGBFloat,        GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
    { GUID_WICPixelFormat128bppRGBAFixedPoint,  GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
    { GUID_WICPixelFormat128bppRGBFixedPoint,   GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
    { GUID_WICPixelFormat32bppRGBE,             GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT

    { GUID_WICPixelFormat32bppCMYK,             GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
    { GUID_WICPixelFormat64bppCMYK,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat40bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat80bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
    { GUID_WICPixelFormat32bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
    { GUID_WICPixelFormat64bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat64bppPRGBAHalf,        GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
#endif

    // We don't support n-channel formats
};

#if 1
// Viewerアプリ用カスタマイズ処理
//  テクスチャフェッチを正しく行うために，1チャンネルのものを強制的に4チャンネルフォーマットに変換する.
static WICConvert g_WICCustomConvert[] = {
    // Note target GUID in this conversion table must be one of those directly supported formats (above).

    { GUID_WICPixelFormatBlackWhite,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8_UNORM

    { GUID_WICPixelFormat2bppGray,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8_UNORM
    { GUID_WICPixelFormat4bppGray,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8_UNORM

    { GUID_WICPixelFormat16bppGrayFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16_FLOAT
    { GUID_WICPixelFormat32bppGrayFixedPoint,   GUID_WICPixelFormat32bppGrayFloat }, // DXGI_FORMAT_R32_FLOAT

    { GUID_WICPixelFormat32bppGrayFloat,        GUID_WICPixelFormat128bppRGBAFloat },
    { GUID_WICPixelFormat16bppGrayHalf,         GUID_WICPixelFormat64bppRGBAHalf },
    { GUID_WICPixelFormat16bppGray,             GUID_WICPixelFormat64bppRGBAHalf },
    { GUID_WICPixelFormat8bppGray,              GUID_WICPixelFormat32bppRGBA },

    { GUID_WICPixelFormat8bppAlpha,             GUID_WICPixelFormat32bppRGBA },
};
#endif

//-------------------------------------------------------------------------------------------------
static IWICImagingFactory* GetWIC()
{
    static IWICImagingFactory* s_Factory = nullptr;

    if ( s_Factory )
        return s_Factory;

  #if(_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory2,
        nullptr,
        CLSCTX_INPROC_SERVER,
        __uuidof(IWICImagingFactory2),
        (LPVOID*)&s_Factory
        );

    if ( SUCCEEDED(hr) )
    {
        // WIC2 is available on Windows 8 and Windows 7 SP1 with KB 2670838 installed
        g_WIC2 = true;
    }
    else
    {
        hr = CoCreateInstance(
            CLSID_WICImagingFactory1,
            nullptr,
            CLSCTX_INPROC_SERVER,
            __uuidof(IWICImagingFactory),
            (LPVOID*)&s_Factory
            );

        if ( FAILED(hr) )
        {
            s_Factory = nullptr;
            return nullptr;
        }
    }
  #else
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        __uuidof(IWICImagingFactory),
        (LPVOID*)&s_Factory
        );

    if ( FAILED(hr) )
    {
        s_Factory = nullptr;
        return nullptr;
    }
  #endif

    return s_Factory;
}

//-------------------------------------------------------------------------------------------------
static DXGI_FORMAT WICToDXGI( const GUID& guid )
{
    for( size_t i=0; i < _countof(g_WICFormats); ++i )
    {
        if ( memcmp( &g_WICFormats[i].wic, &guid, sizeof(GUID) ) == 0 )
            return g_WICFormats[i].format;
    }

  #if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
    if ( g_WIC2 )
    {
        if ( memcmp( &GUID_WICPixelFormat96bppRGBFloat, &guid, sizeof(GUID) ) == 0 )
            return DXGI_FORMAT_R32G32B32_FLOAT;
    }
  #endif

    return DXGI_FORMAT_UNKNOWN;
}

//-------------------------------------------------------------------------------------------------
static size_t WICBitsPerPixel( REFGUID targetGuid )
{
    IWICImagingFactory* pWIC = GetWIC();
    if ( !pWIC )
        return 0;

    ComPtr<IWICComponentInfo> cinfo;
    if ( FAILED( pWIC->CreateComponentInfo( targetGuid, cinfo.GetAddressOf() ) ) )
        return 0;

    WICComponentType type;
    if ( FAILED( cinfo->GetComponentType( &type ) ) )
        return 0;

    if ( type != WICPixelFormat )
        return 0;

    ComPtr<IWICPixelFormatInfo> pfinfo;
    if ( FAILED( cinfo.As( &pfinfo ) ) )
        return 0;

    UINT bpp;
    if ( FAILED( pfinfo->GetBitsPerPixel( &bpp ) ) )
        return 0;

    return bpp;
}


//-------------------------------------------------------------------------------------------------
static DXGI_FORMAT MakeSRGB( _In_ DXGI_FORMAT format )
{
    switch( format )
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    case DXGI_FORMAT_BC1_UNORM:
        return DXGI_FORMAT_BC1_UNORM_SRGB;

    case DXGI_FORMAT_BC2_UNORM:
        return DXGI_FORMAT_BC2_UNORM_SRGB;

    case DXGI_FORMAT_BC3_UNORM:
        return DXGI_FORMAT_BC3_UNORM_SRGB;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

    case DXGI_FORMAT_B8G8R8X8_UNORM:
        return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

    case DXGI_FORMAT_BC7_UNORM:
        return DXGI_FORMAT_BC7_UNORM_SRGB;

    default:
        return format;
    }
}


//-------------------------------------------------------------------------------------------------
//      マスクをチェックします.
//-------------------------------------------------------------------------------------------------
bool CheckMask
(
    const DDPixelFormat& pixelFormat,
    unsigned int maskR,
    unsigned int maskG,
    unsigned int maskB,
    unsigned int maskA
)
{
    if ( ( pixelFormat.maskR == maskR )
      && ( pixelFormat.maskG == maskG )
      && ( pixelFormat.maskB == maskB )
      && ( pixelFormat.maskA == maskA ) )
    { return true; }

    return false;
}

size_t GetBitPerPixel( unsigned int format )
{
    switch( format )
    {
    case NATIVE_TEXTURE_FORMAT_R8:
    case NATIVE_TEXTURE_FORMAT_A8:
        { return 8; }

    case NATIVE_TEXTURE_FORMAT_ARGB_8888:
    case NATIVE_TEXTURE_FORMAT_ABGR_8888:
    case NATIVE_TEXTURE_FORMAT_XRGB_8888:
    case NATIVE_TEXTURE_FORMAT_XBGR_8888:
        { return 32; }

    case NATIVE_TEXTURE_FORMAT_BC1:
    case NATIVE_TEXTURE_FORMAT_BC4U:
    case NATIVE_TEXTURE_FORMAT_BC4S:
        { return 4; }

    case NATIVE_TEXTURE_FORMAT_BC2:
    case NATIVE_TEXTURE_FORMAT_BC3:
    case NATIVE_TEXTURE_FORMAT_BC5U:
    case NATIVE_TEXTURE_FORMAT_BC5S:
        { return 8; }

    case NATIVE_TEXTURE_FORMAT_R16_FLOAT:
        { return 16; }

    case NATIVE_TEXTURE_FORMAT_G16R16_FLOAT:
        { return 32; }

    case NATIVE_TEXTURE_FORMAT_A16B16G16R16_FLOAT:
        { return 64; }

    case NATIVE_TEXTURE_FORMAT_R32_FLOAT:
        { return 32; }

    case NATIVE_TEXTURE_FORMAT_G32R32_FLOAT:
        { return 64; }

    case NATIVE_TEXTURE_FORMAT_A32B32G32R32_FLOAT:
        { return 128; }

    default:
        { return 0; }
    }
}

//-------------------------------------------------------------------------------------------------
//      最大値を取得します.
//-------------------------------------------------------------------------------------------------
template<typename T>
T Max( const T a, const T b )
{ return ( a > b ) ? a : b; }

//-------------------------------------------------------------------------------------------------
//      ワイド文字列に変換します.
//-------------------------------------------------------------------------------------------------
std::wstring ToStringW( const std::string& value )
{
    auto length = MultiByteToWideChar(CP_ACP, 0, value.c_str(), int(value.size() + 1), nullptr, 0 );
    auto buffer = new wchar_t[length];

    MultiByteToWideChar(CP_ACP, 0, value.c_str(), int(value.size() + 1),  buffer, length );

    std::wstring result( buffer );
    delete[] buffer;

    return result;
}

//-------------------------------------------------------------------------------------------------
//      ファイルパスから拡張子を取得します.
//-------------------------------------------------------------------------------------------------
std::wstring GetExtW( const wchar_t* filePath )
{
    std::wstring path = filePath;
    size_t idx = path.find_last_of( L"." );
    if ( idx != std::wstring::npos )
    {
        std::wstring result = path.substr( idx + 1 );

        // 小文字化.
        std::transform( result.begin(), result.end(), result.begin(), tolower );

        return result;
    }

    return std::wstring();
}

//-------------------------------------------------------------------------------------------------
//      ファイルパスから拡張子を取得します.
//-------------------------------------------------------------------------------------------------
std::string GetExtA( const char* filePath )
{
    std::string path = filePath;
    size_t idx = path.find_last_of( "." );
    if ( idx != std::string::npos )
    {
        std::string result = path.substr( idx + 1 );

        // 小文字化.
        std::transform( result.begin(), result.end(), result.begin(), tolower );

        return result;
    }

    return std::string();
}

//-------------------------------------------------------------------------------------------------
//! @brief      8Bitインデックスカラー形式を解析します.
//!
//! @param[in]      pColorMap       カラーマップです.
//-------------------------------------------------------------------------------------------------
void Parse8Bits( FILE* pFile, uint32_t size, uint8_t* pColorMap, uint8_t* pPixels )
{
    uint8_t color = 0;
    for( uint32_t i=0; i<size; ++i )
    {
        color = (uint8_t)fgetc( pFile );
        pPixels[ i * 4 + 2 ] = pColorMap[ color * 3 + 0 ];
        pPixels[ i * 4 + 1 ] = pColorMap[ color * 3 + 1 ];
        pPixels[ i * 4 + 0 ] = pColorMap[ color * 3 + 2 ];
        pPixels[ i * 4 + 3 ] = 255;
    }
}

//-------------------------------------------------------------------------------------------------
//! @brief      16Bitフルカラー形式を解析します.
//-------------------------------------------------------------------------------------------------
void Parse16Bits( FILE* pFile, uint32_t size, uint8_t* pPixels )
{
    for( uint32_t i=0; i<size; ++i )
    {
        uint16_t color = fgetc( pFile ) + ( fgetc( pFile ) << 8 );
        pPixels[ i * 4 + 0 ] = (uint8_t)(( ( color & 0x7C00 ) >> 10 ) << 3);
        pPixels[ i * 4 + 1 ] = (uint8_t)(( ( color & 0x03E0 ) >>  5 ) << 3);
        pPixels[ i * 4 + 2 ] = (uint8_t)(( ( color & 0x001F ) >>  0 ) << 3);
        pPixels[ i * 4 + 3 ] = 255;
    }
}

//-------------------------------------------------------------------------------------------------
//! @brief      24Bitフルカラー形式を解析します.
//-------------------------------------------------------------------------------------------------
void Parse24Bits( FILE* pFile, uint32_t size, uint8_t* pPixels )
{
    for( uint32_t i=0; i<size; ++i )
    {
        pPixels[ i * 4 + 2 ] = (uint8_t)fgetc( pFile );
        pPixels[ i * 4 + 1 ] = (uint8_t)fgetc( pFile );
        pPixels[ i * 4 + 0 ] = (uint8_t)fgetc( pFile );
        pPixels[ i * 4 + 3 ] = 255;
    }
}

//-------------------------------------------------------------------------------------------------
//! @brief      32Bitフルカラー形式を解析します.
//-------------------------------------------------------------------------------------------------
void Parse32Bits( FILE* pFile, uint32_t size, uint8_t* pPixels )
{
    for( uint32_t i=0; i<size; ++i )
    {
        pPixels[ i * 4 + 2 ] = (uint8_t)fgetc( pFile );
        pPixels[ i * 4 + 1 ] = (uint8_t)fgetc( pFile );
        pPixels[ i * 4 + 0 ] = (uint8_t)fgetc( pFile );
        pPixels[ i * 4 + 3 ] = (uint8_t)fgetc( pFile );
    }
}

//-------------------------------------------------------------------------------------------------
//! @brief     8Bitグレースケール形式を解析します.
//-------------------------------------------------------------------------------------------------
void Parse8BitsGrayScale( FILE* pFile, uint32_t size, uint8_t* pPixels )
{
    for( uint32_t i=0; i<size; ++i )
    {
        pPixels[ i ] = (uint8_t)fgetc( pFile );
    }
}

//-------------------------------------------------------------------------------------------------
//! @brief      16Bitグレースケール形式を解析します.
//-------------------------------------------------------------------------------------------------
void Parse16BitsGrayScale( FILE* pFile, uint32_t size, uint8_t* pPixels )
{
    for( uint32_t i=0; i<size; ++i )
    {
        pPixels[ i * 2 + 0 ] = (uint8_t)fgetc( pFile );
        pPixels[ i * 2 + 1 ] = (uint8_t)fgetc( pFile );
    }
}

//-------------------------------------------------------------------------------------------------
//! @brief      8BitRLE圧縮インデックスカラー形式を解析します.
//!
//! @param[in]  pColorMap       カラーマップです.
//-------------------------------------------------------------------------------------------------
void Parse8BitsRLE( FILE* pFile, uint8_t* pColorMap, uint32_t size, uint8_t* pPixels )
{
    uint32_t count  = 0;
    uint8_t  color  = 0;
    uint8_t  header = 0;
    uint8_t* ptr    = pPixels;

    while( ptr < pPixels + size )   // size = width * height * 3.
    {
        header = (uint8_t)fgetc( pFile );
        count = 1 + ( header & 0x7F );

        if ( header & 0x80 )
        {
            color = (uint8_t)fgetc( pFile );

            //for( uint32_t i=0; i<count; ++i, ptr+=3 )
            for( uint32_t i=0; i<count; ++i, ptr+=4 )
            {
                ptr[ 0 ] = pColorMap[ color * 3 + 2 ];
                ptr[ 1 ] = pColorMap[ color * 3 + 1 ];
                ptr[ 2 ] = pColorMap[ color * 3 + 0 ];
                ptr[ 3 ] = 255;
            }
        }
        else
        {
            //for( uint32_t i=0; i<count; ++i, ptr+=3 )
            for( uint32_t i=0; i<count; ++i, ptr+=4 )
            {
                color = (uint8_t)fgetc( pFile );

                ptr[ 0 ] = pColorMap[ color * 3 + 2 ];
                ptr[ 1 ] = pColorMap[ color * 3 + 1 ];
                ptr[ 2 ] = pColorMap[ color * 3 + 0 ];
                ptr[ 3 ] = 255;
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
//! @brief      16BitRLE圧縮フルカラー形式を解析します.
//-------------------------------------------------------------------------------------------------
void Parse16BitsRLE( FILE* pFile, uint32_t size, uint8_t* pPixels )
{
    uint32_t count  = 0;
    uint16_t color  = 0;
    uint8_t  header = 0;
    uint8_t* ptr    = pPixels;

    while( ptr < pPixels + size )   // size = width * height * 3.
    {
        header = (uint8_t)fgetc( pFile );
        count = 1 + ( header & 0x7F );

        if ( header & 0x80 )
        {
            color = fgetc( pFile ) + ( fgetc( pFile ) << 8 ); 

            //for( uint32_t i=0; i<count; ++i, ptr+=3 )
            for( uint32_t i=0; i<count; ++i, ptr+=4 )
            {
                ptr[ 0 ] = (uint8_t)(( ( color & 0x7C00 ) >> 10 ) << 3);
                ptr[ 1 ] = (uint8_t)(( ( color & 0x03E0 ) >>  5 ) << 3);
                ptr[ 2 ] = (uint8_t)(( ( color & 0x001F ) >>  0 ) << 3);
                ptr[ 3 ] = 255;
            }
        }
        else
        {
            //for( uint32_t i=0; i<count; ++i, ptr+=3 )
            for( uint32_t i=0; i<count; ++i, ptr+=4 )
            {
                color = fgetc( pFile ) + ( fgetc( pFile ) << 8 );

                ptr[ 0 ] = (uint8_t)(( ( color & 0x7C00 ) >> 10 ) << 3);
                ptr[ 1 ] = (uint8_t)(( ( color & 0x03E0 ) >>  5 ) << 3);
                ptr[ 2 ] = (uint8_t)(( ( color & 0x001F ) >>  0 ) << 3);
                ptr[ 3 ] = 255;
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
//! @brief      24BitRLE圧縮フルカラー形式を解析します.
//-------------------------------------------------------------------------------------------------
void Parse24BitsRLE( FILE* pFile, uint32_t size, uint8_t* pPixels )
{
    uint32_t count    = 0;
    uint8_t  color[3] = { 0, 0, 0 };
    uint8_t  header   = 0;
    uint8_t* ptr      = pPixels;

    while( ptr < pPixels + size )   // size = width * height * 3.
    {
        header = (uint8_t)fgetc( pFile );
        count = 1 + ( header & 0x7F );

        if ( header & 0x80 )
        {
            fread( color, sizeof(uint8_t), 3, pFile );

            //for( uint32_t i=0; i<count; ++i, ptr+=3 )
            for( uint32_t i=0; i<count; ++i, ptr+=4 )
            {
                ptr[ 0 ] = color[ 2 ];
                ptr[ 1 ] = color[ 1 ];
                ptr[ 2 ] = color[ 0 ];
                ptr[ 3 ] = 255;
            }
        }
        else
        {
            //for( uint32_t i=0; i<count; ++i, ptr+=3 )
            for( uint32_t i=0; i<count; ++i, ptr+=4 )
            {
                ptr[ 2 ] = (uint8_t)fgetc( pFile );
                ptr[ 1 ] = (uint8_t)fgetc( pFile );
                ptr[ 0 ] = (uint8_t)fgetc( pFile );
                ptr[ 3 ] = 255;
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
//! @brief      32BitRLE圧縮フルカラー形式を解析します.
//-------------------------------------------------------------------------------------------------
void Parse32BitsRLE( FILE* pFile, uint32_t size, uint8_t* pPixels )
{
    uint32_t count    = 0;
    uint8_t  color[4] = { 0, 0, 0, 0 };
    uint8_t  header   = 0;
    uint8_t* ptr      = pPixels;

    while( ptr < pPixels + size )   // size = width * height * 4.
    {
        header = (uint8_t)fgetc( pFile );
        count = 1 + ( header & 0x7F );

        if ( header & 0x80 )
        {
            fread( color, sizeof(uint8_t), 4, pFile );

            for( uint32_t i=0; i<count; ++i, ptr+=4 )
            {
                ptr[ 0 ] = color[ 2 ];
                ptr[ 1 ] = color[ 1 ];
                ptr[ 2 ] = color[ 0 ];
                ptr[ 3 ] = color[ 3 ];
            }
        }
        else
        {
            for( uint32_t i=0; i<count; ++i, ptr+=4 )
            {
                ptr[ 2 ] = (uint8_t)fgetc( pFile );
                ptr[ 1 ] = (uint8_t)fgetc( pFile );
                ptr[ 0 ] = (uint8_t)fgetc( pFile );
                ptr[ 3 ] = (uint8_t)fgetc( pFile );
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
//! @brief      8BitRLE圧縮グレースケール形式を解析します.
//-------------------------------------------------------------------------------------------------
void Parse8BitsGrayScaleRLE( FILE* pFile, uint32_t size, uint8_t* pPixles )
{
    uint32_t count  = 0;
    uint8_t  color  = 0;
    uint8_t  header = 0;
    uint8_t* ptr    = pPixles;

    while( ptr < pPixles + size ) // size = width * height
    {
        header = (uint8_t)fgetc( pFile );
        count = 1 + ( header & 0x7F );

        if ( header & 0x80 )
        {
            color = (uint8_t)fgetc( pFile );

            for( uint32_t i=0; i<count; ++i, ptr++ )
            { (*ptr) = color; }
        }
        else
        {
            for( uint32_t i=0; i<count; ++i, ptr++ )
            { (*ptr) = (uint8_t)fgetc( pFile ); }
        }
    }
}

//-------------------------------------------------------------------------------------------------
//! @brief      16BitRLE圧縮グレースケール形式を解析します.
//-------------------------------------------------------------------------------------------------
void Parse16BitsGrayScaleRLE( FILE* pFile, uint32_t size, uint8_t* pPixles )
{
    uint32_t count  = 0;
    uint8_t  color  = 0;
    uint8_t  alpha  = 0;
    uint8_t  header = 0;
    uint8_t* ptr    = pPixles;

    while( ptr < pPixles + size ) // size = width * height * 2
    {
        header = (uint8_t)fgetc( pFile );
        count = 1 + ( header & 0x7F );

        if ( header & 0x80 )
        {
            color = (uint8_t)fgetc( pFile );
            alpha = (uint8_t)fgetc( pFile );

            for( uint32_t i=0; i<count; ++i, ptr+=2 )
            {
                ptr[ 0 ] = color;
                ptr[ 1 ] = alpha;
            }
        }
        else
        {
            for( uint32_t i=0; i<count; ++i, ptr+=2 )
            {
                ptr[ 0 ] = (uint8_t)fgetc( pFile );
                ptr[ 1 ] = (uint8_t)fgetc( pFile );
            }
        }
    }
}

} // namespace /* anonymous */


namespace asdx {

//-------------------------------------------------------------------------------------------------
//      WICからリソーステクスチャを生成します.
//      (BMP, JPEG, PNG, TIFF, GIF, HD Photoファイルからリソーステクスチャを生成します).
//-------------------------------------------------------------------------------------------------
bool CreateResTextureFromWICFileW( const wchar_t* filename, asdx::ResTexture& resTexture )
{
    bool forceSRGB = false;

    if ( filename == nullptr )
    { return false; }

    IWICImagingFactory* pWIC = GetWIC();
    if ( !pWIC )
    { return false; }

    // WICの初期化.
    ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr = pWIC->CreateDecoderFromFilename(
        filename, 0, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf() );
    if ( FAILED( hr ) )
    { return false; }

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame( 0, frame.GetAddressOf() );
    if ( FAILED( hr ) )
    { return false; }

    uint32_t width  = 0;
    uint32_t height = 0;
    hr = frame->GetSize( &width, &height );
    if ( FAILED( hr ) )
    { return false; }

    assert( width > 0 && height > 0 );

    uint32_t origWidth  = width;
    uint32_t origHeight = height;

    if ( width  > MAX_TEXTURE_SIZE
      || height > MAX_TEXTURE_SIZE )
    {
        float ar = float( height ) / float( width );
        if ( width > height )
        {
            width  = MAX_TEXTURE_SIZE;
            height = uint32_t( float( MAX_TEXTURE_SIZE ) * ar );
        }
        else
        {
            width = uint32_t( float( MAX_TEXTURE_SIZE ) / ar );
            height = MAX_TEXTURE_SIZE;
        }
    }

    WICPixelFormatGUID pixelFormat;
    hr = frame->GetPixelFormat( &pixelFormat );
    if ( FAILED( hr ) )
    { return false; }

    WICPixelFormatGUID convertGUID;
    memcpy( &convertGUID, &pixelFormat, sizeof(WICPixelFormatGUID) );

    size_t bpp = 0;
    DXGI_FORMAT format = WICToDXGI( pixelFormat );
    if ( format == DXGI_FORMAT_UNKNOWN )
    {
        if ( memcmp( &GUID_WICPixelFormat96bppRGBFixedPoint, &pixelFormat, sizeof(WICPixelFormatGUID) ) == 0 )
        {
        #if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
            if ( g_WIC2 )
            {
                memcpy( &convertGUID, &GUID_WICPixelFormat96bppRGBFloat, sizeof(WICPixelFormatGUID) );
                format = DXGI_FORMAT_R32G32B32_FLOAT;
            }
            else
        #endif
            {
                memcpy( &convertGUID, &GUID_WICPixelFormat128bppRGBAFloat, sizeof(WICPixelFormatGUID) );
                format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
        }
        else
        {
            for( size_t i=0; i < _countof(g_WICConvert); ++i )
            {
                if ( memcmp( &g_WICConvert[i].source, &pixelFormat, sizeof(WICPixelFormatGUID) ) == 0 )
                {
                    memcpy( &convertGUID, &g_WICConvert[i].target, sizeof(WICPixelFormatGUID) );

                    format = WICToDXGI( g_WICConvert[i].target );
                    assert( format != DXGI_FORMAT_UNKNOWN );
                    bpp = WICBitsPerPixel( convertGUID );
                    break;
                }
            }
        }

        if ( format == DXGI_FORMAT_UNKNOWN )
        { return false; }
    }
#if 0
    //  テクスチャフェッチをした結果が正しくなるようにチャンネル数を4チャンネルにする.
    else if ( format == DXGI_FORMAT_R8_UNORM
           || format == DXGI_FORMAT_A8_UNORM
           || format == DXGI_FORMAT_R16_UNORM
           || format == DXGI_FORMAT_R16_FLOAT
           || format == DXGI_FORMAT_R32_FLOAT )
    {
        for( size_t i=0; i < _countof(g_WICCustomConvert); ++i )
        {
            if ( memcmp( &g_WICCustomConvert[i].source, &pixelFormat, sizeof(WICPixelFormatGUID) ) == 0 )
            {
                memcpy( &convertGUID, &g_WICCustomConvert[i].target, sizeof(WICPixelFormatGUID) );

                format = WICToDXGI( g_WICCustomConvert[i].target );
                assert( format != DXGI_FORMAT_UNKNOWN );
                bpp = WICBitsPerPixel( convertGUID );
                break;
            }
        }

        if ( format == DXGI_FORMAT_UNKNOWN )
        { return false; }
    }
#endif
    else
    {
        bpp = WICBitsPerPixel( pixelFormat );
    }

    if ( !bpp )
    { return false; }

    // Handle sRGB formats
    if ( forceSRGB )
    {
        format = MakeSRGB( format );
    }
    else
    {
        ComPtr<IWICMetadataQueryReader> metareader;
        if ( SUCCEEDED( frame->GetMetadataQueryReader( metareader.GetAddressOf() ) ) )
        {
            GUID containerFormat;
            if ( SUCCEEDED( metareader->GetContainerFormat( &containerFormat ) ) )
            {
                // Check for sRGB colorspace metadata
                bool sRGB = false;

                PROPVARIANT value;
                PropVariantInit( &value );

                if ( memcmp( &containerFormat, &GUID_ContainerFormatPng, sizeof(GUID) ) == 0 )
                {
                    // Check for sRGB chunk
                    if ( SUCCEEDED( metareader->GetMetadataByName( L"/sRGB/RenderingIntent", &value ) ) && value.vt == VT_UI1 )
                    {
                        sRGB = true;
                    }
                }
                else if ( SUCCEEDED( metareader->GetMetadataByName( L"System.Image.ColorSpace", &value ) ) && value.vt == VT_UI2 && value.uiVal == 1 )
                {
                    sRGB = true;
                }

                PropVariantClear( &value );

                if ( sRGB )
                { format = MakeSRGB( format ); }
            }
        }
    }

    // １行当たりのバイト数.
    size_t rowPitch = ( width * bpp + 7 ) / 8;

    // ピクセルデータのサイズ.
    size_t imageSize = rowPitch * height;

    // ピクセルデータのメモリを確保.
    uint8_t* pPixels = new (std::nothrow) uint8_t [ imageSize ];
    if ( !pPixels )
    { return false; }

    // 元サイズと同じ場合.
    if ( 0 == memcmp( &convertGUID, &pixelFormat, sizeof(GUID) )
      && width  == origWidth
      && height == origHeight )
    {
        hr = frame->CopyPixels( 0, uint32_t( rowPitch ), uint32_t( imageSize ), pPixels );
        if ( FAILED( hr ) )
        {
            SafeDeleteArray( pPixels );
            return false;
        }
    }
    else if ( width != origWidth
          || height != origHeight )
    {
        // リサイズ処理.

        ComPtr<IWICBitmapScaler> scaler;
        hr = pWIC->CreateBitmapScaler( scaler.GetAddressOf() );
        if ( FAILED( hr ) )
        {
            SafeDeleteArray( pPixels );
            return false;
        }

        hr = scaler->Initialize( frame.Get(), width, height, WICBitmapInterpolationModeFant );
        if ( FAILED( hr ) )
        {
            SafeDeleteArray( pPixels );
            return false;
        }

        WICPixelFormatGUID pfScalar;
        hr = scaler->GetPixelFormat( &pfScalar );
        if ( FAILED( hr ) )
        {
            SafeDeleteArray( pPixels );
            return false;
        }

        if ( 0 == memcmp( &convertGUID, &pfScalar, sizeof(GUID) ) )
        {
            hr = scaler->CopyPixels( 0, uint32_t( rowPitch ), uint32_t( imageSize ), pPixels );
            if ( FAILED( hr ) )
            {
                SafeDeleteArray( pPixels );
                return false;
            }
        }
        else
        {
            ComPtr<IWICFormatConverter> conv;
            hr =  pWIC->CreateFormatConverter( conv.GetAddressOf() );
            if ( FAILED( hr ) )
            {
                SafeDeleteArray( pPixels );
                return false;
            }

            hr = conv->Initialize( scaler.Get(), convertGUID, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom );
            if ( FAILED( hr ) )
            {
                SafeDeleteArray( pPixels );
                return false;
            }

            hr = conv->CopyPixels( 0, uint32_t( rowPitch ), uint32_t( imageSize ), pPixels );
            if ( FAILED( hr ) )
            {
                SafeDeleteArray( pPixels );
                return false;
            }
        }
    }
    else
    {
        // フォーマットのみが違う場合.


        ComPtr<IWICFormatConverter> conv;
        hr = pWIC->CreateFormatConverter( conv.GetAddressOf() );
        if ( FAILED( hr ) )
        {
            SafeDeleteArray( pPixels );
            return false;
        }

        hr = conv->Initialize( frame.Get(), convertGUID, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom );
        if ( FAILED( hr ) )
        {
            SafeDeleteArray( pPixels );
            return false;
        }

        hr = conv->CopyPixels( 0, uint32_t( rowPitch ), uint32_t( imageSize ), pPixels );
        if ( FAILED( hr ) )
        {
            SafeDeleteArray( pPixels );
            return false;
        }
    }

    // サブリソースのメモリを確保.
    asdx::SubResource* pRes = new (std::nothrow) asdx::SubResource();
    if ( !pRes )
    {
        SafeDeleteArray( pPixels );
        return false;
    }

    // サブリソースを設定.
    pRes->Width      = width;
    pRes->Height     = height;
    pRes->MipIndex   = 0;
    pRes->Pitch      = uint32_t( rowPitch );
    pRes->SlicePitch = uint32_t( imageSize );
    pRes->pPixels    = pPixels;

    // リソーステクスチャを設定.
    resTexture.Dimension    = TEXTURE_DIMENSION_2D;
    resTexture.Width        = width;
    resTexture.Height       = height;
    resTexture.Depth        = 0;
    resTexture.Format       = uint32_t( format );
    resTexture.MipMapCount  = 1;
    resTexture.SurfaceCount = 1;
    resTexture.pResources   = pRes;

    // 正常終了.
    return true;
}

//-------------------------------------------------------------------------------------------------
//      WICからリソーステクスチャを生成します.
//      (BMP, JPEG, PNG, TIFF, GIF, HD Photoファイルからリソーステクスチャを生成します).
//-------------------------------------------------------------------------------------------------
bool CreateResTextureFromWICFileA( const char* filename, asdx::ResTexture& resTexture )
{
    auto path = ToStringW(filename);
    return CreateResTextureFromWICFileW(path.c_str(), resTexture);
}

//-------------------------------------------------------------------------------------------------
//      WICからリソーステクスチャを生成します.
//      (BMP, JPEG, PNG, TIFF, GIF, HD Photoファイルからリソーステクスチャを生成します).
//-------------------------------------------------------------------------------------------------
bool CreateResTextureFromWICMemory(const uint8_t* pBinary, uint32_t bufferSize, asdx::ResTexture& resTexture)
{
    bool forceSRGB = false;

    if ( pBinary == nullptr )
    { return false; }

    IWICImagingFactory* pWIC = GetWIC();
    if ( !pWIC )
    { return false; }

    ComPtr<IWICStream> pStream;
    HRESULT hr = pWIC->CreateStream( &pStream );
    if ( FAILED( hr ) )
    { return false; }

    hr = pStream->InitializeFromMemory( (BYTE*)pBinary, bufferSize );
    if ( FAILED( hr ) )
    { return false; }

    // WICの初期化.
    ComPtr<IWICBitmapDecoder> decoder;
    hr = pWIC->CreateDecoderFromStream( pStream.Get(), 0, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf() );
    if ( FAILED( hr ) )
    { return false; }

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame( 0, frame.GetAddressOf() );
    if ( FAILED( hr ) )
    { return false; }

    uint32_t width  = 0;
    uint32_t height = 0;
    hr = frame->GetSize( &width, &height );
    if ( FAILED( hr ) )
    { return false; }

    assert( width > 0 && height > 0 );

    uint32_t origWidth  = width;
    uint32_t origHeight = height;

    if ( width  > MAX_TEXTURE_SIZE
      || height > MAX_TEXTURE_SIZE )
    {
        float ar = float( height ) / float( width );
        if ( width > height )
        {
            width  = MAX_TEXTURE_SIZE;
            height = uint32_t( float( MAX_TEXTURE_SIZE ) * ar );
        }
        else
        {
            width = uint32_t( float( MAX_TEXTURE_SIZE ) / ar );
            height = MAX_TEXTURE_SIZE;
        }
    }

    WICPixelFormatGUID pixelFormat;
    hr = frame->GetPixelFormat( &pixelFormat );
    if ( FAILED( hr ) )
    { return false; }

    WICPixelFormatGUID convertGUID;
    memcpy( &convertGUID, &pixelFormat, sizeof(WICPixelFormatGUID) );

    size_t bpp = 0;
    DXGI_FORMAT format = WICToDXGI( pixelFormat );
    if ( format == DXGI_FORMAT_UNKNOWN )
    {
        if ( memcmp( &GUID_WICPixelFormat96bppRGBFixedPoint, &pixelFormat, sizeof(WICPixelFormatGUID) ) == 0 )
        {
        #if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
            if ( g_WIC2 )
            {
                memcpy( &convertGUID, &GUID_WICPixelFormat96bppRGBFloat, sizeof(WICPixelFormatGUID) );
                format = DXGI_FORMAT_R32G32B32_FLOAT;
            }
            else
        #endif
            {
                memcpy( &convertGUID, &GUID_WICPixelFormat128bppRGBAFloat, sizeof(WICPixelFormatGUID) );
                format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
        }
        else
        {
            for( size_t i=0; i < _countof(g_WICConvert); ++i )
            {
                if ( memcmp( &g_WICConvert[i].source, &pixelFormat, sizeof(WICPixelFormatGUID) ) == 0 )
                {
                    memcpy( &convertGUID, &g_WICConvert[i].target, sizeof(WICPixelFormatGUID) );

                    format = WICToDXGI( g_WICConvert[i].target );
                    assert( format != DXGI_FORMAT_UNKNOWN );
                    bpp = WICBitsPerPixel( convertGUID );
                    break;
                }
            }
        }

        if ( format == DXGI_FORMAT_UNKNOWN )
        { return false; }
    }
#if 0
    //  テクスチャフェッチをした結果が正しくなるようにチャンネル数を4チャンネルにする.
    else if ( format == DXGI_FORMAT_R8_UNORM
           || format == DXGI_FORMAT_A8_UNORM
           || format == DXGI_FORMAT_R16_UNORM
           || format == DXGI_FORMAT_R16_FLOAT
           || format == DXGI_FORMAT_R32_FLOAT )
    {
        for( size_t i=0; i < _countof(g_WICCustomConvert); ++i )
        {
            if ( memcmp( &g_WICCustomConvert[i].source, &pixelFormat, sizeof(WICPixelFormatGUID) ) == 0 )
            {
                memcpy( &convertGUID, &g_WICCustomConvert[i].target, sizeof(WICPixelFormatGUID) );

                format = WICToDXGI( g_WICCustomConvert[i].target );
                assert( format != DXGI_FORMAT_UNKNOWN );
                bpp = WICBitsPerPixel( convertGUID );
                break;
            }
        }

        if ( format == DXGI_FORMAT_UNKNOWN )
        { return false; }
    }
#endif
    else
    {
        bpp = WICBitsPerPixel( pixelFormat );
    }

    if ( !bpp )
    { return false; }

    // Handle sRGB formats
    if ( forceSRGB )
    {
        format = MakeSRGB( format );
    }
    else
    {
        ComPtr<IWICMetadataQueryReader> metareader;
        if ( SUCCEEDED( frame->GetMetadataQueryReader( metareader.GetAddressOf() ) ) )
        {
            GUID containerFormat;
            if ( SUCCEEDED( metareader->GetContainerFormat( &containerFormat ) ) )
            {
                // Check for sRGB colorspace metadata
                bool sRGB = false;

                PROPVARIANT value;
                PropVariantInit( &value );

                if ( memcmp( &containerFormat, &GUID_ContainerFormatPng, sizeof(GUID) ) == 0 )
                {
                    // Check for sRGB chunk
                    if ( SUCCEEDED( metareader->GetMetadataByName( L"/sRGB/RenderingIntent", &value ) ) && value.vt == VT_UI1 )
                    {
                        sRGB = true;
                    }
                }
                else if ( SUCCEEDED( metareader->GetMetadataByName( L"System.Image.ColorSpace", &value ) ) && value.vt == VT_UI2 && value.uiVal == 1 )
                {
                    sRGB = true;
                }

                PropVariantClear( &value );

                if ( sRGB )
                { format = MakeSRGB( format ); }
            }
        }
    }

    // １行当たりのバイト数.
    size_t rowPitch = ( width * bpp + 7 ) / 8;

    // ピクセルデータのサイズ.
    size_t imageSize = rowPitch * height;

    // ピクセルデータのメモリを確保.
    uint8_t* pPixels = new (std::nothrow) uint8_t [ imageSize ];
    if ( !pPixels )
    { return false; }

    // 元サイズと同じ場合.
    if ( 0 == memcmp( &convertGUID, &pixelFormat, sizeof(GUID) )
      && width  == origWidth
      && height == origHeight )
    {
        hr = frame->CopyPixels( 0, uint32_t( rowPitch ), uint32_t( imageSize ), pPixels );
        if ( FAILED( hr ) )
        {
            SafeDeleteArray( pPixels );
            return false;
        }
    }
    else if ( width != origWidth
          || height != origHeight )
    {
        // リサイズ処理.

        ComPtr<IWICBitmapScaler> scaler;
        hr = pWIC->CreateBitmapScaler( scaler.GetAddressOf() );
        if ( FAILED( hr ) )
        {
            SafeDeleteArray( pPixels );
            return false;
        }

        hr = scaler->Initialize( frame.Get(), width, height, WICBitmapInterpolationModeFant );
        if ( FAILED( hr ) )
        {
            SafeDeleteArray( pPixels );
            return false;
        }

        WICPixelFormatGUID pfScalar;
        hr = scaler->GetPixelFormat( &pfScalar );
        if ( FAILED( hr ) )
        {
            SafeDeleteArray( pPixels );
            return false;
        }

        if ( 0 == memcmp( &convertGUID, &pfScalar, sizeof(GUID) ) )
        {
            hr = scaler->CopyPixels( 0, uint32_t( rowPitch ), uint32_t( imageSize ), pPixels );
            if ( FAILED( hr ) )
            {
                SafeDeleteArray( pPixels );
                return false;
            }
        }
        else
        {
            ComPtr<IWICFormatConverter> conv;
            hr =  pWIC->CreateFormatConverter( conv.GetAddressOf() );
            if ( FAILED( hr ) )
            {
                SafeDeleteArray( pPixels );
                return false;
            }

            hr = conv->Initialize( scaler.Get(), convertGUID, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom );
            if ( FAILED( hr ) )
            {
                SafeDeleteArray( pPixels );
                return false;
            }

            hr = conv->CopyPixels( 0, uint32_t( rowPitch ), uint32_t( imageSize ), pPixels );
            if ( FAILED( hr ) )
            {
                SafeDeleteArray( pPixels );
                return false;
            }
        }
    }
    else
    {
        // フォーマットのみが違う場合.

        ComPtr<IWICFormatConverter> conv;
        hr = pWIC->CreateFormatConverter( conv.GetAddressOf() );
        if ( FAILED( hr ) )
        {
            SafeDeleteArray( pPixels );
            return false;
        }

        hr = conv->Initialize( frame.Get(), convertGUID, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom );
        if ( FAILED( hr ) )
        {
            SafeDeleteArray( pPixels );
            return false;
        }

        hr = conv->CopyPixels( 0, uint32_t( rowPitch ), uint32_t( imageSize ), pPixels );
        if ( FAILED( hr ) )
        {
            SafeDeleteArray( pPixels );
            return false;
        }
    }

    // サブリソースのメモリを確保.
    asdx::SubResource* pRes = new (std::nothrow) asdx::SubResource();
    if ( !pRes )
    {
        SafeDeleteArray( pPixels );
        return false;
    }

    // サブリソースを設定.
    pRes->Width      = width;
    pRes->Height     = height;
    pRes->MipIndex   = 0;
    pRes->Pitch      = uint32_t( rowPitch );
    pRes->SlicePitch = uint32_t( imageSize );
    pRes->pPixels    = pPixels;

    // リソーステクスチャを設定.
    resTexture.Dimension    = TEXTURE_DIMENSION_2D;
    resTexture.Width        = width;
    resTexture.Height       = height;
    resTexture.Depth        = 0;
    resTexture.Format       = uint32_t( format );
    resTexture.MipMapCount  = 1;
    resTexture.SurfaceCount = 1;
    resTexture.pResources   = pRes;

    // 正常終了.
    return true;
}

//-------------------------------------------------------------------------------------------------
//      DDSからリソーステクスチャを生成します.
//-------------------------------------------------------------------------------------------------
bool CreateResTextureFromDDSFile(FILE* pFile, asdx::ResTexture& resTexture)
{
    DDSurfaceDesc   ddsd            = {};
    char            magic[4]        = {};
    uint32_t        width           = 0;
    uint32_t        height          = 0;
    uint32_t        depth           = 0;
    uint32_t        nativeFormat    = 0;

   // マジックを読み込む.
    fread( magic, sizeof(char), 4, pFile);

    // マジックをチェック.
    if ( ( magic[0] != 'D' )
      || ( magic[1] != 'D' )
      || ( magic[2] != 'S' )
      || ( magic[3] != ' ' ) )
    {
        // エラーログ出力.
        ELOGW( "Error : Invalid File" );

        // ファイルを閉じる.
        fclose( pFile );

        // 異常終了.
        return false;
    }


    // サーフェイスデスクリプションを読み込み.
    fread( &ddsd, sizeof( DDSurfaceDesc ), 1, pFile );

    // 幅有効.
    if ( ddsd.flags & DDSD_WIDTH )
    {
        width = ddsd.width;
        resTexture.Dimension = TEXTURE_DIMENSION_1D;
    }

    // 高さ有効.
    if ( ddsd.flags & DDSD_HEIGHT )
    {
        height = ddsd.height;
        resTexture.Dimension = TEXTURE_DIMENSION_2D;
    }

    // 奥行有効.
    if ( ddsd.flags & DDSD_DEPTH )
    { depth = ddsd.depth; }

    resTexture.Width        = width;
    resTexture.Height       = height;
    resTexture.Depth        = 0;
    resTexture.SurfaceCount = 1;
    resTexture.MipMapCount  = 1;

    // ミップマップ数.有効
    if ( ddsd.flags & DDSD_MIPMAPCOUNT )
    { resTexture.MipMapCount = ddsd.mipMapLevels; }

    // キューブマップとボリュームテクスチャのチェック.
    if ( ddsd.caps & DDSCAPS_COMPLEX )
    {
        // キューブマップの場合.
        if ( ddsd.caps2 & DDSCAPS2_CUBEMAP )
        {
            unsigned int surfaceCount = 0;

            // サーフェイス数をチェック.
            if ( ddsd.caps2 & DDSCAPS2_CUBEMAP_POSITIVE_X ) { surfaceCount++; }
            if ( ddsd.caps2 & DDSCAPS2_CUBEMAP_NEGATIVE_X ) { surfaceCount++; }
            if ( ddsd.caps2 & DDSCAPS2_CUBEMAP_POSITIVE_Y ) { surfaceCount++; }
            if ( ddsd.caps2 & DDSCAPS2_CUBEMAP_NEGATIVE_Y ) { surfaceCount++; }
            if ( ddsd.caps2 & DDSCAPS2_CUBEMAP_POSITIVE_Z ) { surfaceCount++; }
            if ( ddsd.caps2 & DDSCAPS2_CUBEMAP_NEGATIVE_Z ) { surfaceCount++; }

            // 一応チェック.
            assert( surfaceCount == 6 );

            // サーフェイス数を設定.
            resTexture.SurfaceCount = surfaceCount;
            resTexture.Dimension    = TEXTURE_DIMENSION_CUBE;
            resTexture.Depth        = 1;
        }
        // ボリュームテクスチャの場合.
        else if ( ddsd.caps2 & DDSCAPS2_VOLUME )
        {
            // 奥行の値を設定.
            resTexture.Depth = depth;
            resTexture.Dimension = TEXTURE_DIMENSION_3D;
        }
    }

    // サポートフォーマットのチェックフラグ.
    bool isSupportFormat = false;

    // ピクセルフォーマット有効.
    if ( ddsd.flags & DDSD_PIXELFORMAT )
    {
        // dwFourCC有効
        if ( ddsd.pixelFormat.flags & DDPF_FOURCC )
        {
            switch( ddsd.pixelFormat.fourCC )
            {
            case FOURCC_DXT1:
                {
                    resTexture.Format = DXGI_FORMAT_BC1_UNORM_SRGB;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC1;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_DXT2:
                {
                    resTexture.Format = DXGI_FORMAT_BC2_UNORM_SRGB;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC2;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_DXT3:
                {
                    resTexture.Format = DXGI_FORMAT_BC2_UNORM_SRGB;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC2;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_DXT4:
                {
                    resTexture.Format = DXGI_FORMAT_BC3_UNORM_SRGB;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC3;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_DXT5:
                {
                    resTexture.Format = DXGI_FORMAT_BC3_UNORM_SRGB;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC3;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_ATI1:
            case FOURCC_BC4U:
                {
                    resTexture.Format = DXGI_FORMAT_BC4_UNORM;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC4U;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_BC4S:
                {
                    resTexture.Format = DXGI_FORMAT_BC5_SNORM;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC4S;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_ATI2:
            case FOURCC_BC5U:
                {
                    resTexture.Format = DXGI_FORMAT_BC5_UNORM;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC5U;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_BC5S:
                {
                    resTexture.Format = DXGI_FORMAT_BC5_SNORM;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC5S;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_DX10:
                {
                }
                break;

            case 36: // D3DFMT_A16B16G16R16
                {
                    // DXGI_FORMAT_R16G61B16A16_UNORM
                }
                break;

            case 110: // D3DFMT_W16W16V16U16
                {
                    // DXGI_FORMAT_R16G16B16A16_SNORM;
                }
                break;

            case 111: // D3DFMT_R16F
                {
                    // DXGI_FORMAT_R16_FLOAT
                    resTexture.Format = DXGI_FORMAT_R16_FLOAT;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_R16_FLOAT;
                    isSupportFormat   = true;
                }
                break;

            case 112: // D3DFMT_G16R16F
                {
                    // DXGI_FORMAT_R16G16_FLOAT
                    resTexture.Format = DXGI_FORMAT_R16G16_FLOAT;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_G16R16_FLOAT;
                    isSupportFormat   = true;
                }
                break;

            case 113: // D3DFMT_A16B16G16R16F
                {
                    // DXGI_FORMAT_R16G16B16A16_FLOAT
                    resTexture.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_A16B16G16R16_FLOAT;
                    isSupportFormat   = true;
                }
                break;

            case 114: // D3DFMT_R32F
                {
                    // DXGI_FORMAT_R32_FLOAT
                    resTexture.Format = DXGI_FORMAT_R32_FLOAT;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_R32_FLOAT;
                    isSupportFormat   = true;
                }
                break;

            case 115: // D3DFMT_G32R32F
                {
                    // DXGI_FORMAT_R32G32_FLOAT
                    resTexture.Format = DXGI_FORMAT_R32G32_FLOAT;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_G32R32_FLOAT;
                    isSupportFormat   = true;
                }
                break;

            case 116: // D3DFMT_A32B32G32R32F
                {
                    // DXGI_FORMAT_R32G32B32A32_FLOAT
                    resTexture.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_A32B32G32R32_FLOAT;
                    isSupportFormat   = true;
                }
                break;

            default:
                {
                    isSupportFormat = false;
                }
                break;
            }
        }
        else if ( ddsd.pixelFormat.flags & DDPF_RGB )
        {
            switch( ddsd.pixelFormat.bpp )
            {
            case 32:
                {
                    if ( CheckMask( ddsd.pixelFormat, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 ) )
                    {
                        // A8 R8 G8 B8
                        isSupportFormat   = true;
                        resTexture.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                        nativeFormat      = NATIVE_TEXTURE_FORMAT_ARGB_8888;
                    }

                    if ( CheckMask( ddsd.pixelFormat, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 ) )
                    {
                        // A8 B8 G8 R8
                        isSupportFormat   = true;
                        resTexture.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                        nativeFormat      = NATIVE_TEXTURE_FORMAT_ABGR_8888;
                    }

                    if ( CheckMask( ddsd.pixelFormat, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 ) )
                    {
                        // X8 R8 G8 B8
                        isSupportFormat   = true;
                        resTexture.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                        nativeFormat      = NATIVE_TEXTURE_FORMAT_XRGB_8888;
                    }

                    if ( CheckMask( ddsd.pixelFormat, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000 ) )
                    {
                        // X8 B8 G8 R8
                        isSupportFormat = true;
                        resTexture.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                        nativeFormat = NATIVE_TEXTURE_FORMAT_XBGR_8888;
                    }

                #if 0
                    //if ( CheckMask( ddsd.pixelFormat, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000 ) )
                    //{
                    //    // R10 G10 B10 A2
                    //    /* NOT_SUPPORT */
                    //}

                    //if ( CheckMask( ddsd.pixelFormat, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000 ) )
                    //{
                    //    // R16 G16
                    //    /* NOT_SUPPORT */
                    //}
                    //if ( CheckMask( ddsd.pixelFormat, 0xffffffff, 0x00000000,0x00000000,0x00000000 ) )
                    //{
                    //    // R32
                    //    /* NOT_SUPPORT */
                    //}
                #endif
                }
                break;

            case 24:
                {
                #if 0
                    /* NOT_SUPPORT */
                #endif
                }
                break;

            case 16:
                {
                #if 0
                    //if ( CheckMask( ddsd.pixelFormat, 0x7c00, 0x03e0, 0x001f, 0x8000 ) )
                    //{
                    //    // B5 G5 R5 A1
                    //    /* NOT_SUPPORT */
                    //}

                    //if ( CheckMask( ddsd.pixelFormat, 0xf800, 0x07e0, 0x001f, 0x0000 ) )
                    //{
                    //    // B5 G6 R5
                    //    /* NOT_SUPPORT */
                    //}
                #endif
                }
                break;
            }
        }
        else if ( ddsd.pixelFormat.flags & DDPF_LUMINANCE )
        {
            switch( ddsd.pixelFormat.bpp )
            {
            case 8:
                {
                    if ( CheckMask( ddsd.pixelFormat, 0x000000ff, 0x00000000, 0x00000000, 0x00000000 ) )
                    {
                        // R8
                        isSupportFormat   = true;
                        nativeFormat      = NATIVE_TEXTURE_FORMAT_R8;
                        resTexture.Format = DXGI_FORMAT_R8_UNORM;
                    }
                }
                break;

            case 16:
                {
                #if 0
                    //if ( CheckMask( ddsd.pixelFormat, 0x0000ffff, 0x00000000, 0x00000000, 0x00000000 ) )
                    //{
                    //    // R16
                    //    /* NOT_SUPPORT */
                    //}

                    //if ( CheckMask( ddsd.pixelFormat, 0x000000ff, 0x00000000, 0x00000000, 0x0000ff00 ) )
                    //{
                    //    // R8 G8
                    //    /* NOT_SUPPORT */
                    //}
                #endif
                }
                break;
            }
        }
        else if ( ddsd.pixelFormat.flags & DDPF_ALPHA )
        {
            if ( 8 == ddsd.pixelFormat.bpp )
            {
                // A8
                isSupportFormat   = true;
                nativeFormat      = NATIVE_TEXTURE_FORMAT_A8;
                resTexture.Format = DXGI_FORMAT_R8_UNORM;
            }
        }
    }

    // サポートフォーマットがあったかチェック.
    if ( !isSupportFormat )
    {
        // ファイルを閉じる.
        fclose( pFile );

        // エラーログ出力.
        ELOG( "Error : Unsupported Format." );

        // 異常終了.
        return false;
    }

    // 現在のファイル位置を記憶.
    long curr = ftell( pFile );

    // ファイル末端に移動.
    fseek( pFile, 0, SEEK_END );

    // ファイル末端位置を記憶.
    long end = ftell( pFile );

    // ファイル位置を元に戻す.
    fseek( pFile, curr, SEEK_SET );

    // ピクセルデータのサイズを算出.
    size_t pixelSize = end - curr;

    // ピクセルデータのメモリを確保.
    unsigned char* pPixelData = new unsigned char [pixelSize];

    // NULLチェック.
    if ( pPixelData == nullptr )
    {
        // ファイルを閉じる.
        fclose( pFile );

        // エラーログ出力.
        ELOG( "Error : Memory Allocate Failed." );

        // 異常終了.
        return false;
    }

    // ピクセルデータ読み込み.
    fread( pPixelData, sizeof( unsigned char ), pixelSize, pFile );

    // ファイルを閉じる.
    fclose( pFile );

    // リトルエンディアンなのでピクセルの並びを補正.
    switch( nativeFormat )
    {
        // 一括読み込みでくるっているので修正.
        case NATIVE_TEXTURE_FORMAT_ARGB_8888:
        case NATIVE_TEXTURE_FORMAT_XRGB_8888:
        {
            for( size_t i=0; i<pixelSize; i+=4 )
            {
                // BGRA -> RGBA
                unsigned char R = pPixelData[ i + 0 ];
                unsigned char B = pPixelData[ i + 2 ];
                pPixelData[ i + 0 ] = B;
                pPixelData[ i + 2 ] = R;
            }
        }
        break;
    }

    size_t offset  = 0;
    size_t idx     = 0;
    size_t skipMip = 0;

    // リソースデータのメモリを確保.
    resTexture.pResources = new SubResource[ resTexture.MipMapCount * resTexture.SurfaceCount ];

    size_t w = width;
    size_t h = height;
    size_t d = depth;

    for( size_t i=0; i<resTexture.SurfaceCount; ++i )
    {
        w = width;
        h = height;
        d = depth;

        // 各ミップレベルごとに処理.
        for ( size_t j=0; j<resTexture.MipMapCount; ++j )
        {
            size_t rowBytes = 0;
            size_t numRows  = 0;
            size_t numBytes = 0;

            // ブロック圧縮フォーマットの場合.
            if ( ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC1 )
              || ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC2 )
              || ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC3 )
              || ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC4U )
              || ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC4S )
              || ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC5U )
              || ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC5S ) )
            {
                size_t bcPerBlock = 0;
                size_t blockWide  = 0;
                size_t blockHigh  = 0;

                // BC1, BC4の場合.
                if ( ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC1 )
                  || ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC4S )
                  || ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC4U ) )
                { bcPerBlock = 8; }
                // BC2, BC3, BC5, BC6H, BC7の場合.
                else
                { bcPerBlock = 16; }

                // 横幅が1以上の場合.
                if ( w > 0 )
                { blockWide = Max< size_t >( 1, ( w + 3 ) / 4 ); }

                // 縦幅が異常の場合.
                if ( h > 0 )
                { blockHigh = Max< size_t >( 1, ( h + 3 ) / 4 ); }

                // 一行のバイト数.
                rowBytes = blockWide * bcPerBlock;

                // 行数.
                numRows  = blockHigh;
            }
            // ブロック圧縮フォーマット以外の場合.
            else
            {
                // 1ピクセル当たりのビット数.
                size_t bpp = GetBitPerPixel( nativeFormat );

                // 一行のバイト数.
                rowBytes = ( w * bpp + 7 ) / 8;

                // 行数.
                numRows  = h;
            }

            // データ数 = (1行当たりのバイト数) * 行数.
            numBytes = rowBytes * numRows;

            // リソースデータを設定.
            resTexture.pResources[ idx ].Width      = uint32_t( w );
            resTexture.pResources[ idx ].Height     = uint32_t( h );
            resTexture.pResources[ idx ].MipIndex   = uint32_t( j );
            resTexture.pResources[ idx ].Pitch      = uint32_t( rowBytes );
            resTexture.pResources[ idx ].SlicePitch = uint32_t( numBytes );
            resTexture.pResources[ idx ].pPixels    = new uint8_t [ numBytes ];

            // NULLチェック.
            if ( resTexture.pResources[ i ].pPixels == nullptr )
            {
                // エラーログ出力.
                ELOG( "Error : Memory Allocate Failed." );

                // 異常終了.
                return false;
            }

            // ピクセルデータをコピー.
            std::memcpy( resTexture.pResources[ idx ].pPixels, pPixelData + offset, numBytes );

            // オフセットをカウントアップ.
            offset += numBytes;

            // インデックスをカウントアップ.
            idx++;

            // 横幅，縦幅を更新.
            w = w >> 1;
            h = h >> 1;
            d = d >> 1;

            // クランプ処理.
            if ( w == 0 ) { w = 1; }
            if ( h == 0 ) { h = 1; }
            if ( d == 0 ) { d = 1; }
        }
    }

    // 不要になったメモリを解放.
    delete [] pPixelData;
    pPixelData = nullptr;

    // 正常終了.
    return true;
}

//-------------------------------------------------------------------------------------------------
//      DDSファイルからリソーステクスチャを生成します.
//-------------------------------------------------------------------------------------------------
bool CreateResTextureFromDDSFileA(const char* filename, asdx::ResTexture& resTexture)
{
    FILE* pFile = nullptr;
    auto err = fopen_s(&pFile, filename, "rb");
    if (err != 0)
    {
        ELOGA("Error : File Open Failed. path = %s", filename);
        return false;
    }

    return CreateResTextureFromDDSFile(pFile, resTexture);
}

//-------------------------------------------------------------------------------------------------
//      DDSファイルからリソーステクスチャを生成します.
//-------------------------------------------------------------------------------------------------
bool CreateResTextureFromDDSFileW(const wchar_t* filename, asdx::ResTexture& resTexture)
{
    FILE* pFile = nullptr;
    auto err = _wfopen_s(&pFile, filename, L"rb");
    if (err != 0)
    {
        ELOGA("Error : File Open Failed. path = %ls", filename);
        return false;
    }

    return CreateResTextureFromDDSFile(pFile, resTexture);
}

//-------------------------------------------------------------------------------------------------
//      DDSからリソーステクスチャを生成します.
//-------------------------------------------------------------------------------------------------
bool CreateResTextureFromDDSMemory(const uint8_t* pBinary, uint32_t bufferSize, asdx::ResTexture& resTexture)
{
    uint32_t    width  = 0;
    uint32_t    height = 0;
    uint32_t    depth  = 0;
    uint32_t    nativeFormat = 0;

    if ( pBinary == nullptr || bufferSize == 0 )
    {
        ELOG( "Error : Invalid Argument." );
        return false;
    }

    // マジックをチェック.
    if ( ( pBinary[0] != 'D' )
      || ( pBinary[1] != 'D' )
      || ( pBinary[2] != 'S' )
      || ( pBinary[3] != ' ' ) )
    {
        // エラーログ出力.
        ELOG( "Error : Invalid File. " );

        // 異常終了.
        return false;
    }

    uint8_t* pCur = (uint8_t*)pBinary + sizeof(char) * 4;
    uint32_t offset = sizeof(char) * 4;
    if ( offset > bufferSize )
    {
        ELOG( "Error : Out of Range." );
        return false;
    }
    DDSurfaceDesc* ddsd = (DDSurfaceDesc*)pCur;
    offset += sizeof(DDSurfaceDesc);
    if ( offset > bufferSize )
    {
        ELOG( "Error : Out of Range." );
        return false;
    }
    pCur += sizeof(DDSurfaceDesc);

    // 幅有効.
    if ( ddsd->flags & DDSD_WIDTH )
    {
        width = ddsd->width;
        resTexture.Dimension = TEXTURE_DIMENSION_1D;
    }

    // 高さ有効.
    if ( ddsd->flags & DDSD_HEIGHT )
    {
        height = ddsd->height;
        resTexture.Dimension = TEXTURE_DIMENSION_2D;
    }

    // 奥行有効.
    if ( ddsd->flags & DDSD_DEPTH )
    { depth = ddsd->depth; }

    resTexture.Width        = width;
    resTexture.Height       = height;
    resTexture.Depth        = 0;
    resTexture.SurfaceCount = 1;
    resTexture.MipMapCount  = 1;

    // ミップマップ数.有効
    if ( ddsd->flags & DDSD_MIPMAPCOUNT )
    { resTexture.MipMapCount = ddsd->mipMapLevels; }

    // キューブマップとボリュームテクスチャのチェック.
    if ( ddsd->caps & DDSCAPS_COMPLEX )
    {
        // キューブマップの場合.
        if ( ddsd->caps2 & DDSCAPS2_CUBEMAP )
        {
            unsigned int surfaceCount = 0;

            // サーフェイス数をチェック.
            if ( ddsd->caps2 & DDSCAPS2_CUBEMAP_POSITIVE_X ) { surfaceCount++; }
            if ( ddsd->caps2 & DDSCAPS2_CUBEMAP_NEGATIVE_X ) { surfaceCount++; }
            if ( ddsd->caps2 & DDSCAPS2_CUBEMAP_POSITIVE_Y ) { surfaceCount++; }
            if ( ddsd->caps2 & DDSCAPS2_CUBEMAP_NEGATIVE_Y ) { surfaceCount++; }
            if ( ddsd->caps2 & DDSCAPS2_CUBEMAP_POSITIVE_Z ) { surfaceCount++; }
            if ( ddsd->caps2 & DDSCAPS2_CUBEMAP_NEGATIVE_Z ) { surfaceCount++; }

            // 一応チェック.
            assert( surfaceCount == 6 );

            // サーフェイス数を設定.
            resTexture.SurfaceCount = surfaceCount;
            resTexture.Dimension = TEXTURE_DIMENSION_CUBE;
        }
        // ボリュームテクスチャの場合.
        else if ( ddsd->caps2 & DDSCAPS2_VOLUME )
        {
            // 奥行の値を設定.
            resTexture.Depth = depth;
            resTexture.Dimension = TEXTURE_DIMENSION_3D;
        }
    }

    // サポートフォーマットのチェックフラグ.
    bool isSupportFormat = false;

    // ピクセルフォーマット有効.
    if ( ddsd->flags & DDSD_PIXELFORMAT )
    {
        // dwFourCC有効
        if ( ddsd->pixelFormat.flags & DDPF_FOURCC )
        {
            switch( ddsd->pixelFormat.fourCC )
            {
            case FOURCC_DXT1:
                {
                    resTexture.Format = DXGI_FORMAT_BC1_UNORM_SRGB;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC1;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_DXT2:
                {
                    resTexture.Format = DXGI_FORMAT_BC2_UNORM_SRGB;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC2;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_DXT3:
                {
                    resTexture.Format = DXGI_FORMAT_BC2_UNORM_SRGB;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC2;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_DXT4:
                {
                    resTexture.Format = DXGI_FORMAT_BC3_UNORM_SRGB;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC3;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_DXT5:
                {
                    resTexture.Format = DXGI_FORMAT_BC3_UNORM_SRGB;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC3;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_ATI1:
            case FOURCC_BC4U:
                {
                    resTexture.Format = DXGI_FORMAT_BC4_UNORM;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC4U;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_BC4S:
                {
                    resTexture.Format = DXGI_FORMAT_BC5_SNORM;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC4S;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_ATI2:
            case FOURCC_BC5U:
                {
                    resTexture.Format = DXGI_FORMAT_BC5_UNORM;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC5U;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_BC5S:
                {
                    resTexture.Format = DXGI_FORMAT_BC5_SNORM;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_BC5S;
                    isSupportFormat   = true;
                }
                break;

            case FOURCC_DX10:
                {
                }
                break;

            case 36: // D3DFMT_A16B16G16R16
                {
                    // DXGI_FORMAT_R16G61B16A16_UNORM
                }
                break;

            case 110: // D3DFMT_W16W16V16U16
                {
                    // DXGI_FORMAT_R16G16B16A16_SNORM;
                }
                break;

            case 111: // D3DFMT_R16F
                {
                    // DXGI_FORMAT_R16_FLOAT
                    resTexture.Format = DXGI_FORMAT_R16_FLOAT;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_R16_FLOAT;
                    isSupportFormat   = true;
                }
                break;

            case 112: // D3DFMT_G16R16F
                {
                    // DXGI_FORMAT_R16G16_FLOAT
                    resTexture.Format = DXGI_FORMAT_R16G16_FLOAT;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_G16R16_FLOAT;
                    isSupportFormat   = true;
                }
                break;

            case 113: // D3DFMT_A16B16G16R16F
                {
                    // DXGI_FORMAT_R16G16B16A16_FLOAT
                    resTexture.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_A16B16G16R16_FLOAT;
                    isSupportFormat   = true;
                }
                break;

            case 114: // D3DFMT_R32F
                {
                    // DXGI_FORMAT_R32_FLOAT
                    resTexture.Format = DXGI_FORMAT_R32_FLOAT;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_R32_FLOAT;
                    isSupportFormat   = true;
                }
                break;

            case 115: // D3DFMT_G32R32F
                {
                    // DXGI_FORMAT_R32G32_FLOAT
                    resTexture.Format = DXGI_FORMAT_R32G32_FLOAT;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_G32R32_FLOAT;
                    isSupportFormat   = true;
                }
                break;

            case 116: // D3DFMT_A32B32G32R32F
                {
                    // DXGI_FORMAT_R32G32B32A32_FLOAT
                    resTexture.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                    nativeFormat      = NATIVE_TEXTURE_FORMAT_A32B32G32R32_FLOAT;
                    isSupportFormat   = true;
                }
                break;

            default:
                {
                    isSupportFormat = false;
                }
                break;
            }
        }
        else if ( ddsd->pixelFormat.flags & DDPF_RGB )
        {
            switch( ddsd->pixelFormat.bpp )
            {
            case 32:
                {
                    if ( CheckMask( ddsd->pixelFormat, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 ) )
                    {
                        // A8 R8 G8 B8
                        isSupportFormat   = true;
                        resTexture.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                        nativeFormat      = NATIVE_TEXTURE_FORMAT_ARGB_8888;
                    }

                    if ( CheckMask( ddsd->pixelFormat, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 ) )
                    {
                        // A8 B8 G8 R8
                        isSupportFormat   = true;
                        resTexture.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                        nativeFormat      = NATIVE_TEXTURE_FORMAT_ABGR_8888;
                    }

                    if ( CheckMask( ddsd->pixelFormat, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 ) )
                    {
                        // X8 R8 G8 B8
                        isSupportFormat   = true;
                        resTexture.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                        nativeFormat      = NATIVE_TEXTURE_FORMAT_XRGB_8888;
                    }

                    if ( CheckMask( ddsd->pixelFormat, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000 ) )
                    {
                        // X8 B8 G8 R8
                        isSupportFormat = true;
                        resTexture.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                        nativeFormat = NATIVE_TEXTURE_FORMAT_XBGR_8888;
                    }

                #if 0
                    //if ( CheckMask( ddsd->pixelFormat, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000 ) )
                    //{
                    //    // R10 G10 B10 A2
                    //    /* NOT_SUPPORT */
                    //}

                    //if ( CheckMask( ddsd->pixelFormat, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000 ) )
                    //{
                    //    // R16 G16
                    //    /* NOT_SUPPORT */
                    //}
                    //if ( CheckMask( ddsd->pixelFormat, 0xffffffff, 0x00000000,0x00000000,0x00000000 ) )
                    //{
                    //    // R32
                    //    /* NOT_SUPPORT */
                    //}
                #endif
                }
                break;

            case 24:
                {
                #if 0
                    /* NOT_SUPPORT */
                #endif
                }
                break;

            case 16:
                {
                #if 0
                    //if ( CheckMask( ddsd->pixelFormat, 0x7c00, 0x03e0, 0x001f, 0x8000 ) )
                    //{
                    //    // B5 G5 R5 A1
                    //    /* NOT_SUPPORT */
                    //}

                    //if ( CheckMask( ddsd->pixelFormat, 0xf800, 0x07e0, 0x001f, 0x0000 ) )
                    //{
                    //    // B5 G6 R5
                    //    /* NOT_SUPPORT */
                    //}
                #endif
                }
                break;
            }
        }
        else if ( ddsd->pixelFormat.flags & DDPF_LUMINANCE )
        {
            switch( ddsd->pixelFormat.bpp )
            {
            case 8:
                {
                    if ( CheckMask( ddsd->pixelFormat, 0x000000ff, 0x00000000, 0x00000000, 0x00000000 ) )
                    {
                        // R8
                        isSupportFormat   = true;
                        nativeFormat      = NATIVE_TEXTURE_FORMAT_R8;
                        resTexture.Format = DXGI_FORMAT_R8_UNORM;
                    }
                }
                break;

            case 16:
                {
                #if 0
                    //if ( CheckMask( ddsd->pixelFormat, 0x0000ffff, 0x00000000, 0x00000000, 0x00000000 ) )
                    //{
                    //    // R16
                    //    /* NOT_SUPPORT */
                    //}

                    //if ( CheckMask( ddsd->pixelFormat, 0x000000ff, 0x00000000, 0x00000000, 0x0000ff00 ) )
                    //{
                    //    // R8 G8
                    //    /* NOT_SUPPORT */
                    //}
                #endif
                }
                break;
            }
        }
        else if ( ddsd->pixelFormat.flags & DDPF_ALPHA )
        {
            if ( 8 == ddsd->pixelFormat.bpp )
            {
                // A8
                isSupportFormat   = true;
                nativeFormat      = NATIVE_TEXTURE_FORMAT_A8;
                resTexture.Format = DXGI_FORMAT_R8_UNORM;
            }
        }
    }

    // サポートフォーマットがあったかチェック.
    if ( !isSupportFormat )
    {
        // エラーログ出力.
        ELOG( "Error : Unsupported Format." );

        // 異常終了.
        return false;
    }

    // ピクセルデータのサイズを算出.
    size_t pixelSize = bufferSize - offset;

    // ピクセルデータのメモリを確保.
    unsigned char* pPixelData = new unsigned char [pixelSize];

    // NULLチェック.
    if ( pPixelData == nullptr )
    {
        // エラーログ出力.
        ELOG( "Error : Memory Allocate Failed." );

        // 異常終了.
        return false;
    }

    memcpy( pPixelData, pCur, sizeof(unsigned char) * pixelSize );

    // リトルエンディアンなのでピクセルの並びを補正.
    switch( nativeFormat )
    {
        // 一括読み込みでくるっているので修正.
        case NATIVE_TEXTURE_FORMAT_ARGB_8888:
        case NATIVE_TEXTURE_FORMAT_XRGB_8888:
        {
            for( size_t i=0; i<pixelSize; i+=4 )
            {
                // BGRA -> RGBA
                unsigned char R = pPixelData[ i + 0 ];
                unsigned char B = pPixelData[ i + 2 ];
                pPixelData[ i + 0 ] = B;
                pPixelData[ i + 2 ] = R;
            }
        }
        break;
    }

    size_t byteOffset = 0;
    size_t idx    = 0;
    size_t skipMip = 0;

    // リソースデータのメモリを確保.
    resTexture.pResources = new SubResource[ resTexture.MipMapCount * resTexture.SurfaceCount ];

    size_t w = width;
    size_t h = height;
    size_t d = depth;

    for( size_t i=0; i<resTexture.SurfaceCount; ++i )
    {
        w = width;
        h = height;
        d = depth;

        // 各ミップレベルごとに処理.
        for ( size_t j=0; j<resTexture.MipMapCount; ++j )
        {
            size_t rowBytes = 0;
            size_t numRows  = 0;
            size_t numBytes = 0;

            // ブロック圧縮フォーマットの場合.
            if ( ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC1 )
              || ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC2 )
              || ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC3 )
              || ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC4U )
              || ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC4S )
              || ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC5U )
              || ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC5S ) )
            {
                size_t bcPerBlock = 0;
                size_t blockWide  = 0;
                size_t blockHigh  = 0;

                // BC1, BC4の場合.
                if ( ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC1 )
                  || ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC4S )
                  || ( nativeFormat == NATIVE_TEXTURE_FORMAT_BC4U ) )
                { bcPerBlock = 8; }
                // BC2, BC3, BC5, BC6H, BC7の場合.
                else
                { bcPerBlock = 16; }

                blockWide = Max< size_t >( 1, ( w + 3 ) / 4 );
                blockHigh = Max< size_t >( 1, ( h + 3 ) / 4 );

                // 一行のバイト数.
                rowBytes = blockWide * bcPerBlock;

                // 行数.
                numRows  = blockHigh;
            }
            // ブロック圧縮フォーマット以外の場合.
            else
            {
                // 1ピクセル当たりのビット数.
                size_t bpp = GetBitPerPixel( nativeFormat );

                // 一行のバイト数.
                rowBytes = ( w * bpp + 7 ) / 8;

                // 行数.
                numRows  = h;
            }

            // データ数 = (1行当たりのバイト数) * 行数.
            numBytes = rowBytes * numRows;

            // リソースデータを設定.
            resTexture.pResources[ idx ].Width      = uint32_t( w );
            resTexture.pResources[ idx ].Height     = uint32_t( h );
            resTexture.pResources[ idx ].MipIndex   = uint32_t( j );
            resTexture.pResources[ idx ].Pitch      = uint32_t( rowBytes );
            resTexture.pResources[ idx ].SlicePitch = uint32_t( numBytes );
            resTexture.pResources[ idx ].pPixels    = pPixelData + byteOffset;

            // オフセットをカウントアップ.
            byteOffset += numBytes;

            // インデックスをカウントアップ.
            idx++;

            // 横幅，縦幅を更新.
            w = w >> 1;
            h = h >> 1;
            d = d >> 1;

            // クランプ処理.
            if ( w == 0 ) { w = 1; }
            if ( h == 0 ) { h = 1; }
            if ( d == 0 ) { d = 1; }
        }
    }

    // 正常終了.
    return true;
}

//-------------------------------------------------------------------------------------------------
//      Targaファイルからリソーステクスチャを生成します.
//-------------------------------------------------------------------------------------------------
bool CreateResTextureFromTGAFile(FILE* pFile, asdx::ResTexture& resTexture)
{
    // フッターを読み込み.
    TGA_FOOTER footer;
    long offset = sizeof(footer);
    fseek( pFile, -offset, SEEK_END );
    fread( &footer, sizeof(footer), 1, pFile );

    // ファイルマジックをチェック.
    if ( strcmp( footer.Tag, "TRUEVISION-XFILE." ) != 0 )
    {
        ELOG( "Error : Invalid File Format." );
        fclose( pFile );
        return false;
    }

    // 拡張データがある場合は読み込み.
    if ( footer.OffsetExt != 0 )
    {
        TGA_EXTENSION extension;

        fseek( pFile, footer.OffsetExt, SEEK_SET );
        fread( &extension, sizeof(extension), 1, pFile );
    }

    // ディベロッパーエリアがある場合.
    if ( footer.OffsetDev != 0 )
    {
        /* NOT IMPLEMENT */
    }

    // ファイル先頭に戻す.
    fseek( pFile, 0, SEEK_SET );

    // ヘッダデータを読み込む.
    TGA_HEADER header;
    fread( &header, sizeof(header), 1, pFile );

    // フォーマット判定.
    uint32_t bytePerPixel = 0;
    switch( header.Format )
    {
    // 該当なし.
    case TGA_FORMAT_NONE:
        {
            ELOG( "Error : Invalid Format." );
            fclose( pFile );
            return false;
        }
        break;

    // グレースケール
    case TGA_FORMAT_GRAYSCALE:
    case TGA_FORMAT_RLE_GRAYSCALE:
        { 
            if ( header.BitPerPixel == 8 )
            { bytePerPixel = 1; }
            else
            { bytePerPixel = 2; }
        }
        break;

    // カラー.
    case TGA_FORMAT_INDEXCOLOR:
    case TGA_FORMAT_FULLCOLOR:
    case TGA_FORMAT_RLE_INDEXCOLOR:
    case TGA_FORMAT_RLE_FULLCOLOR:
        {
            if ( header.BitPerPixel <= 24 )
            { bytePerPixel = 3; }
            else
            { bytePerPixel = 4; }
        }
        break;

    // 上記以外.
    default:
        {
            ELOG( "Error : Unsupported Format." );
            fclose( pFile );
            return false;
        }
        break;
    }

    // IDフィールドサイズ分だけオフセットを移動させる.
    if (header.IdFieldLength != 0)
    {
        fseek(pFile, header.IdFieldLength, SEEK_CUR);
    }

    // RGBのみはテクスチャがサポートされないので，強制的にRGBAにする.
    auto bpp = (bytePerPixel == 3) ? 4 : bytePerPixel;

    // ピクセルサイズを決定してメモリを確保.
    auto size = header.Width * header.Height * bpp;
    auto pPixels = new (std::nothrow) uint8_t [ size ];
    if ( pPixels == nullptr )
    {
        ELOG( "Error : Out Of Memory." );
        fclose( pFile );
        return false;
    }

    // カラーマップを持つかチェック.
    uint8_t* pColorMap = nullptr;
    if ( header.HasColorMap )
    {
        // カラーマップサイズを算出.
        uint32_t colorMapSize = header.ColorMapEntry * ( header.ColorMapEntrySize >> 3 );

        // メモリを確保.
        pColorMap = new (std::nothrow) uint8_t [ colorMapSize ];
        if ( pColorMap == nullptr )
        {
            ELOG( "Error : Out Of Memory." );
            delete[] pPixels;
            pPixels = nullptr;
            fclose( pFile );
            return false;
        }

        // がばっと読み込む.
        fread( pColorMap, sizeof(uint8_t), colorMapSize, pFile );
    }

    // 幅・高さ・ビットの深さ・ハッシュキーを設定.
    auto width       = header.Width;
    auto height      = header.Height;
    auto format      = static_cast<TGA_FORMAT_TYPE>( header.Format );
   
    // フォーマットに合わせてピクセルデータを解析する.
    switch( header.Format )
    {
    // パレット.
    case TGA_FORMAT_INDEXCOLOR:
        { Parse8Bits( pFile, width * height, pColorMap, pPixels ); }
        break;

    // フルカラー.
    case TGA_FORMAT_FULLCOLOR:
        {
            switch( header.BitPerPixel )
            {
            case 16:
                { Parse16Bits( pFile, width * height, pPixels ); }
                break;

            case 24:
                { Parse24Bits( pFile, width * height, pPixels ); }
                break;

            case 32:
                { Parse32Bits( pFile, width * height, pPixels ); }
                break;
            }
        }
        break;

    // グレースケール.
    case TGA_FORMAT_GRAYSCALE:
        {
            if ( header.BitPerPixel == 8 )
            { Parse8BitsGrayScale( pFile, width * height, pPixels ); }
            else
            { Parse16BitsGrayScale( pFile, width * height, pPixels ); }
        }
        break;

    // パレットRLE圧縮.
    case TGA_FORMAT_RLE_INDEXCOLOR:
        { Parse8BitsRLE( pFile, pColorMap, width * height * 3, pPixels ); }
        break;

    // フルカラーRLE圧縮.
    case TGA_FORMAT_RLE_FULLCOLOR:
        {
            switch( header.BitPerPixel )
            {
            case 16:
                { Parse16BitsRLE( pFile, width * height * 4, pPixels ); }
                break;

            case 24:
                { Parse24BitsRLE( pFile, width * height * 4, pPixels ); }
                break;

            case 32:
                { Parse32BitsRLE( pFile, width * height * 4, pPixels ); }
                break;
            }
        }
        break;

    // グレースケールRLE圧縮.
    case TGA_FORMAT_RLE_GRAYSCALE:
        {
            if ( header.BitPerPixel == 8 )
            { Parse8BitsGrayScaleRLE( pFile, width * height, pPixels ); }
            else
            { Parse16BitsGrayScaleRLE( pFile, width * height * 2, pPixels ); }
        }
        break;
    }

    // 不要なメモリを解放.
    if (pColorMap != nullptr)
    {
        delete[] pColorMap;
        pColorMap = nullptr;
    }

    // ファイルを閉じる.
    fclose( pFile );

    auto surface = new SubResource();
    if (surface == nullptr)
    {
        ELOG("Error : Out of Memory.");
        return false;
    }

    surface->Width      = width;
    surface->Height     = height;
    surface->MipIndex   = 0;
    surface->Pitch      = width * bpp;
    surface->SlicePitch = width * height * bpp;
    surface->pPixels    = pPixels;

    resTexture.Dimension    = TEXTURE_DIMENSION_2D;
    resTexture.Width        = width;
    resTexture.Height       = height;
    resTexture.Depth        = 1;
    resTexture.SurfaceCount = 1;
    resTexture.MipMapCount  = 1;
    resTexture.pResources   = surface;

    switch(format)
    {
    case TGA_FORMAT_NONE:
        resTexture.Format = DXGI_FORMAT_UNKNOWN;
        break;

    case TGA_FORMAT_FULLCOLOR:
        resTexture.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;

    case TGA_FORMAT_GRAYSCALE:
        resTexture.Format = DXGI_FORMAT_R8_UNORM;
        break;

    case TGA_FORMAT_RLE_FULLCOLOR:
        resTexture.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;

    case TGA_FORMAT_RLE_GRAYSCALE:
        resTexture.Format = DXGI_FORMAT_R8_UNORM;
        break;

    case TGA_FORMAT_INDEXCOLOR:
    case TGA_FORMAT_RLE_INDEXCOLOR:
        {
            switch (bytePerPixel)
            {
            case 1:
                { resTexture.Format = DXGI_FORMAT_R8_UNORM; }
                break;

            case 2:
                { resTexture.Format = DXGI_FORMAT_B5G5R5A1_UNORM; }
                break;

            default:
                {
                    ELOG("Error : Unknown Format. bytePerPixel = %u", bytePerPixel );
                    assert(false);
                    return false;
                }
            }
        }
        break;
    }

    // 正常終了.
    return true;
}

//-------------------------------------------------------------------------------------------------
//      Targaファイルからリソーステクスチャを生成します.
//-------------------------------------------------------------------------------------------------
bool CreateResTextureFromTGAFileA(const char* filename, asdx::ResTexture& resTexture)
{
    FILE* pFile = nullptr;
    auto err = fopen_s(&pFile, filename, "rb");
    if (err != 0)
    {
        ELOGA("Error : File Open Failed. path = %s", filename);
        return false;
    }

    return CreateResTextureFromTGAFile(pFile, resTexture);
}

//-------------------------------------------------------------------------------------------------
//      Targaファイルからリソーステクスチャを生成します.
//-------------------------------------------------------------------------------------------------
bool CreateResTextureFromTGAFileW(const wchar_t* filename, asdx::ResTexture& resTexture)
{
    FILE* pFile = nullptr;
    auto err = _wfopen_s(&pFile, filename, L"rb");
    if (err != 0)
    {
        ELOGW("Error : File Open Failed. path = %ls", filename);
        return false;
    }

    return CreateResTextureFromTGAFile(pFile, resTexture);
}

//------------------------------------------------------------------------------------------
//      HDRファイルのヘッダを読み込みします.
//------------------------------------------------------------------------------------------
bool ReadHdrHeader( FILE* pFile, int32_t& width, int32_t& height, float& gamma, float& exposure )
{
    char buf[ 256 ];
    fread( buf, sizeof(char), 2, pFile );

    if ( buf[0] != '#' || buf[1] != '?' )
    { return false; }

    auto valid = false;
    for( ;; )
    {
        if ( fgets( buf, 256, pFile ) == nullptr )
        { break; }

        if ( buf[0] == '\n' )
        { break; }
        else if ( buf[0] == '#' )
        { continue; }
        else
        {
            auto g = 1.0f;
            auto e = 1.0f;
            if ( sscanf_s( buf, "GAMMA=%f\n", &g ) != 0 ) 
            { gamma = g; }
            else if ( sscanf_s( buf, "EXPOSURE=%f\n", &e ) != 0 )
            { exposure = e; }
            else if ( strcmp( buf, "FORMAT=32-bit_rle_rgbe\n" ) == 0 )
            { valid = true; }
        }
    }

    if ( !valid )
    { return false; }

    if ( fgets( buf, 256, pFile ) != nullptr )
    {
        auto w = 0;
        auto h = 0;
        if ( sscanf_s( buf, "-Y %d +X %d\n", &h, &w ) != 0 )
        {
            width = w;
            height = h;
        }
        else if ( sscanf_s( buf, "+X %d -Y %d\n", &w, &h ) != 0 )
        {
            width = w;
            height = h;
        }
        else
        { return false; }
    }

    return true;
}

//------------------------------------------------------------------------------------------
//      旧形式のカラーを読み取ります.
//------------------------------------------------------------------------------------------
bool ReadOldColors( FILE* pFile, RGBE* pLine, int32_t count )
{
    auto shift = 0;
    while( 0 < count )
    {
        pLine[0].r = getc( pFile );
        pLine[0].g = getc( pFile );
        pLine[0].b = getc( pFile );
        pLine[0].e = getc( pFile );

        if ( feof( pFile ) || ferror( pFile ) )
            return false;

        if ( pLine[0].r == 1
          && pLine[0].g == 1
          && pLine[0].b == 1 )
        {
            for( auto i=pLine[0].e << shift; i > 0; i-- )
            {
                pLine[0].r = pLine[-1].r;
                pLine[0].g = pLine[-1].g;
                pLine[0].b = pLine[-1].b;
                pLine[0].e = pLine[-1].e;
                pLine++;
                count--;
            }
            shift += 8;
        }
        else
        {
            pLine++;
            count--;
            shift = 0;
        }
    }

    return true;
}

//------------------------------------------------------------------------------------------
//      カラーを読み取ります.
//------------------------------------------------------------------------------------------
bool ReadColor( FILE* pFile, RGBE* pLine, int32_t count )
{
    if ( count < 8 || 0x7fff < count )
    { return ReadOldColors( pFile, pLine, count ); }

    auto i = getc( pFile );
    if ( i == EOF )
        return false;

    if ( i != 2 )
    {
        ungetc( i, pFile );
        return ReadOldColors( pFile, pLine, count );
    }

    pLine[0].g = getc( pFile );
    pLine[0].b = getc( pFile );

    if ( ( i = getc( pFile ) ) == EOF )
        return false;

    if ( pLine[0].g != 2 || pLine[0].b & 128 )
    {
        pLine[0].r = 2;
        pLine[0].e = i;
        return ReadOldColors( pFile, pLine + 1, count -1 );
    }

    if ( ( pLine[0].b << 8 | i ) != count )
        return false;

    for( i=0; i<4; ++i )
    {
        for( auto j=0; j<count; )
        {
            auto code = getc( pFile );
            if ( code == EOF )
                return false;

            if ( 128 < code )
            {
                code &= 127;
                auto val = getc( pFile );
                while( code-- )
                { pLine[j++].v[i] = val; }
            }
            else
            {
                while( code-- )
                { pLine[j++].v[i] = getc( pFile ); }
            }
        }
    }

    return ( feof( pFile ) ? false : true );
}

//------------------------------------------------------------------------------------------
//      HDRデータを読み取ります.
//------------------------------------------------------------------------------------------
bool ReadHdrData( FILE* pFile, const int32_t width, const int32_t height, float** ppPixels )
{
    auto pLines = new(std::nothrow) RGBE [ width * height ];
    if ( pLines == nullptr )
    { return false; }

    auto pixels = new (std::nothrow) float [ width * height * 4 ];
    if ( pixels == nullptr )
    { return false; }

    for( auto y=0; y<height; ++y )
    {
        if ( !ReadColor( pFile, pLines, width ) )
        {
            SafeDeleteArray( pLines );
            SafeDeleteArray( pixels );
            return false;
        }

        for( auto x =0; x < width; x++ )
        {
            auto pix = RGBEToVec3( pLines[x] );
            auto idx = ( x * 4 ) + ( y * width *  4 );
            pixels[idx + 0] = pix.x;
            pixels[idx + 1] = pix.y;
            pixels[idx + 2] = pix.z;
            pixels[idx + 3] = 1.0f;
        }
    }

    SafeDeleteArray( pLines );
    (*ppPixels) = pixels;

    return true;
}

//------------------------------------------------------------------------------------------
//      HDRファイルからデータをロードします.
//------------------------------------------------------------------------------------------
bool CreateResTextureFromHDRFileA( const char* filename, asdx::ResTexture& resTexture)
{
    FILE* pFile = nullptr;

    auto err = fopen_s( &pFile, filename, "rb" );
    if ( err != 0 )
    {
        ELOGA( "Error : LoadFromHDR() Failed. File Open Failed. filename = %s", filename );
        return false;
    }

    int32_t width    = 0;
    int32_t height   = 0;
    float   gamma    = 1.0f;
    float   exposure = 1.0f;
    if ( !ReadHdrHeader(pFile, width, height, gamma, exposure) )
    {
        ELOGA( "Error : LoadFromHDR() Failed. Header Read Failed. filename = %s", filename );
        fclose(pFile);
        return false;
    }

    resTexture.Dimension    = TEXTURE_DIMENSION_2D;
    resTexture.Width        = uint32_t(width);
    resTexture.Height       = uint32_t(height);
    resTexture.Depth        = 0;
    resTexture.Format       = DXGI_FORMAT_R32G32B32A32_FLOAT;
    resTexture.MipMapCount  = 1;
    resTexture.SurfaceCount = 1;
    resTexture.pResources   = new SubResource[1];

    resTexture.pResources[0].Width      = uint32_t(width);
    resTexture.pResources[0].Height     = uint32_t(height);
    resTexture.pResources[0].Pitch      = width * sizeof(float) * 4;
    resTexture.pResources[0].SlicePitch = resTexture.pResources[0].Pitch * height;

    if ( !ReadHdrData(pFile, width, height, reinterpret_cast<float**>(&resTexture.pResources[0].pPixels)) )
    {
        ELOGA( "Error : LoadFromHDR() Failed. Data Read Failed. filename = %s", filename );
        fclose(pFile);
        return false;
    }

    fclose(pFile);
    return true;
}

//------------------------------------------------------------------------------------------
//      HDRファイルからデータをロードします.
//------------------------------------------------------------------------------------------
bool CreateResTextureFromHDRFileW(const wchar_t* filename, asdx::ResTexture& resTexture)
{
    FILE* pFile = nullptr;

    auto err = _wfopen_s(&pFile, filename, L"rb" );
    if ( err != 0 )
    {
        ELOGW( "Error : LoadFromHDR() Failed. File Open Failed. filename = %s", filename );
        return false;
    }

    int32_t width    = 0;
    int32_t height   = 0;
    float   gamma    = 1.0f;
    float   exposure = 1.0f;
    if ( !ReadHdrHeader(pFile, width, height, gamma, exposure) )
    {
        ELOGW( "Error : LoadFromHDR() Failed. Header Read Failed. filename = %s", filename );
        fclose(pFile);
        return false;
    }

    resTexture.Dimension    = TEXTURE_DIMENSION_2D;
    resTexture.Width        = uint32_t(width);
    resTexture.Height       = uint32_t(height);
    resTexture.Depth        = 0;
    resTexture.Format       = DXGI_FORMAT_R32G32B32A32_FLOAT;
    resTexture.MipMapCount  = 1;
    resTexture.SurfaceCount = 1;
    resTexture.pResources   = new SubResource[1];

    resTexture.pResources[0].Width      = uint32_t(width);
    resTexture.pResources[0].Height     = uint32_t(height);
    resTexture.pResources[0].Pitch      = width * sizeof(float) * 4;
    resTexture.pResources[0].SlicePitch = resTexture.pResources[0].Pitch * height;

    if ( !ReadHdrData(pFile, width, height, reinterpret_cast<float**>(&resTexture.pResources[0].pPixels)) )
    {
        ELOGW( "Error : LoadFromHDR() Failed. Data Read Failed. filename = %s", filename );
        fclose(pFile);
        return false;
    }

    fclose(pFile);
    return true;
}


//-------------------------------------------------------------------------------------------------
//      ファイルからテクスチャを生成します.
//-------------------------------------------------------------------------------------------------
bool CreateResTextureFromFileW(const wchar_t* filename, asdx::ResTexture& resTexture)
{
    if ( filename == nullptr )
    {
        ELOGW( "Error : Invalid Argument." );
        return false;
    }

    auto ext = GetExtW(filename);

    if (ext == L"dds")
    { return CreateResTextureFromDDSFileW( filename, resTexture ); }
    else if (ext == L"tga")
    { return CreateResTextureFromTGAFileW( filename, resTexture ); }
    else if (ext == L"hdr")
    { return CreateResTextureFromHDRFileW( filename, resTexture ); }

    return CreateResTextureFromWICFileW( filename, resTexture );
}


//-------------------------------------------------------------------------------------------------
//      ファイルからテクスチャを生成します.
//-------------------------------------------------------------------------------------------------
bool CreateResTextureFromFileA(const char* filename, asdx::ResTexture& resTexture)
{
    if ( filename == nullptr )
    {
        ELOGA( "Error : Invalid Argument." );
        return false;
    }

    auto ext = GetExtA(filename);

    if (ext == "dds")
    { return CreateResTextureFromDDSFileA( filename, resTexture ); }
    else if (ext == "tga")
    { return CreateResTextureFromTGAFileA( filename, resTexture ); }
    else if (ext == "hdr")
    { return CreateResTextureFromHDRFileA( filename, resTexture ); }

    return CreateResTextureFromWICFileA( filename, resTexture );
}


//-------------------------------------------------------------------------------------------------
//      メモリストリームからテクスチャを生成します.
//-------------------------------------------------------------------------------------------------
bool CreateResTextureFromMemory(const uint8_t* pBinary, uint32_t bufferSize, asdx::ResTexture& resTexture)
{
    if ( pBinary == nullptr || bufferSize < 4 )
    {
        ELOG( "Error : Invalid Argument." );
        return false;
    }

    bool isDDS = true;
    if ( ( pBinary[0] != 'D' )
      || ( pBinary[1] != 'D' )
      || ( pBinary[2] != 'S' )
      || ( pBinary[3] != ' ' ) )
    {
        isDDS = false;
    }

    if ( isDDS )
    { return CreateResTextureFromDDSMemory( pBinary, bufferSize, resTexture ); }

    return CreateResTextureFromWICMemory( pBinary, bufferSize, resTexture );
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// ResTexture class
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//      ファイルからテクスチャリソースを生成します.
//-------------------------------------------------------------------------------------------------
bool ResTexture::LoadFromFileA(const char* filename)
{ return CreateResTextureFromFileA( filename, (*this) ); }

//-------------------------------------------------------------------------------------------------
//      ファイルからテクスチャリソースを生成します.
//-------------------------------------------------------------------------------------------------
bool ResTexture::LoadFromFileW(const wchar_t* filename)
{ return CreateResTextureFromFileW( filename, (*this) ); }

//-------------------------------------------------------------------------------------------------
//      メモリストリームからテクスチャリソースを生成します.
//-------------------------------------------------------------------------------------------------
bool ResTexture::LoadFromMemory(const uint8_t* pBuffer, uint32_t bufferSize)
{ return CreateResTextureFromMemory( pBuffer, bufferSize, (*this) ); }


} // namespace asdx
