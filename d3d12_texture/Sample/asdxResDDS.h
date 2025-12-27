//-----------------------------------------------------------------------------
// File : asdxResDDS.h
// Desc : Direct Draw Surface Module.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cstdint>


namespace asdx {

//-----------------------------------------------------------------------------
// Constant Values
//-----------------------------------------------------------------------------

// dwFlags Value
static const uint32_t DDSD_CAPS         = 0x00000001;   // dwCaps/dwCaps2が有効.
static const uint32_t DDSD_HEIGHT       = 0x00000002;   // dwHeightが有効.
static const uint32_t DDSD_WIDTH        = 0x00000004;   // dwWidthが有効.
static const uint32_t DDSD_PITCH        = 0x00000008;   // dwPitchOrLinearSizeがPitchを表す.
static const uint32_t DDSD_PIXELFORMAT  = 0x00001000;   // dwPfSize/dwPfFlags/dwRGB～等の直接定義が有効.
static const uint32_t DDSD_MIPMAPCOUNT  = 0x00020000;   // dwMipMapCountが有効.
static const uint32_t DDSD_LINEARSIZE   = 0x00080000;   // dwPitchOrLinearSizeがLinerSizeを表す.
static const uint32_t DDSD_DEPTH        = 0x00800000;   // dwDepthが有効.

// dwPfFlags Value
static const uint32_t DDPF_ALPHAPIXELS      = 0x00000001;   // RGB以外にalphaが含まれている.
static const uint32_t DDPF_ALPHA            = 0x00000002;   // pixelはAlpha成分のみ.
static const uint32_t DDPF_FOURCC           = 0x00000004;   // dwFourCCが有効.
static const uint32_t DDPF_PALETTE_INDEXED4 = 0x00000008;   // Palette 16 colors.
static const uint32_t DDPF_PALETTE_INDEXED8 = 0x00000020;   // Palette 256 colors.
static const uint32_t DDPF_RGB              = 0x00000040;   // dwRGBBitCount/dwRBitMask/dwGBitMask/dwBBitMask/dwRGBAlphaBitMaskによってフォーマットが定義されていることを示す.
static const uint32_t DDPF_LUMINANCE        = 0x00020000;   // 1chのデータがRGB全てに展開される.
static const uint32_t DDPF_BUMPDUDV         = 0x00080000;   // pixelが符号付きであることを示す.

// dwCaps Value
static const uint32_t DDSCAPS_ALPHA     = 0x00000002;       // Alphaが含まれている場合.
static const uint32_t DDSCAPS_COMPLEX   = 0x00000008;       // 複数のデータが含まれている場合Palette/Mipmap/Cube/Volume等.
static const uint32_t DDSCAPS_TEXTURE   = 0x00001000;       // 常に1.
static const uint32_t DDSCAPS_MIPMAP    = 0x00400000;       // MipMapが存在する場合.

// dwCaps2 Value
static const uint32_t DDSCAPS2_CUBEMAP              = 0x00000200;   // CubeMapが存在する場合.
static const uint32_t DDSCAPS2_CUBEMAP_POSITIVE_X   = 0x00000400;   // CubeMap X+
static const uint32_t DDSCAPS2_CUBEMAP_NEGATIVE_X   = 0x00000800;   // CubeMap X-
static const uint32_t DDSCAPS2_CUBEMAP_POSITIVE_Y   = 0x00001000;   // CubeMap Y+
static const uint32_t DDSCAPS2_CUBEMAP_NEGATIVE_Y   = 0x00002000;   // CubeMap Y-
static const uint32_t DDSCAPS2_CUBEMAP_POSITIVE_Z   = 0x00004000;   // CubeMap Z+
static const uint32_t DDSCAPS2_CUBEMAP_NEGATIVE_Z   = 0x00008000;   // CubeMap Z-
static const uint32_t DDSCAPS2_VOLUME               = 0x00400000;   // VolumeTextureの場合.

// dwFourCC Value
static const uint32_t FOURCC_DXT1           = '1TXD';           // DXT1
static const uint32_t FOURCC_DXT2           = '2TXD';           // DXT2
static const uint32_t FOURCC_DXT3           = '3TXD';           // DXT3
static const uint32_t FOURCC_DXT4           = '4TXD';           // DXT4
static const uint32_t FOURCC_DXT5           = '5TXD';           // DXT5
static const uint32_t FOURCC_ATI1           = '1ITA';           // 3Dc ATI2
static const uint32_t FOURCC_ATI2           = '2ITA';           // 3Dc ATI2
static const uint32_t FOURCC_DX10           = '01XD';           // DX10
static const uint32_t FOURCC_BC4U           = 'U4CB';           // BC4U
static const uint32_t FOURCC_BC4S           = 'S4CB';           // BC4S
static const uint32_t FOURCC_BC5U           = 'U5CB';           // BC5U
static const uint32_t FOURCC_BC5S           = 'S5CB';           // BC5S
static const uint32_t FOURCC_RGBG           = 'GBGR';           // RGBG
static const uint32_t FOURCC_GRGB           = 'BGRG';           // GRGB
static const uint32_t FOURCC_YUY2           = '2YUY';           // YUY2
static const uint32_t FOURCC_A16B16G16R16   = 0x00000024;
static const uint32_t FOURCC_Q16W16V16U16   = 0x0000006e;
static const uint32_t FOURCC_R16F           = 0x0000006f;
static const uint32_t FOURCC_G16R16F        = 0x00000070;
static const uint32_t FOURCC_A16B16G16R16F  = 0x00000071;
static const uint32_t FOURCC_R32F           = 0x00000072;
static const uint32_t FOURCC_G32R32F        = 0x00000073;
static const uint32_t FOURCC_A32B32G32R32F  = 0x00000074;
static const uint32_t FOURCC_CxV8U8         = 0x00000075;
static const uint32_t FOURCC_Q8W8V8U8       = 0x0000003f;

static const uint32_t DDS_RESOURCE_MISC_TEXTRECUBE = 0x4L;


///////////////////////////////////////////////////////////////////////////////
// DDS_RESOURCE_DIMENSION
///////////////////////////////////////////////////////////////////////////////
enum DDS_RESOURCE_DIMENSION
{
    DDS_RESOURCE_DIMENSION_TEXTURE1D = 2,
    DDS_RESOURCE_DIMENSION_TEXTURE2D,
    DDS_RESOURCE_DIMENSION_TEXTURE3D,
};


///////////////////////////////////////////////////////////////////////////////
// DDS_FORMAT_TYPE
///////////////////////////////////////////////////////////////////////////////
enum DDS_FORMAT_TYPE
{
    DDS_FORMAT_UNKNOWN = 0,

