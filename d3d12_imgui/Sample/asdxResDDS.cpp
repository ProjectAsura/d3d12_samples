//-------------------------------------------------------------------------------------------------
// File : asdxResDDS.cpp
// Desc : Direct Draw Surface Module.
// Copyright(c) Project Asura. All right reserved.
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------
#include "asdxResDDS.h"
#include "asdxMath.h"
#include <cstdio>
#include <cassert>
#include <new>


#ifndef ELOG
#define ELOG( x, ... )  fprintf_s( stderr, "[File: %s, Line:%d] " x, __FILE__, __LINE__, ##__VA_ARGS__ )
#endif//ELOG


namespace /* anonymous */ {

//-------------------------------------------------------------------------------------------------
//      安全に配列を削除します.
//-------------------------------------------------------------------------------------------------
template<typename T>
void SafeDeleteArray( T*& ptr )
{
    if ( ptr != nullptr )
    {
        delete[] ptr;
        ptr = nullptr;
    }
}

//-------------------------------------------------------------------------------------------------
//      マスクをチェックします.
//-------------------------------------------------------------------------------------------------
bool CheckMask
(
    const asdx::DDS_PIXEL_FORMAT& pixelFormat,
    uint32_t maskR,
    uint32_t maskG,
    uint32_t maskB,
    uint32_t maskA
)
{
    if ( ( pixelFormat.MaskR == maskR )
      && ( pixelFormat.MaskG == maskG )
      && ( pixelFormat.MaskB == maskB )
      && ( pixelFormat.MaskA == maskA ) )
    { return true; }

    return false;
}

//-------------------------------------------------------------------------------------------------
//      1ピクセル当たりのビット数を取得します.
//-------------------------------------------------------------------------------------------------
uint32_t GetBitPerPixel( const uint32_t format )
{
    uint32_t result = 0;
    switch( format )
    {
    case asdx::DDS_FORMAT_R32G32B32A32_FLOAT:
        { result = 128; }
        break;

    case asdx::DDS_FORMAT_R16G16B16A16_UNORM:
    case asdx::DDS_FORMAT_R16G16B16A16_FLOAT:
    case asdx::DDS_FORMAT_R32G32_FLOAT:
        { result = 64; }
        break;

    case asdx::DDS_FORMAT_R10G10B10A2_UNORM:
    //case asdx::DDS_FORMAT_B10G10R10A2_UNORM:
    case asdx::DDS_FORMAT_R8G8B8A8_UNORM:
    case asdx::DDS_FORMAT_R8G8B8X8_UNORM:
    case asdx::DDS_FORMAT_R16G16_FLOAT:
    case asdx::DDS_FORMAT_R16G16_UNORM:
    case asdx::DDS_FORMAT_R32_FLOAT:
    case asdx::DDS_FORMAT_B8G8R8A8_UNORM:
    case asdx::DDS_FORMAT_B8G8R8X8_UNORM:
#if 0
    //case asdx::DDS_FORMAT_R8G8_B8G8_UNORM:
    //case asdx::DDS_FORMAT_G8R8_G8B8_UNORM:
    //case asdx::DDS_FORMAT_YUY2:
#endif
        { result = 32; }
        break;

    case asdx::DDS_FORMAT_R8G8_UNORM:
    case asdx::DDS_FORMAT_R16_FLOAT:
    case asdx::DDS_FORMAT_R16_UNORM:
    case asdx::DDS_FORMAT_B5G6R5_UNORM:
    case asdx::DDS_FORMAT_B5G5R5A1_UNORM:
    case asdx::DDS_FORMAT_B5G5R5X1_UNORM:
    case asdx::DDS_FORMAT_B4G4R4A4_UNORM:
    case asdx::DDS_FORMAT_B4G4R4X4_UNORM:
#if 0
    //case asdx::DDS_FORMAT_A8P8:
#endif
        { result = 16; }
        break;

    case asdx::DDS_FORMAT_R8_UNORM:
    case asdx::DDS_FORMAT_A8_UNORM:
#if 0
    //case asdx::DDS_FORMAT_P8:
#endif
        { result = 8; }
        break;

    case asdx::DDS_FORMAT_BC1_UNORM:
    case asdx::DDS_FORMAT_BC4_SNORM:
    case asdx::DDS_FORMAT_BC4_UNORM:
        { result = 4; }
        break;

    case asdx::DDS_FORMAT_BC2_UNORM:
    case asdx::DDS_FORMAT_BC3_UNORM:
    case asdx::DDS_FORMAT_BC5_SNORM:
    case asdx::DDS_FORMAT_BC5_UNORM:
    case asdx::DDS_FORMAT_BC6H_SF16:
    case asdx::DDS_FORMAT_BC6H_UF16:
    case asdx::DDS_FORMAT_BC7_UNORM:
        { result = 8; }
        break;

    case asdx::DDS_FORMAT_B8G8R8_UNORM:
        { result = 24; }
        break;

    default:
        { ELOG( "Error : Unsupported Format(%u).", format ); }
        break;
    }

    return result;
}

//-------------------------------------------------------------------------------------------------
//      サーフェイス情報を取得します.
//-------------------------------------------------------------------------------------------------
void GetSurfaceInfo
(
    const int32_t   width,
    const int32_t   height,
    const uint32_t  format,
    uint32_t*       pNumBytes, 
    uint32_t*       pRowBytes,
    uint32_t*       pNumRows 
)
{
    auto numBytes = 0;
    auto rowBytes = 0;
    auto numRows  = 0;

    auto bc     = false;
    auto packed = false;
    uint32_t bpe = 0;

    switch (format)
    {
    case asdx::DDS_FORMAT_BC1_UNORM:
    case asdx::DDS_FORMAT_BC4_UNORM:
    case asdx::DDS_FORMAT_BC4_SNORM:
        {
            bc = true;
            bpe = 8;
        }
        break;

    case asdx::DDS_FORMAT_BC2_UNORM:
    case asdx::DDS_FORMAT_BC3_UNORM:
    case asdx::DDS_FORMAT_BC5_UNORM:
    case asdx::DDS_FORMAT_BC5_SNORM:
    case asdx::DDS_FORMAT_BC6H_SF16:
    case asdx::DDS_FORMAT_BC6H_UF16:
    case asdx::DDS_FORMAT_BC7_UNORM:
        {
            bc = true;
            bpe = 16;
        }
        break;

#if 0
    //case asdx::DDS_FORMAT_R8G8_B8G8_UNORM:
    //case asdx::DDS_FORMAT_G8R8_G8B8_UNORM:
    //case asdx::DDS_FORMAT_YUY2:
    //    {
    //        packed = true;
    //        bpe = 4;
    //    }
    //    break;
#endif
    }

    if ( bc )
    {
        uint32_t numBlockWide = 0;
        if ( width > 0 )
        { numBlockWide = asdx::Max<uint32_t>( 1, (width + 3) / 4 ); }

        uint32_t numBlockHeigh = 0;
        if ( height > 0 )
        { numBlockHeigh = asdx::Max<uint32_t>( 1, (height + 3) / 4 ); }

        rowBytes = numBlockWide * bpe;
        numRows  = numBlockHeigh;
        numBytes = rowBytes * numRows;
    }
    else if ( packed )
    {
        rowBytes = (( width + 1 ) >> 1 ) * bpe;
        numRows  = height;
        numBytes = rowBytes * numRows;
    }
    else
    {
        auto bpp = GetBitPerPixel( format );
        rowBytes = ( width * bpp + 7 ) / 8;
        numRows  = height;
        numBytes = rowBytes * numRows;
    }

    if ( pRowBytes )
    { (*pRowBytes) = rowBytes; }

    if ( pNumRows )
    { (*pNumRows) = numRows; }

    if ( pNumBytes )
    { (*pNumBytes) = numBytes; }
}

} // namespace /* anonymous */

