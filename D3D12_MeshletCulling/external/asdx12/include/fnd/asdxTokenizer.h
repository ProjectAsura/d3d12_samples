//-----------------------------------------------------------------------------
// File : asdxTokenizer.h
// Desc : Tokenizer Module.
// Copyright(c) Project Asura All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cstdint>
#include <string>


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// Tokenizer class
///////////////////////////////////////////////////////////////////////////////
class Tokenizer
{
    //=========================================================================
    // list of friend classes
    //=========================================================================
    /* NOTHING */

public:
    //=========================================================================
    // public variables
    //=========================================================================
    /* NOTHING */

    //=========================================================================
    // public methods
    //=========================================================================
    Tokenizer();
    virtual ~Tokenizer();

    bool        Init            ( uint32_t size );
    void        Term            ();
    void        SetSeparator    ( const char* separator );
    void        SetCutOff       ( const char* cutoff );
    void        SetBuffer       ( char *buffer, size_t bufferSize);
    bool        Compare         ( const char *token ) const;
    bool        CompareAsLower  ( const char *token ) const;
    char*       Contain         ( const char *token ) const;
    bool        IsEnd           () const;
    bool        IsValidToken    () const;
    char*       GetAsChar       () const;
    double      GetAsDouble     () const;
    float       GetAsFloat      () const;
    int         GetAsInt        () const;
    bool        GetAsBool       () const;
    uint32_t    GetAsUint       () const;
    void        Next            ();
    char*       NextAsChar      ();
    double      NextAsDouble    ();
    float       NextAsFloat     ();
    int         NextAsInt       ();
    bool        NextAsBool      ();
    uint32_t    NextAsUint      ();
    char*       GetPtr          () const;
    char*       GetBuffer       () const;
    void        SkipTo          ( const char* text );
    void        SkipLine        ();

private:
    //=========================================================================
    // private variables
    //=========================================================================
    char*           m_pBuffer;      //!< 先頭ポインタ.
    char*           m_pPtr;         //!< バッファ位置です.
    char*           m_pToken;       //!< トークン.
    std::string     m_Separator;    //!< 区切り文字.
    std::string     m_CutOff;       //!< 切り出し文字.
    size_t          m_BufferSize;   //!< バッファサイズ.

    //=========================================================================
    // private methods
    //=========================================================================
    Tokenizer       (const Tokenizer&) = delete;
    void operator = (const Tokenizer&) = delete;
};

} // namespace asdx