    DDS_FORMAT_R32G32B32A32_FLOAT = 2,

    DDS_FORMAT_R16G16B16A16_FLOAT = 10,
    DDS_FORMAT_R16G16B16A16_UNORM = 11,

    DDS_FORMAT_R32G32_FLOAT = 16,

    DDS_FORMAT_R10G10B10A2_UNORM = 24,

    DDS_FORMAT_R8G8B8A8_UNORM = 28,

    DDS_FORMAT_R16G16_FLOAT = 34,
    DDS_FORMAT_R16G16_UNORM = 35,

    DDS_FORMAT_R32_FLOAT = 41,

    DDS_FORMAT_R8G8_UNORM = 49,

    DDS_FORMAT_R16_FLOAT = 54,
    DDS_FORMAT_R16_UNORM = 56,

    DDS_FORMAT_R8_UNORM = 63,
    DDS_FORMAT_A8_UNORM = 65,

    DDS_FORMAT_BC1_UNORM = 71,
    DDS_FORMAT_BC2_UNORM = 74,
    DDS_FORMAT_BC3_UNORM = 77,
    DDS_FORMAT_BC4_UNORM = 80,
    DDS_FORMAT_BC4_SNORM = 81,
    DDS_FORMAT_BC5_UNORM = 83,
    DDS_FORMAT_BC5_SNORM = 84,
    DDS_FORMAT_BC6H_UF16 = 95,
    DDS_FORMAT_BC6H_SF16 = 96,
    DDS_FORMAT_BC7_UNORM = 98,

    DDS_FORMAT_B5G6R5_UNORM = 85,
    DDS_FORMAT_B5G5R5A1_UNORM = 86,
    DDS_FORMAT_B8G8R8A8_UNORM = 87,
    DDS_FORMAT_B8G8R8X8_UNORM = 88,
    DDS_FORMAT_B4G4R4A4_UNORM = 115,

    // 以下, 対応するDXGIは無いのでフォーマット変換が必要.
    DDS_FORMAT_B8G8R8_UNORM      = 300,
    DDS_FORMAT_B5G5R5X1_UNORM    = 301,
    DDS_FORMAT_B4G4R4X4_UNORM    = 302,
    DDS_FORMAT_R8G8B8X8_UNORM    = 304,
};


///////////////////////////////////////////////////////////////////////////////
// DDS_PIXEL_FORMAT structure
///////////////////////////////////////////////////////////////////////////////
struct DDS_PIXEL_FORMAT
{
    uint32_t     Size;
    uint32_t     Flags;
    uint32_t     FourCC;
    uint32_t     Bpp;
    uint32_t     MaskR;
    uint32_t     MaskG;
    uint32_t     MaskB;
    uint32_t     MaskA;
};

///////////////////////////////////////////////////////////////////////////////
// DDS_COLOR_KEY structure
///////////////////////////////////////////////////////////////////////////////
struct DDS_COLOR_KEY
{
    uint32_t     Low;
    uint32_t     Hight;
};

///////////////////////////////////////////////////////////////////////////////
// DDS_SURFACE_DESC structure
///////////////////////////////////////////////////////////////////////////////
struct DDS_SURFACE_DESC
{
    uint32_t            Size;
    uint32_t            Flags;
    uint32_t            Height;
    uint32_t            Width;
    uint32_t            Pitch;
    uint32_t            Depth;
    uint32_t            MipMapLevels;
    uint32_t            AlphaBitDepth;
    uint32_t            Reserved;
    uint32_t            Surface;

