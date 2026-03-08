//-----------------------------------------------------------------------------
// File : asdxTablet.h
// Desc : Table Device.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

#ifdef ASDX_ENABLE_PENTAB

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cstdint>
#include <string>
#include <map>
#include <Windows.h>
#include <wintab/WINTAB.H>

#define PACKETDATA ( PK_X | PK_Y | PK_Z | PK_BUTTONS | PK_NORMAL_PRESSURE | PK_ORIENTATION | PK_ROTATION | PK_CURSOR | PK_TIME)
#define PACKETMODE          PK_BUTTONS
#define PACKETEXPKEYS       PKEXT_ABSOLUTE
#define PACKETTOUCHSTRIP    PKEXT_ABSOLUTE
#define PACKETTOUCHRING     PKEXT_ABSOLUTE
#include <wintab/PKTDEF.H>


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// Tablet class
///////////////////////////////////////////////////////////////////////////////
class Tablet
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    /* NOTHING */

public:
    ///////////////////////////////////////////////////////////////////////////
    // CURSOR_TYPE
    ///////////////////////////////////////////////////////////////////////////
    enum CURSOR_TYPE
    {
        CURSOR_TYPE_INVALID     = -1,
        CURSOR_TYPE_CURSOR      = 0,
        CURSOR_TYPE_PEN         = 1,
        CURSOR_TYPE_TAIL_SWITCH = 2,
        CURSOR_TYPE_COUNT,
    };

    ///////////////////////////////////////////////////////////////////////////
    // BUTTON_STATE
    ///////////////////////////////////////////////////////////////////////////
    enum BUTTON_STATE
    {
        BUTTON_STATE_NONE,      // 何もしてない.
        BUTTON_STATE_DOWN,      // 押した瞬間.
        BUTTON_STATE_HOLD,      // 押しっぱなし.
        BUTTON_STATE_UP,        // 離した瞬間.
    };

    ///////////////////////////////////////////////////////////////////////////
    // KEY_STATE
    ///////////////////////////////////////////////////////////////////////////
    enum KEY_STATE
    {
        KEY_STATE_NONE,     // 何もしてない.
        KEY_STATE_DOWN,     // 押した瞬間.
        KEY_STATE_HOLD,     // 押しっぱなし.
        KEY_STATE_UP,       // 離した瞬間.
    };

    //=========================================================================
    // public variables.
    //=========================================================================
    /* NOTHING */

    //=========================================================================
    // public methods.
    //=========================================================================
    static bool InitLibrary();
    static void TermLibrary();
    static bool IsLibraryLoaded(); 

    Tablet();
    ~Tablet();
    bool Init(HWND hWnd);
    void Term();
    void Update();

    bool OnPacket   (LPARAM lp, WPARAM wp);
    bool OnPacketEx (LPARAM lp, WPARAM wp);
    void OnProximity(LPARAM lp, WPARAM wp);
    void Overwrap();
    void Enable(WPARAM wp);

    bool SupportPressure() const;
    bool SupportWheel() const;
    bool SupportOrientation() const;
    bool SupportKeys() const;

    const std::string& GetDeviceName() const;
    const std::string& GetVersion() const;
    uint32_t GetDeviceCount() const;
    uint32_t GetKeyCount(uint32_t tabletId) const;
    int GetCursorX() const;
    int GetCursorY() const;
    float GetX() const;
    float GetY() const;
    float GetPressure() const;
    float GetWheel() const;
    float GetAzimuth() const;
    float GetAltitude() const;
    float GetTwist() const;
    uint32_t GetPenId() const;
    CURSOR_TYPE GetCursorType() const;
    uint32_t GetTime() const;
    bool GetProximity() const;
    bool IsButtonDown(uint16_t id) const;
    bool IsButtonHold(uint16_t id) const;
    bool IsButtonUp(uint16_t id) const;
    bool IsKeyDown(uint16_t tabletId, uint16_t controlId) const;
    bool IsKeyHold(uint16_t tabletId, uint16_t controlId) const;
    bool IsKeyUp(uint16_t tabletId, uint16_t controlId) const;

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    static HINSTANCE s_hLibrary;
    HWND        m_hWnd                  = nullptr;
    HCTX        m_hContext              = nullptr;
    std::string m_Version;
    std::string m_DeviceName;
    PACKET      m_Packet                = {};
    PACKETEXT   m_PacketEx              = {};
    bool        m_SupportPressure       = false;
    bool        m_SupportWheel          = false;
    bool        m_SupportOrientation    = false;
    bool        m_SupportKeys           = false;
    WTPKT       m_KeyMask               = 0;
    uint32_t    m_DeviceCount           = 0;
    AXIS        m_CoordX                = {};
    AXIS        m_CoordY                = {};
    AXIS        m_Pressure              = {};
    AXIS        m_Wheel                 = {};
    AXIS        m_Azimuth               = {};
    AXIS        m_Altitude              = {};
    AXIS        m_Twist                 = {};
    bool        m_Proximity             = false;
    int         m_CursorX               = 0;
    int         m_CursorY               = 0;
    std::map<uint16_t, BUTTON_STATE>                  m_Buttons;
    std::map<uint16_t, std::map<uint16_t, KEY_STATE>> m_Keys;

    //=========================================================================
    // private methods.
    //=========================================================================
    bool FindExtenstion(uint32_t extension, uint32_t& index);

    template<typename T>
    T ExtGet(
        uint32_t    extension,
        uint8_t     tabletId,
        uint8_t     controlId,
        uint8_t     functionId,
        uint16_t    property);

    template<typename T>
    bool ExtSet(
        uint32_t    extension,
        uint8_t     tabletId,
        uint8_t     controlId,
        uint8_t     functionId,
        uint16_t    property,
        T           value);

    bool IsValidButton(uint16_t id) const;
    bool IsValidKey(uint16_t tabletId, uint16_t controlId) const;
    void UpdateButtonStates();
    void UpdateKeyStates();
};

} // namespace asdx

#endif//ASDX_ENABLE_PENTAB