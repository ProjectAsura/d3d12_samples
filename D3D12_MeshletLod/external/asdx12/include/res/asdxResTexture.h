//-----------------------------------------------------------------------------
// File : asdxResTexture.h
// Desc : Texture Resource.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cstdint>


namespace asdx {

//-----------------------------------------------------------------------------
//      nullptrを考慮してdelete[]を呼び出します.
//-----------------------------------------------------------------------------
template<typename T>
void SafeDeleteArray(T*&ptr)
{
    if (ptr != nullptr)
    {
        delete[] ptr;
        ptr = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
// TEXTURE_DIMENSION enum
///////////////////////////////////////////////////////////////////////////////
enum TEXTURE_DIMENSION
{
    TEXTURE_DIMENSION_UNKNOWN,
    TEXTURE_DIMENSION_1D,
    TEXTURE_DIMENSION_2D,
    TEXTURE_DIMENSION_3D,
    TEXTURE_DIMENSION_CUBE
};

///////////////////////////////////////////////////////////////////////////////
// SubResource structure
///////////////////////////////////////////////////////////////////////////////
struct SubResource
{
    uint32_t     Width;          //!< 横幅です.
    uint32_t     Height;         //!< 縦幅です.
    uint32_t     MipIndex;       //!< ミップ番号です.
    uint32_t     Pitch;          //!< 1行当たりのバイト数です.
    uint32_t     SlicePitch;     //!< 1スライス当たりのバイト数です(つまり，テクセルデータのバイト数).
    uint8_t*     pPixels;        //!< テクセルデータです.

    //-------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //-------------------------------------------------------------------------
    SubResource()
    : Width     ( 0 )
    , Height    ( 0 )
    , MipIndex  ( 0 )
    , Pitch     ( 0 )
    , SlicePitch( 0 )
    , pPixels   ( nullptr )
    { /* DO_NOTHING */ }

    //-------------------------------------------------------------------------
    //! @brief      解放処理を行います.
    //-------------------------------------------------------------------------
    void Release()
    { SafeDeleteArray( pPixels ); }
};


///////////////////////////////////////////////////////////////////////////////
// ResTexture structure
///////////////////////////////////////////////////////////////////////////////
struct ResTexture
{
    TEXTURE_DIMENSION    Dimension;      //!< 次元です.
    uint32_t             Width;          //!< 画像の横幅です.
    uint32_t             Height;         //!< 画像の縦幅です.
    uint32_t             Depth;          //!< 画像の奥行です.
    uint32_t             Format;         //!< 画像のフォーマットです.
    uint32_t             MipMapCount;    //!< ミップマップ数です.
    uint32_t             SurfaceCount;   //!< サーフェイス数です(1次元配列テクスチャ, 2次元配列テクスチャ, キューブマップの場合のみ6以上の数が入ります).
    SubResource*         pResources;     //!< サブリソースです.

    //-------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //-------------------------------------------------------------------------
    ResTexture()
    : Dimension     ( TEXTURE_DIMENSION_UNKNOWN )
    , Width         ( 0 )
    , Height        ( 0 )
    , Depth         ( 0 )
    , Format        ( 0 )
    , MipMapCount   ( 0 )
    , SurfaceCount  ( 0 )
    , pResources    ( nullptr )
    { /* DO_NOTHING */ }

    //-------------------------------------------------------------------------
    //! @brief      解放処理を行います.
    //-------------------------------------------------------------------------
    void Dispose()
    {
        uint32_t mipCount = ( MipMapCount > 0 ) ? MipMapCount : 1;

        for( uint32_t i=0; i<SurfaceCount * mipCount; ++i )
        { pResources[i].Release(); }

        SafeDeleteArray( pResources );
    }

    //---------------------------------------------------------------------------------------------
    //! @brief      ファイルからテクスチャリソースを生成します.
    //!             読み込み可能なファイルはDDS, BMP, JPG, PNG, TIFF, GIF, HDP, TGA, HDRです.
    //!
    //! @param[in]      filename        ファイル名です.
    //! @retval true    リソース生成に成功.
    //! @retval false   リソース生成に失敗.
    //---------------------------------------------------------------------------------------------
    bool LoadFromFileA(const char* filename);

    //---------------------------------------------------------------------------------------------
    //! @brief      ファイルからテクスチャリソースを生成します.
    //!             読み込み可能なファイルはDDS, BMP, JPG, PNG, TIFF, GIF, HDP, TGA, HDRです.
    //!
    //! @param[in]      filename        ファイル名です.
    //! @retval true    リソース生成に成功.
    //! @retval false   リソース生成に失敗.
    //---------------------------------------------------------------------------------------------
    bool LoadFromFileW(const wchar_t* filename);

    //---------------------------------------------------------------------------------------------
    //! @brief      メモリストリームからテクスチャリソースを生成します.
    //!             メモリストリームの形式は DDS, BMP, JPG, PNG, TIFF, GIF, HDP である必要があります.
    //!
    //! @param[in]      pBuffer         バッファです.
    //! @param[in]      bufferSize      バッファサイズです.
    //! @retval true    リソース生成に成功.
    //! @retval false   リソース生成に失敗.
    //---------------------------------------------------------------------------------------------
    bool LoadFromMemory(const uint8_t* pBuffer, uint32_t bufferSize);
};

} // namespace asdx