namespace asdx {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Surface structure
//////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//      メモリを解放します.
//-------------------------------------------------------------------------------------------------
void Surface::Release()
{
    SafeDeleteArray( pPixels );
    Width      = 0;
    Height     = 0;
    Pitch      = 0;
    SlicePitch = 0;
}

//-------------------------------------------------------------------------------------------------
//      代入演算子です.
//-------------------------------------------------------------------------------------------------
Surface& Surface::operator = ( const Surface& value )
{
    Width      = value.Width;
    Height     = value.Height;
    Pitch      = value.Pitch;
    SlicePitch = value.SlicePitch;
    SafeDeleteArray( pPixels );

    pPixels = new (std::nothrow) uint8_t [ SlicePitch ];
    assert( pPixels != nullptr );

    if ( pPixels )
    { memcpy( pPixels, value.pPixels, SlicePitch * sizeof(uint8_t) ); }

    return (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ResDDS class
//////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//      コンストラクタです.
//-------------------------------------------------------------------------------------------------
ResDDS::ResDDS()
: m_Width       ( 0 )
, m_Height      ( 0 )
, m_Depth       ( 0 )
, m_SurfaceCount( 0 )
, m_MipMapCount ( 0 )
, m_Format      ( 0 )
, m_Dimension   ( DDS_RESOURCE_DIMENSION_TEXTURE2D )
, m_IsCubeMap   ( false )
, m_pSurfaces   ( nullptr )
{ /* DO_NOTHING */ }

//-------------------------------------------------------------------------------------------------
//      デストラクタです.
//-------------------------------------------------------------------------------------------------
ResDDS::~ResDDS()
{
    Release();
}

//-------------------------------------------------------------------------------------------------
//      ファイルから読み込みします.
//-------------------------------------------------------------------------------------------------
bool ResDDS::Load(const wchar_t* filename) 
{
    if ( filename == nullptr )
    {
        ELOG( "Error : Invalid Argument." );
        return false;
    }

    FILE* pFile;
    auto err = _wfopen_s( &pFile, filename, L"rb" );
    if ( err != 0 )
    {
        ELOG( "Error : File Open Failed. filename = %ls", filename );
        return false;
    }

    char sig[4];
    fread( sig, sizeof(char), 4, pFile );

    if ( (sig[0] != 'D')
      || (sig[1] != 'D')
      || (sig[2] != 'S')
      || (sig[3] != ' '))
    {
        ELOG( "Error : Invalid File." );
        fclose( pFile );
        return false;
    }

    auto width  = 0;
    auto height = 0;
    auto depth  = 0;
    auto mipMapCount  = 1;
    auto surfaceCount = 1;
    auto isCubeMap    = false;
    auto isVolume     = false;
    auto dimension    = DDS_RESOURCE_DIMENSION_TEXTURE2D;
    auto format       = (uint32_t)DDS_FORMAT_UNKNOWN;

    DDS_SURFACE_DESC desc;
    fread( &desc, sizeof(desc), 1, pFile );

    if ( desc.Flags & DDSD_HEIGHT )
    { height = desc.Height; }

    if ( desc.Flags & DDSD_WIDTH )
    { width = desc.Width; }

    if ( desc.Flags & DDSD_DEPTH )
    { depth = desc.Depth; }

    if ( desc.Flags & DDSD_MIPMAPCOUNT )
    { mipMapCount = desc.MipMapLevels; }

    if ( desc.Caps & DDSCAPS_COMPLEX )
    {
        if ( desc.Caps2 & DDSCAPS2_CUBEMAP )
        {
            surfaceCount = 0;
            if ( desc.Caps2 & DDSCAPS2_CUBEMAP_NEGATIVE_X ) { surfaceCount++; }
            if ( desc.Caps2 & DDSCAPS2_CUBEMAP_NEGATIVE_Y ) { surfaceCount++; }
            if ( desc.Caps2 & DDSCAPS2_CUBEMAP_NEGATIVE_Z ) { surfaceCount++; }
            if ( desc.Caps2 & DDSCAPS2_CUBEMAP_POSITIVE_X ) { surfaceCount++; }
            if ( desc.Caps2 & DDSCAPS2_CUBEMAP_POSITIVE_Y ) { surfaceCount++; }
            if ( desc.Caps2 & DDSCAPS2_CUBEMAP_POSITIVE_Z ) { surfaceCount++; }
            assert( surfaceCount == 6 );

            if ( surfaceCount == 6 )
            { isCubeMap = true; }
        }
        else if ( desc.Caps2 & DDSCAPS2_VOLUME )
        { isVolume = true; }
    }

    if ( width > 0 && height == 1 && depth == 0 )
    { dimension = DDS_RESOURCE_DIMENSION_TEXTURE1D; }
    else if ( width > 0 && height > 0 && depth == 0 )
    { dimension = DDS_RESOURCE_DIMENSION_TEXTURE2D; }
    else if ( width > 0 && height > 0 && depth > 0 && isVolume )
    { dimension = DDS_RESOURCE_DIMENSION_TEXTURE3D; }

    // ピクセルフォーマット有効.
    if ( desc.Flags & DDSD_PIXELFORMAT )
    {
        if ( desc.PixelFormat.Flags & DDPF_FOURCC )
        {
            switch( desc.PixelFormat.FourCC )
            {
            case FOURCC_DXT1:
                { format = DDS_FORMAT_BC1_UNORM; }
                break;

            case FOURCC_DXT2:
            case FOURCC_DXT3:
                { format = DDS_FORMAT_BC2_UNORM; }
                break;

            case FOURCC_DXT4:
            case FOURCC_DXT5:
                { format = DDS_FORMAT_BC3_UNORM; }
                break;

            case FOURCC_ATI1:
            case FOURCC_BC4U:
                { format = DDS_FORMAT_BC4_UNORM; }
                break;

            case FOURCC_BC4S:
                { format = DDS_FORMAT_BC4_SNORM; }
                break;

            case FOURCC_ATI2:
            case FOURCC_BC5U:
                { format = DDS_FORMAT_BC5_UNORM; }
                break;

            case FOURCC_BC5S:
                { format = DDS_FORMAT_BC5_SNORM; }
                break;

            case FOURCC_DX10:
                {
                    DDS_DXT10_HEADER ext;
                    fread( &ext, sizeof(ext), 1, pFile );

                    format = ext.DXGIFormat;
                    surfaceCount = ext.ArraySize;

                    switch( ext.ResourceDimension )
                    {
                    case DDS_RESOURCE_DIMENSION_TEXTURE1D:
                        {
                            if ( height != 1 )
                            {
                                ELOG( "Error : Texture1D Height is must be 1." );
                                fclose( pFile );
                                return false;
                            }

                            dimension = DDS_RESOURCE_DIMENSION_TEXTURE1D;
                        }
                        break;

                    case DDS_RESOURCE_DIMENSION_TEXTURE2D:
                        {
                            if ( ext.MiscFlag & DDS_RESOURCE_MISC_TEXTRECUBE )
                            { surfaceCount = ext.ArraySize * 6; }

                            dimension = DDS_RESOURCE_DIMENSION_TEXTURE2D;
                        }
                        break;

                    case DDS_RESOURCE_DIMENSION_TEXTURE3D:
                        {
                            if ( !isVolume )
                            {
                                ELOG( "Error : Invalid Texture3D. Volume Flag is none." );
                                fclose( pFile );
                                return false;
                            }

                            if ( surfaceCount > 1 )
                            {
                                ELOG( "Error : Texture3D is not support array." );
                                fclose( pFile );
                                return false;
                            }

                            dimension = DDS_RESOURCE_DIMENSION_TEXTURE3D;
                        }
                        break;
                    }
                }
                break;

        #if 0
            //case FOURCC_RGBG:
            //    { format = DDS_FORMAT_R8G8_B8G8_UNORM; }
            //    break;

            //case FOURCC_GRGB:
            //    { format = DDS_FORMAT_G8R8_G8B8_UNORM; }
            //    break;

            //case FOURCC_YUY2:
            //    { format = DDS_FORMAT_YUY2; }
            //    break;
        #endif

            case FOURCC_A16B16G16R16:
                { format = DDS_FORMAT_R16G16B16A16_UNORM; }
                break;

            case FOURCC_Q16W16V16U16:
                { format = DDS_FORMAT_R16G16B16A16_UNORM; }
                break;

            case FOURCC_R16F:
                { format = DDS_FORMAT_R16_FLOAT; }
                break;

            case FOURCC_G16R16F:
                { format = DDS_FORMAT_R16G16_FLOAT; }
                break;

            case FOURCC_A16B16G16R16F:
                { format = DDS_FORMAT_R16G16B16A16_FLOAT; }
                break;

            case FOURCC_R32F:
                { format = DDS_FORMAT_R32_FLOAT; }
                break;

            case FOURCC_G32R32F:
                { format = DDS_FORMAT_R32G32_FLOAT; }
                break;

            case FOURCC_A32B32G32R32F:
                { format = DDS_FORMAT_R32G32B32A32_FLOAT; }
                break;
            }
        }
        else if ( desc.PixelFormat.Flags & DDPF_RGB )
        {
            switch( desc.PixelFormat.Bpp )
            {
            case 32:
                {
                    if ( CheckMask( desc.PixelFormat, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 ) )
                    { format = DDS_FORMAT_R8G8B8A8_UNORM; }

                    if ( CheckMask( desc.PixelFormat, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000 ) )
                    { format = DDS_FORMAT_R8G8B8X8_UNORM; }

                    if ( CheckMask( desc.PixelFormat, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 ) )
                    { format = DDS_FORMAT_B8G8R8A8_UNORM; }

                    if ( CheckMask( desc.PixelFormat, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 ) )
                    { format = DDS_FORMAT_B8G8R8X8_UNORM; }
 
                    if ( CheckMask( desc.PixelFormat, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000 ) )
                    { format = DDS_FORMAT_R10G10B10A2_UNORM; }

                    //if ( CheckMask( desc.PixelFormat, 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000 ) )
                    //{ format = DDS_FORMAT_B10G10R10A2_UNORM; }

                    if ( CheckMask( desc.PixelFormat, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000 ) )
                    { format = DDS_FORMAT_R16G16_UNORM; }

                    if ( CheckMask( desc.PixelFormat, 0xffffffff, 0x00000000,0x00000000,0x00000000 ) )
                    { format = DDS_FORMAT_R32_FLOAT; }
                }
                break;

            case 24:
                { format = DDS_FORMAT_B8G8R8_UNORM; }
                break;

            case 16:
                {
                    if ( CheckMask( desc.PixelFormat, 0x7c00, 0x03e0, 0x001f, 0x8000 ) )
                    { format = DDS_FORMAT_B5G5R5A1_UNORM; }

                    if ( CheckMask( desc.PixelFormat, 0x7c00, 0x03e0, 0x001f, 0x0000 ) )
                    { format = DDS_FORMAT_B5G5R5X1_UNORM; }

                    if ( CheckMask( desc.PixelFormat, 0xf800, 0x07e0, 0x001f, 0x0000 ) )
                    { format = DDS_FORMAT_B5G6R5_UNORM; }

                    if ( CheckMask( desc.PixelFormat, 0x0f00, 0x00f0, 0x000f, 0xf000 ) )
                    { format = DDS_FORMAT_B4G4R4A4_UNORM; }

                    if ( CheckMask( desc.PixelFormat, 0x0f00, 0x00f0, 0x000f, 0x0000 ) )
                    { format = DDS_FORMAT_B4G4R4X4_UNORM; }
                }
                break;

            default:
                break;
            }
        }
        else if ( desc.PixelFormat.Flags & DDPF_LUMINANCE )
        {
            switch( desc.PixelFormat.Bpp )
            {
            case 8:
                {
                    if ( CheckMask( desc.PixelFormat, 0x000000ff, 0x00000000, 0x00000000, 0x00000000 ) )
                    { format = DDS_FORMAT_R8_UNORM; }
                }
                break;

            case 16:
                {
                    if ( CheckMask( desc.PixelFormat, 0x0000ffff, 0x00000000, 0x00000000, 0x00000000 ) )
                    { format = DDS_FORMAT_R16_UNORM; }

                    if ( CheckMask( desc.PixelFormat, 0x000000ff, 0x00000000, 0x00000000, 0x0000ff00 ) )
                    { format = DDS_FORMAT_R8G8_UNORM; }
                }
                break;
            }
        }
        else if ( desc.PixelFormat.Flags & DDPF_ALPHA )
        {
            if ( 8 == desc.PixelFormat.Bpp )
            { format = DDS_FORMAT_A8_UNORM; }
        }
    }

    if ( format == DDS_FORMAT_UNKNOWN )
    {
        ELOG( "Error : Unsupported Format." );
        return false;
    }

    auto cur = ftell( pFile );
    fseek( pFile, 0, SEEK_END );
    auto end = ftell( pFile );
    fseek( pFile, cur, SEEK_SET );
    auto pixelSize = end - cur;

    auto pPixels = new (std::nothrow) uint8_t [ pixelSize ];
    assert( pPixels != nullptr );
    if ( pPixels == nullptr )
    {
        ELOG( "Error : Out of Memory." );
        fclose( pFile );
        return false;
    }

    fread( pPixels, sizeof(uint8_t), pixelSize, pFile );
    fclose( pFile );

    auto pSurfaces = new (std::nothrow) Surface[ mipMapCount * surfaceCount ];
    assert( pSurfaces != nullptr );
    if ( pSurfaces == nullptr )
    {
        ELOG( "Error : Out of Memory." );
        SafeDeleteArray( pPixels );
        return false;
    }

    uint32_t w = width;
    uint32_t h = height;
    uint32_t d = depth;
    uint32_t offset = 0;

    for( auto j=0; j<surfaceCount; ++j )
    {
        for( auto i=0; i<mipMapCount; ++i )
        {
            auto idx = ( mipMapCount * j ) + i;
            uint32_t rowBytes = 0;
            uint32_t numRows  = 0;
            uint32_t numBytes = 0;

            GetSurfaceInfo( w, h, format, &numBytes, &rowBytes, &numRows );

            pSurfaces[ idx ].Width      = w;
            pSurfaces[ idx ].Height     = h;
            pSurfaces[ idx ].Pitch      = rowBytes;
            pSurfaces[ idx ].SlicePitch = numBytes;
            pSurfaces[ idx ].pPixels    = new (std::nothrow) uint8_t [ numBytes ];
            assert( pSurfaces[ idx ].pPixels != nullptr );

            if ( pSurfaces[ idx ].pPixels == nullptr )
            {
                ELOG( "Error : Out of Memory." );
                for( auto k=0; k<idx; ++k )
                { pSurfaces[k].Release(); }

                SafeDeleteArray( pPixels );
                SafeDeleteArray( pSurfaces );
                return false;
            }

            memcpy( pSurfaces[ idx ].pPixels, pPixels + offset, numBytes );

            if ( depth != 0 )
            { offset += numBytes * d; }
            else
            { offset += numBytes; }

            w = w >> 1;
            h = h >> 1;
            d = d >> 1;

            if ( w == 0 ) { w = 1; }
            if ( h == 0 ) { h = 1; }
            if ( d == 0 ) { d = 1; }
        }
    }

    SafeDeleteArray( pPixels );

    m_Width         = width;
    m_Height        = height;
    m_Depth         = depth;
    m_Dimension     = dimension;
    m_Format        = format;
    m_SurfaceCount  = surfaceCount;
    m_MipMapCount   = mipMapCount;
    m_IsCubeMap     = isCubeMap;
    m_pSurfaces     = pSurfaces;

    return true;
}

//-------------------------------------------------------------------------------------------------
//      メモリを解放します.
//-------------------------------------------------------------------------------------------------
void ResDDS::Release()
{
    auto size = m_SurfaceCount * m_MipMapCount;
    for( uint32_t i=0; i<size; ++i )
    { m_pSurfaces[i].Release(); }

    SafeDeleteArray( m_pSurfaces );
    m_Width         = 0;
    m_Height        = 0;
    m_Depth         = 0;
    m_SurfaceCount  = 0;
    m_MipMapCount   = 0;
    m_Dimension     = DDS_RESOURCE_DIMENSION_TEXTURE2D;
    m_IsCubeMap     = false;
}

//-------------------------------------------------------------------------------------------------
//      画像の横幅を取得します.
//-------------------------------------------------------------------------------------------------
const uint32_t ResDDS::GetWidth() const
{ return m_Width; }

//-------------------------------------------------------------------------------------------------
//      画像の縦幅を取得します.
//-------------------------------------------------------------------------------------------------
const uint32_t ResDDS::GetHeight() const
{ return m_Height; }

//-------------------------------------------------------------------------------------------------
//      画像の奥行を取得します.
//-------------------------------------------------------------------------------------------------
const uint32_t ResDDS::GetDepth() const
{ return m_Depth; }

//-------------------------------------------------------------------------------------------------
//      サーフェイス数を取得します.
//-------------------------------------------------------------------------------------------------
const uint32_t ResDDS::GetSurfaceCount() const
{ return m_SurfaceCount; }

//-------------------------------------------------------------------------------------------------
//      ミップマップ数を取得します.
//-------------------------------------------------------------------------------------------------
const uint32_t ResDDS::GetMipMapCount() const
{ return m_MipMapCount; }

//-------------------------------------------------------------------------------------------------
//      フォーマットを取得します.
//-------------------------------------------------------------------------------------------------
const uint32_t ResDDS::GetFormat() const
{ return m_Format; }

//-------------------------------------------------------------------------------------------------
//      次元数を取得します.
//-------------------------------------------------------------------------------------------------
const DDS_RESOURCE_DIMENSION ResDDS::GetDimension() const
{ return m_Dimension; }

//-------------------------------------------------------------------------------------------------
//      キューブマップかどうかを取得します.
//-------------------------------------------------------------------------------------------------
const bool ResDDS::IsCubeMap() const
{ return m_IsCubeMap; }

//-------------------------------------------------------------------------------------------------
//      サーフェイスを取得します.
//-------------------------------------------------------------------------------------------------
const Surface* ResDDS::GetSurfaces() const
{ return m_pSurfaces; }

} // namespace asdx