    DDS_COLOR_KEY       DstOverlay;
    DDS_COLOR_KEY       DstBit;
    DDS_COLOR_KEY       SrcOverlay;
    DDS_COLOR_KEY       SrcBit;

    DDS_PIXEL_FORMAT    PixelFormat;
    uint32_t            Caps;
    uint32_t            Caps2;
    uint32_t            ReservedCaps[ 2 ];

    uint32_t            TextureStage;
};

///////////////////////////////////////////////////////////////////////////////
// DDS_DXT10_HEADER structure
///////////////////////////////////////////////////////////////////////////////
struct DDS_DXT10_HEADER
{
    uint32_t     DXGIFormat;
    uint32_t     ResourceDimension;
    uint32_t     MiscFlag;
    uint32_t     ArraySize;
    uint32_t     MiscFlags2;
};

///////////////////////////////////////////////////////////////////////////////
// Surface structure
///////////////////////////////////////////////////////////////////////////////
struct Surface
{
    uint32_t     Width;          //!< 横幅です.
    uint32_t     Height;         //!< 高さです.
    uint32_t     Pitch;          //!< ピッチです.
    uint32_t     SlicePitch;     //!< スライスピッチです.
    uint8_t*     pPixels;        //!< ピクセルデータです.

    void Release();
    Surface& operator = (const Surface& value);
};

///////////////////////////////////////////////////////////////////////////////
// ResDDS class
///////////////////////////////////////////////////////////////////////////////
class ResDDS
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
    // public methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //-------------------------------------------------------------------------
    ResDDS();

    //-------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //-------------------------------------------------------------------------
    ~ResDDS();

    //-------------------------------------------------------------------------
    //! @brief      ファイルから読み込みを行います.
    //!
    //! @param[in]      filename        ファイル名です.
    //! @retval true    読み込みに成功.
    //! @retval false   読み込みに失敗.
    //-------------------------------------------------------------------------
    bool Load(const wchar_t* filename);

    //-------------------------------------------------------------------------
    //! @brief      メモリを解放します.
    //-------------------------------------------------------------------------
    void Release();

    //-------------------------------------------------------------------------
    //! @brief      画像の横幅を取得します.
    //!
    //! @return     画像の横幅を返却します.
    //-------------------------------------------------------------------------
    const uint32_t GetWidth() const;

    //-------------------------------------------------------------------------
    //! @brief      画像の縦幅を取得します.
    //!
    //! @return     画像の縦幅を返却します.
    //-------------------------------------------------------------------------
    const uint32_t GetHeight() const;

    //-------------------------------------------------------------------------
    //! @brief      画像の奥行きを取得します.
    //!
    //! @return     画像の奥行きを返却します.
    //-------------------------------------------------------------------------
    const uint32_t GetDepth() const;

    //-------------------------------------------------------------------------
    //! @brief      サーフェイス数を取得します.
    //!
    //! @return     サーフェイス数を返却します.
    //-------------------------------------------------------------------------
    const uint32_t GetSurfaceCount() const;

    //-------------------------------------------------------------------------
    //! @brief      ミップマップ数を取得します.
    //!
    //! @return     ミップマップ数を返却します.
    //-------------------------------------------------------------------------
    const uint32_t GetMipMapCount() const;

    //-------------------------------------------------------------------------
    //! @brief      フォーマットを取得します.
    //!
    //! @return     フォーマットを返却します.
    //-------------------------------------------------------------------------
    const uint32_t GetFormat() const;

    //-------------------------------------------------------------------------
    //! @brief      次元数を取得します.
    //!
    //! @return     次元数を返却します.
    //-------------------------------------------------------------------------
    const DDS_RESOURCE_DIMENSION GetDimension() const;

    //-------------------------------------------------------------------------
    //! @brief      キューブマップかどうかチェックします.
    //!
    //! @return     キューブマップであればtrueを返却します.
    //-------------------------------------------------------------------------
    const bool IsCubeMap() const;

    //-------------------------------------------------------------------------
    //! @brief      サーフェイスを取得します.
    //!
    //! @return     サーフェイスを返却します.
    //-------------------------------------------------------------------------
    const Surface* GetSurfaces() const;

private:
    //=============================================================================================
    // private variables.
    //=============================================================================================
    uint32_t                m_Width;                //!< 画像の横幅です.
    uint32_t                m_Height;               //!< 画像の縦幅です.
    uint32_t                m_Depth;                //!< 画像の奥行きです.
    uint32_t                m_SurfaceCount;         //!< サーフェイス数です.
    uint32_t                m_MipMapCount;          //!< ミップマップ数です.
    uint32_t                m_Format;               //!< フォーマットです.
    DDS_RESOURCE_DIMENSION  m_Dimension;            //!< 次元数です.
    bool                    m_IsCubeMap;            //!< キューブマップかどうか?
    Surface*                m_pSurfaces;            //!< サーフェイスです.

    //=============================================================================================
    // private methods.
    //=============================================================================================
    /* NOTHING */
};

} // namespace asdx
