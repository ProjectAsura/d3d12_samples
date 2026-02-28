//-----------------------------------------------------------------------------
// File : asdxFont.cpp
// Desc : Font Rasterizer.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cassert>
#include <cstdarg>
#include <asdxFont.h>
#include <asdxLogger.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../external/imgui/imstb_truetype.h"


namespace {

//-----------------------------------------------------------------------------
//      ファイルをロードします.
//-----------------------------------------------------------------------------
bool LoadFile(const char* path, std::vector<uint8_t>& binary)
{
    FILE* pFile = nullptr;
    auto err = fopen_s(&pFile, path, "rb");
    if (err != 0)
    {
        return false;
    }

    auto pos = ftell(pFile);
    fseek(pFile, 0, SEEK_END);
    auto end = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);
    auto size = end - pos;

    binary.resize(size);
    fread(binary.data(), size, 1, pFile);
    fclose(pFile);

    return true;
}

} // namespace

namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// Font class
///////////////////////////////////////////////////////////////////////////////
struct Font::Body
{
    float                   Ascent;
    float                   Descent;
    float                   LineGap;
    float                   Scale;
    float                   Height;
    stbtt_fontinfo          Font;
    std::vector<uint8_t>    Binary;
};

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
Font::Font()
: m_pBody(nullptr)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
Font::~Font()
{
    // Term()ちゃんと明示的に呼んでね!
    assert(m_pBody == nullptr);
}

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool Font::Init(const char* path, float fontSize)
{
    m_pBody = new Font::Body();

    if (!LoadFile(path, m_pBody->Binary))
    {
        ELOGA("Error : LoadFile() Failed. path = %s", path);
        Term();
        return false;
    }

    auto offset = stbtt_GetFontOffsetForIndex(m_pBody->Binary.data(), 0);
    if (!stbtt_InitFont(&m_pBody->Font, m_pBody->Binary.data(), offset))
    {
        ELOGA("Error : stbtt_InitFont() Failed.");
        Term();
        return false;
    }

    const auto fontScale = stbtt_ScaleForPixelHeight(&m_pBody->Font, fontSize);

    int ascent  = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_GetFontVMetrics(&m_pBody->Font, &ascent, &descent, &lineGap);

    m_pBody->Ascent  = roundf(ascent * fontScale);
    m_pBody->Descent = roundf(descent * fontScale);
    m_pBody->LineGap = roundf(lineGap * fontScale);
    m_pBody->Scale   = fontScale;
    m_pBody->Height  = (m_pBody->Ascent - m_pBody->Descent) + m_pBody->LineGap;

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void Font::Term()
{
    if (m_pBody != nullptr)
    {
        delete m_pBody;
        m_pBody = nullptr;
    }
}

//-----------------------------------------------------------------------------
//      ラスタライズに必要なサイズを求めます.
//-----------------------------------------------------------------------------
void Font::CalcSize(const wchar_t* text, uint32_t& w, uint32_t& h) const
{
    assert(m_pBody != nullptr);

    auto fw = 0.0f;
    auto fh = m_pBody->Height;

    auto count  = wcslen(text);
    for(auto i=0; i<count; ++i)
    {
        auto code = int(text[i]);
        if (code == '\n')
        {
            // 1行分進める.
            fh += m_pBody->Height;

            // 最大値を記録.
            w = (fw > float(w)) ? uint32_t(fw) : w;

            // リセット.
            fw = 0.0f;

            continue;
        }

        int advance = 0;
        int lsb     = 0;
        stbtt_GetCodepointHMetrics(&m_pBody->Font, code, &advance, &lsb);
        fw += roundf(m_pBody->Scale * advance);

        auto kern = stbtt_GetCodepointKernAdvance(
            &m_pBody->Font, code, int(text[i + 1]));
        fw += roundf(kern * m_pBody->Scale);
    }

    w = (fw > float(w)) ? uint32_t(fw) : w;
    h = uint32_t(fh);
}

//-----------------------------------------------------------------------------
//      ラスタライズ処理を行います.
//-----------------------------------------------------------------------------
Font::Bitmap Font::Rasterize(const wchar_t* text) const
{
    assert(m_pBody != nullptr);
    Font::Bitmap result;

    // メモリサイズ計算.
    CalcSize(text, result.Width, result.Height);

    // メモリ確保.
    result.Resize();

    // ラスタライズ.
    Rasterize(result, text);

    return result;
}

//-----------------------------------------------------------------------------
//      フォーマットを指定してラスタライズ処理を行います.
//-----------------------------------------------------------------------------
Font::Bitmap Font::RasterizeFormat(const wchar_t* format, ...) const
{
    wchar_t buffer[2048] = {};

    va_list arg;
    va_start(arg, format);
    vswprintf_s(buffer, format, arg);
    va_end(arg);

    return Rasterize(buffer);
}

//-----------------------------------------------------------------------------
//      指定されたメモリにラスタライズ処理を行います.
//-----------------------------------------------------------------------------
void Font::Rasterize(Bitmap& bitmap, const wchar_t* text) const
{
    auto x = 0;
    auto y = m_pBody->Ascent;

    auto pixel = bitmap.Pixels.data();

    auto count = wcslen(text);
    for(auto i=0; i<count; ++i)
    {
        auto code = int(text[i]);
        if (code == '\n')
        {
            y += m_pBody->Height;
            x = 0;
            continue;
        }

        int advance = 0;
        int lsb     = 0;
        stbtt_GetCodepointHMetrics(&m_pBody->Font, code, &advance, &lsb);

        int x0 = 0;
        int x1 = 0;
        int y0 = 0;
        int y1 = 0;
        stbtt_GetCodepointBitmapBoxSubpixel(
            &m_pBody->Font,
            code,
            m_pBody->Scale,
            m_pBody->Scale,
            0.0f,
            0.0f,
            &x0, &y0, &x1, &y1);

        auto offset = int(x + roundf(lsb * m_pBody->Scale) + ((y + y0) * bitmap.Width));
        auto w = x1 - x0;
        auto h = y1 - y0;
        stbtt_MakeCodepointBitmapSubpixel(
            &m_pBody->Font,
            pixel + offset,
            w, h,
            bitmap.Width,
            m_pBody->Scale,
            m_pBody->Scale,
            0.0f,
            0.0f,
            code);
        
        x += int(roundf(advance * m_pBody->Scale));

        auto kern = stbtt_GetCodepointKernAdvance(
            &m_pBody->Font, code, int(text[i + 1]));
        x += int(roundf(kern * m_pBody->Scale));
    }
}

//-----------------------------------------------------------------------------
//      フォーマットを指定して指定されたメモリにラスタライズ処理を行います.
//-----------------------------------------------------------------------------
void Font::RasterizeFormat(Bitmap& bitmap, const wchar_t* format, ...) const
{
    wchar_t buffer[2048] = {};

    va_list arg;
    va_start(arg, format);
    vswprintf_s(buffer, format, arg);
    va_end(arg);

    Rasterize(bitmap, buffer);
}

} // namespace asdx
