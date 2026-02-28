//-----------------------------------------------------------------------------
// File : asdxTablet.cpp
// Desc : Table Device.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <fnd/asdxTablet.h>

#ifdef ASDX_ENABLE_PENTAB
#include <fnd/asdxLogger.h>


namespace {

//-----------------------------------------------------------------------------
// Using Statements
//-----------------------------------------------------------------------------
using Func_WTInfoA                      = UINT      (API *)(UINT, UINT, LPVOID);
using Func_WTInfoW                      = UINT      (API *)(UINT, UINT, LPVOID);
using Func_WTOpenA                      = HCTX      (API *)(HWND, LPLOGCONTEXTA, BOOL);
using Func_WTOpenW                      = HCTX      (API *)(HWND, LPLOGCONTEXTW, BOOL);
using Func_WTClose                      = BOOL      (API *)(HCTX);
using Func_WTPacketsGet                 = int       (API *)(HCTX, int, LPVOID);
using Func_WTPacket                     = BOOL      (API *)(HCTX, UINT, LPVOID);
using Func_WTEnable                     = BOOL      (API *)(HCTX, BOOL);
using Func_WTOverlap                    = BOOL      (API *)(HCTX, BOOL);
using Func_WTConfig                     = BOOL      (API *)(HCTX, HWND);
using Func_WTGetA                       = BOOL      (API *)(HCTX, LPLOGCONTEXTA);
using Func_WTGetW                       = BOOL      (API *)(HCTX, LPLOGCONTEXTW);
using Func_WTSetA                       = BOOL      (API *)(HCTX, LPLOGCONTEXTA);
using Func_WTSetW                       = BOOL      (API *)(HCTX, LPLOGCONTEXTW);
using Func_WTExtGet                     = BOOL      (API *)(HCTX, UINT, LPVOID);
using Func_WTExtSet                     = BOOL      (API *)(HCTX, UINT, LPVOID);
using Func_WTSave                       = BOOL      (API *)(HCTX, LPVOID);
using Func_WTRestore                    = HCTX      (API *)(HWND, LPVOID, BOOL);
using Func_WTPacketsPeek                = int       (API *)(HCTX, int, LPVOID);
using Func_WTDataGet                    = int       (API *)(HCTX, UINT, UINT, int, LPVOID, LPINT);
using Func_WTDataPeek                   = int       (API *)(HCTX, UINT, UINT, int, LPVOID, LPINT);
using Func_WTQueueSizeGet               = int       (API *)(HCTX);
using Func_WTQueueSizeSet               = BOOL      (API *)(HCTX, int);
using Func_WTMgrOpen                    = HMGR      (API *)(HWND, UINT);
using Func_WTMgrClose                   = BOOL      (API *)(HMGR);
using Func_WTMgrContextEnum             = BOOL      (API *)(HMGR, WTENUMPROC, LPARAM);
using Func_WTMgrContextOwner            = HWND      (API *)(HMGR, HCTX);
using Func_WTMgrDefContext              = HCTX      (API *)(HMGR, BOOL);
using Func_WTMgrDefContextEx            = HCTX      (API *)(HMGR, UINT, BOOL);
using Func_WTMgrDeviceConfig            = UINT      (API *)(HMGR, UINT, HWND);
using Func_WTMgrExt                     = BOOL      (API *)(HMGR, UINT, LPVOID);
using Func_WTMgrCsrEnable               = BOOL      (API *)(HMGR, UINT, BOOL);
using Func_WTMgrCsrButtonMap            = BOOL      (API *)(HMGR, UINT, LPBYTE, LPBYTE);
using Func_WTMgrCsrPressureBtnMarks     = BOOL      (API *)(HMGR, UINT, DWORD, DWORD);
using Func_WTMgrCsrPressureResponse     = BOOL      (API *)(HMGR, UINT, UINT FAR*, UINT FAR*);
using Func_WTMgrCsrExt                  = BOOL      (API *)(HMGR, UINT, UINT, LPVOID);
using Func_WTQueuePacketsEx             = BOOL      (API *)(HCTX, UINT FAR*, UINT FAR*);
using Func_WTMgrConfigReplaceExA        = BOOL      (API *)(HMGR, BOOL, LPSTR, LPSTR);
using Func_WTMgrConfigReplaceExW        = BOOL      (API *)(HMGR, BOOL, LPWSTR, LPSTR);
using Func_WTMgrPacketHookExA           = HWTHOOK   (API *)(HMGR, int, LPSTR, LPSTR);
using Func_WTMgrPacketHookExW           = HWTHOOK   (API *)(HMGR, int, LPWSTR, LPSTR);
using Func_WTMgrPacketUnhook            = BOOL      (API *)(HWTHOOK);
using Func_WTMgrPacketHookNext          = LRESULT   (API *)(HWTHOOK, int, WPARAM, LPARAM);
using Func_WTMgrCsrPressureBtnMarksEx   = BOOL      (API *)(HMGR, UINT, UINT FAR*, UINT FAR*);

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
constexpr const char*       kDllName            = "Wintab32.dll";
constexpr const uint32_t    kMaxButtonCount     = 16;

//-----------------------------------------------------------------------------
// Global Variables.
//-----------------------------------------------------------------------------
Func_WTInfoA                      WTInfoA_                      = nullptr;
Func_WTInfoW                      WTInfoW_                      = nullptr;
Func_WTOpenA                      WTOpenA_                      = nullptr;
Func_WTOpenW                      WTOpenW_                      = nullptr;
Func_WTClose                      WTClose_                      = nullptr;
Func_WTPacketsGet                 WTPacketsGet_                 = nullptr;
Func_WTPacket                     WTPacket_                     = nullptr;
Func_WTEnable                     WTEnable_                     = nullptr;
Func_WTOverlap                    WTOverlap_                    = nullptr;
Func_WTConfig                     WTConfig_                     = nullptr;
Func_WTGetA                       WTGetA_                       = nullptr;
Func_WTGetW                       WTGetW_                       = nullptr;
Func_WTSetA                       WTSetA_                       = nullptr;
Func_WTSetW                       WTSetW_                       = nullptr;
Func_WTExtGet                     WTExtGet_                     = nullptr;
Func_WTExtSet                     WTExtSet_                     = nullptr;
Func_WTSave                       WTSave_                       = nullptr;
Func_WTRestore                    WTRestore_                    = nullptr;
Func_WTPacketsPeek                WTPacketsPeek_                = nullptr;
Func_WTDataGet                    WTDataGet_                    = nullptr;
Func_WTDataPeek                   WTDataPeek_                   = nullptr;
Func_WTQueueSizeGet               WTQueueSizeGet_               = nullptr;
Func_WTQueueSizeSet               WTQueueSizeSet_               = nullptr;
Func_WTMgrOpen                    WTMgrOpen_                    = nullptr;
Func_WTMgrClose                   WTMgrClose_                   = nullptr;
Func_WTMgrContextEnum             WTMgrContextEnum_             = nullptr;
Func_WTMgrContextOwner            WTMgrContextOwner_            = nullptr;
Func_WTMgrDefContext              WTMgrDefContext_              = nullptr;
Func_WTMgrDefContextEx            WTMgrDefContextEx_            = nullptr;
Func_WTMgrDeviceConfig            WTMgrDeviceConfig_            = nullptr;
Func_WTMgrExt                     WTMgrExt_                     = nullptr;
Func_WTMgrCsrEnable               WTMgrCsrEnable_               = nullptr;
Func_WTMgrCsrButtonMap            WTMgrCsrButtonMap_            = nullptr;
Func_WTMgrCsrPressureBtnMarks     WTMgrCsrPressureBtnMarks_     = nullptr;
Func_WTMgrCsrPressureResponse     WTMgrCsrPressureResponse_     = nullptr;
Func_WTMgrCsrExt                  WTMgrCsrExt_                  = nullptr;
Func_WTQueuePacketsEx             WTQueuePacketsEx_             = nullptr;
Func_WTMgrConfigReplaceExA        WTMgrConfigReplaceExA_        = nullptr;
Func_WTMgrConfigReplaceExW        WTMgrConfigReplaceExW_        = nullptr;
Func_WTMgrPacketHookExA           WTMgrPacketHookExA_           = nullptr;
Func_WTMgrPacketHookExW           WTMgrPacketHookExW_           = nullptr;
Func_WTMgrPacketUnhook            WTMgrPacketUnhook_            = nullptr;
Func_WTMgrPacketHookNext          WTMgrPacketHookNext_          = nullptr;
Func_WTMgrCsrPressureBtnMarksEx   WTMgrCsrPressureBtnMarksEx_   = nullptr;

#define GET_PROC(func, flag) \
    func##_ = (Func_##func)::GetProcAddress(s_hLibrary, #func); \
    flag |= (func##_ == nullptr);

} // namespace

namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// Tablet class
///////////////////////////////////////////////////////////////////////////////
HINSTANCE Tablet::s_hLibrary = nullptr;

//-----------------------------------------------------------------------------
//      ライブラリを初期化します.
//-----------------------------------------------------------------------------
bool Tablet::InitLibrary()
{
    // 初期化済みなら終了.
    if (IsLibraryLoaded())
    { return false; }

    // DLL読み込み.
    s_hLibrary = LoadLibraryA(kDllName);
    if (s_hLibrary == nullptr)
    {
        const auto err = GetLastError();
        WLOG("Warning : Wintab32.dll Load Failed. errcode = 0x%x", err);
        return false;
    }

    // 関数ポインタ取得.
    bool fail = false;
    GET_PROC(WTInfoA, fail);
    GET_PROC(WTInfoW, fail);
    GET_PROC(WTOpenA, fail);
    GET_PROC(WTOpenW, fail);
    GET_PROC(WTClose, fail);
    GET_PROC(WTPacketsGet, fail);
    GET_PROC(WTPacket, fail);
    GET_PROC(WTEnable, fail);
    GET_PROC(WTOverlap, fail);
    GET_PROC(WTConfig, fail);
    GET_PROC(WTGetA, fail);
    GET_PROC(WTGetW, fail);
    GET_PROC(WTSetA, fail);
    GET_PROC(WTSetW, fail);
    GET_PROC(WTExtGet, fail);
    GET_PROC(WTExtSet, fail);
    GET_PROC(WTSave, fail);
    GET_PROC(WTRestore, fail);
    GET_PROC(WTPacketsPeek, fail);
    GET_PROC(WTDataGet, fail);
    GET_PROC(WTDataPeek, fail);
    GET_PROC(WTQueueSizeGet, fail);
    GET_PROC(WTQueueSizeSet, fail);
    GET_PROC(WTMgrOpen, fail);
    GET_PROC(WTMgrClose, fail);
    GET_PROC(WTMgrContextEnum, fail);
    GET_PROC(WTMgrContextOwner, fail);
    GET_PROC(WTMgrDefContext, fail);
    GET_PROC(WTMgrDefContextEx, fail);
    GET_PROC(WTMgrDeviceConfig, fail);
    GET_PROC(WTMgrExt, fail);
    GET_PROC(WTMgrCsrEnable, fail);
    GET_PROC(WTMgrCsrButtonMap, fail);
    GET_PROC(WTMgrCsrPressureBtnMarks, fail);
    GET_PROC(WTMgrCsrPressureResponse, fail);
    GET_PROC(WTMgrCsrExt, fail);
    GET_PROC(WTQueuePacketsEx, fail);
    GET_PROC(WTMgrConfigReplaceExA, fail);
    GET_PROC(WTMgrConfigReplaceExW, fail);
    GET_PROC(WTMgrPacketHookExA, fail);
    GET_PROC(WTMgrPacketHookExW, fail);
    GET_PROC(WTMgrPacketUnhook, fail);
    GET_PROC(WTMgrPacketHookNext, fail);
    GET_PROC(WTMgrCsrPressureBtnMarksEx, fail);

    if (fail == true)
    {
        WLOG("Warning : Tablet::InitLibrary() Failed.");
        return false;
    }

    ILOG("Info : Tablet::InitLibrary() Success.");
    return true;
}

//-----------------------------------------------------------------------------
//      ライブラリを終了します.
//-----------------------------------------------------------------------------
void Tablet::TermLibrary()
{
    if (!IsLibraryLoaded())
    { return; }

    // DLL解放.
    FreeLibrary( s_hLibrary );
    s_hLibrary = nullptr;

    // 関数ポインタをクリアする.
    WTInfoA_                      = nullptr;
    WTInfoW_                      = nullptr;
    WTOpenA_                      = nullptr;
    WTOpenW_                      = nullptr;
    WTClose_                      = nullptr;
    WTPacketsGet_                 = nullptr;
    WTPacket_                     = nullptr;
    WTEnable_                     = nullptr;
    WTOverlap_                    = nullptr;
    WTConfig_                     = nullptr;
    WTGetA_                       = nullptr;
    WTGetW_                       = nullptr;
    WTSetA_                       = nullptr;
    WTSetW_                       = nullptr;
    WTExtGet_                     = nullptr;
    WTExtSet_                     = nullptr;
    WTSave_                       = nullptr;
    WTRestore_                    = nullptr;
    WTPacketsPeek_                = nullptr;
    WTDataGet_                    = nullptr;
    WTDataPeek_                   = nullptr;
    WTQueueSizeGet_               = nullptr;
    WTQueueSizeSet_               = nullptr;
    WTMgrOpen_                    = nullptr;
    WTMgrClose_                   = nullptr;
    WTMgrContextEnum_             = nullptr;
    WTMgrContextOwner_            = nullptr;
    WTMgrDefContext_              = nullptr;
    WTMgrDefContextEx_            = nullptr;
    WTMgrDeviceConfig_            = nullptr;
    WTMgrExt_                     = nullptr;
    WTMgrCsrEnable_               = nullptr;
    WTMgrCsrButtonMap_            = nullptr;
    WTMgrCsrPressureBtnMarks_     = nullptr;
    WTMgrCsrPressureResponse_     = nullptr;
    WTMgrCsrExt_                  = nullptr;
    WTQueuePacketsEx_             = nullptr;
    WTMgrConfigReplaceExA_        = nullptr;
    WTMgrConfigReplaceExW_        = nullptr;
    WTMgrPacketHookExA_           = nullptr;
    WTMgrPacketHookExW_           = nullptr;
    WTMgrPacketUnhook_            = nullptr;
    WTMgrPacketHookNext_          = nullptr;
    WTMgrCsrPressureBtnMarksEx_   = nullptr;
}

//-----------------------------------------------------------------------------
//      初期化済みかどうか?
//-----------------------------------------------------------------------------
bool Tablet::IsLibraryLoaded()
{ return s_hLibrary != nullptr; }

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
Tablet::Tablet()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
Tablet::~Tablet()
{
    m_Buttons.clear();
    m_Keys.clear();
}

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool Tablet::Init(HWND hWnd)
{
    m_hWnd = hWnd;

    {
        USHORT version;
        WTInfoA_(WTI_INTERFACE, IFC_SPECVERSION, &version);
        const BYTE major = (version >> 8) & 0xff;
        const BYTE minor = (version & 0xff);
        m_Version = "Version" + std::to_string(major) + "." + std::to_string(minor);
    }

    {
        char deviceName[256] = {};
        WTInfoA_(WTI_DEVICES, DVC_NAME, deviceName);
        m_DeviceName = deviceName;
    }

    WTInfoA_(WTI_INTERFACE, IFC_NDEVICES, &m_DeviceCount);

    WTInfoA_(WTI_DEVICES, DVC_X, &m_CoordX);
    WTInfoA_(WTI_DEVICES, DVC_Y, &m_CoordY);

    {
        AXIS orientation[3];
        m_SupportPressure    = WTInfoA_(WTI_DEVICES, DVC_NPRESSURE,   &m_Pressure) > 0;
        m_SupportWheel       = WTInfoA_(WTI_DEVICES, DVC_TPRESSURE,   &m_Wheel) > 0;
        m_SupportOrientation = WTInfoA_(WTI_DEVICES, DVC_ORIENTATION, &orientation) > 0;
        m_Azimuth   = orientation[0];
        m_Altitude  = orientation[1];
        m_Twist     = orientation[2];
    }

    {
        UINT keyIndex;
        if (m_SupportKeys = FindExtenstion(WTX_EXPKEYS2, keyIndex))
        {
            WTInfoA_(WTI_EXTENSIONS + keyIndex, EXT_MASK, &m_KeyMask);
        }

        for(auto i=1u; i<=kMaxButtonCount; ++i)
        {
            m_Buttons.emplace(i, BUTTON_STATE_NONE);
        }
    }

    LOGCONTEXTA context;
    WTInfoA_(WTI_DEFCONTEXT, 0, &context);
    sprintf_s(context.lcName, kDllName);
    context.lcOptions   |= CXO_MESSAGES;
    context.lcPktData   = PACKETDATA | m_KeyMask;
    context.lcPktMode   = PACKETMODE;
    context.lcMoveMask  = PACKETDATA;
    context.lcBtnUpMask = context.lcBtnDnMask;
    context.lcInOrgX    = m_CoordX.axMin;
    context.lcInOrgY    = m_CoordY.axMin;
    context.lcInExtX    = m_CoordX.axMax;
    context.lcInExtY    = m_CoordY.axMax;

    m_hContext = WTOpenA_(hWnd, &context, TRUE);
    if (m_hContext == nullptr)
    { return false; }

    for(auto tabletId = 0u; tabletId < m_DeviceCount; ++tabletId)
    {
        const auto controlCount = ExtGet<UINT>(WTX_EXPKEYS2, tabletId, 0, 0, TABLET_PROPERTY_CONTROLCOUNT);
        for(auto controlId = 0u; controlId < controlCount; ++controlId)
        {
            const auto funcCount = ExtGet<UINT>(WTX_EXPKEYS2, tabletId, controlId, 0, TABLET_PROPERTY_CONTROLCOUNT);
            for(auto funcId = 0u; funcId < funcCount; ++funcId)
            {
                ExtSet(WTX_EXPKEYS2, tabletId, controlId, funcId, TABLET_PROPERTY_OVERRIDE, static_cast<BOOL>(TRUE));
            }
        }

        auto& keyMap = m_Keys[tabletId];
        for(auto controlId=0u; controlId <controlCount; ++controlId)
        {
            keyMap.emplace(controlId, KEY_STATE_NONE);
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void Tablet::Term()
{
    if (m_hContext != nullptr)
    {
        WTClose_(m_hContext);
        m_hContext = nullptr;
    }

    m_hWnd = nullptr;
}

//-----------------------------------------------------------------------------
//      更新処理を行います.
//-----------------------------------------------------------------------------
void Tablet::Update()
{
    if (m_hContext == nullptr)
    { return; }

    UpdateButtonStates();
    UpdateKeyStates();
}

//-----------------------------------------------------------------------------
//      通常パケットの処理です.
//-----------------------------------------------------------------------------
bool Tablet::OnPacket(LPARAM lp, WPARAM wp)
{
    auto ret = WTPacket_((HCTX)lp, (UINT)wp, &m_Packet);
    if (ret)
    {
        POINT pt0;
        pt0.x = 0;
        pt0.y = 0;
        ClientToScreen(m_hWnd, &pt0);

        POINT pt1;
        pt1.x = m_Packet.pkX;
        pt1.y = m_Packet.pkY;
        
        ScreenToClient(m_hWnd, &pt1);
        {
            m_CursorX = pt1.x - pt0.x;
            m_CursorY = pt1.y - pt0.y;
        }

        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
//      拡張パケットの処理です.
//-----------------------------------------------------------------------------
bool Tablet::OnPacketEx(LPARAM lp, WPARAM wp)
{ return WTPacket_((HCTX)lp, (UINT)wp, &m_PacketEx) != FALSE; }

//-----------------------------------------------------------------------------
//      近接フラグの処理です.
//-----------------------------------------------------------------------------
void Tablet::OnProximity(LPARAM lp, WPARAM wp)
{ m_Proximity = HIWORD(lp) > 0; }

//-----------------------------------------------------------------------------
//      オーバーラップ処理です.
//-----------------------------------------------------------------------------
void Tablet::Overwrap()
{
    if (m_hContext == nullptr)
    { return; }
    WTOverlap_(m_hContext, TRUE);
}

//-----------------------------------------------------------------------------
//      有効化処理です.
//-----------------------------------------------------------------------------
void Tablet::Enable(WPARAM wp)
{
    if (m_hContext == nullptr)
    { return; }
    WTEnable_(m_hContext, (BOOL)wp);
}

//-----------------------------------------------------------------------------
//      圧力対応しているかどうか?
//-----------------------------------------------------------------------------
bool Tablet::SupportPressure() const
{ return m_SupportPressure; }

//-----------------------------------------------------------------------------
//      ホイール対応しているかどうか?
//-----------------------------------------------------------------------------
bool Tablet::SupportWheel() const
{ return m_SupportWheel; }

//-----------------------------------------------------------------------------
//      向き対応しているかどうか?
//-----------------------------------------------------------------------------
bool Tablet::SupportOrientation() const
{ return m_SupportOrientation; }

//-----------------------------------------------------------------------------
//      キー対応しているかどうか?
//-----------------------------------------------------------------------------
bool Tablet::SupportKeys() const
{ return m_SupportKeys; }

//-----------------------------------------------------------------------------
//      デバイス名を取得します.
//-----------------------------------------------------------------------------
const std::string& Tablet::GetDeviceName() const
{ return m_DeviceName; }

//-----------------------------------------------------------------------------
//      バージョン名を取得します.
//-----------------------------------------------------------------------------
const std::string& Tablet::GetVersion() const
{ return m_Version; }

//-----------------------------------------------------------------------------
//      デバイス数を取得します.
//-----------------------------------------------------------------------------
uint32_t Tablet::GetDeviceCount() const
{ return m_DeviceCount; }

//-----------------------------------------------------------------------------
//      キー数を取得します.
//-----------------------------------------------------------------------------
uint32_t Tablet::GetKeyCount(uint32_t tabletId) const
{
    if (tabletId >= 0 && tabletId < m_Keys.size())
    { return uint32_t(m_Keys.at(tabletId).size()); }

    return 0;
}

//-----------------------------------------------------------------------------
//      カーソルX座標を取得します.
//-----------------------------------------------------------------------------
int Tablet::GetCursorX() const
{ return m_CursorX; }

//-----------------------------------------------------------------------------
//      カーソルY座標を取得します.
//-----------------------------------------------------------------------------
int Tablet::GetCursorY() const
{ return m_CursorY; }

//-----------------------------------------------------------------------------
//      正規化されたカーソルX座標を取得します.
//-----------------------------------------------------------------------------
float Tablet::GetX() const
{ return float(m_Packet.pkX) / (m_CoordX.axMax - m_CoordX.axMin); }

//-----------------------------------------------------------------------------
//      正規化されたカーソルY座標を習得します.
//-----------------------------------------------------------------------------
float Tablet::GetY() const
{ return float(m_Packet.pkY) / (m_CoordY.axMax - m_CoordY.axMin); }

//-----------------------------------------------------------------------------
//      正規化された圧力を取得します.
//-----------------------------------------------------------------------------
float Tablet::GetPressure() const
{ return float(m_Packet.pkNormalPressure) / (m_Pressure.axMax - m_Pressure.axMin); }

//-----------------------------------------------------------------------------
//      正規化されたホイール量を取得します.
//-----------------------------------------------------------------------------
float Tablet::GetWheel() const
{ return float(m_Packet.pkZ) / (m_Wheel.axMax - m_Wheel.axMin); }

//-----------------------------------------------------------------------------
//      正規化されたペンの方位を取得します.
//-----------------------------------------------------------------------------
float Tablet::GetAzimuth() const
{ return float(m_Packet.pkOrientation.orAzimuth) / (m_Azimuth.axMax - m_Azimuth.axMin); }

//-----------------------------------------------------------------------------
//      正規化されたペンの高度を取得します.
//-----------------------------------------------------------------------------
float Tablet::GetAltitude() const
{ return float(m_Packet.pkOrientation.orAltitude) / (m_Altitude.axMax - m_Altitude.axMin); }

//-----------------------------------------------------------------------------
//      正規化されたペンのねじれを取得します.
//-----------------------------------------------------------------------------
float Tablet::GetTwist() const
{ return float(m_Packet.pkOrientation.orTwist) / (m_Twist.axMax - m_Twist.axMin); }

//-----------------------------------------------------------------------------
//      ペンIDを取得します.
//-----------------------------------------------------------------------------
uint32_t Tablet::GetPenId() const
{ return m_Packet.pkCursor / CURSOR_TYPE_COUNT; }

//-----------------------------------------------------------------------------
//      カーソルタイプを取得します.
//-----------------------------------------------------------------------------
Tablet::CURSOR_TYPE Tablet::GetCursorType() const
{ return CURSOR_TYPE(m_Packet.pkCursor % CURSOR_TYPE_COUNT); }

//-----------------------------------------------------------------------------
//      時間を取得します.
//-----------------------------------------------------------------------------
uint32_t Tablet::GetTime() const
{ return m_Packet.pkTime; }

//-----------------------------------------------------------------------------
//      近接フラグを取得します.
//-----------------------------------------------------------------------------
bool Tablet::GetProximity() const
{ return m_Proximity; }

//-----------------------------------------------------------------------------
//      ボタンが押されたかどうか?
//-----------------------------------------------------------------------------
bool Tablet::IsButtonDown(uint16_t id) const
{
    if (!IsValidButton(id))
    { return false; }

    const auto& state = m_Buttons.at(id);
    return state == BUTTON_STATE_DOWN;
}

//-----------------------------------------------------------------------------
//      ボタンが押されっぱなしかどうか?
//-----------------------------------------------------------------------------
bool Tablet::IsButtonHold(uint16_t id) const
{
    if (!IsValidButton(id))
    { return false; }

    const auto& state = m_Buttons.at(id);
    return state == BUTTON_STATE_HOLD;
}

//-----------------------------------------------------------------------------
//      ボタンが離されたかどうか?
//-----------------------------------------------------------------------------
bool Tablet::IsButtonUp(uint16_t id) const
{
    if (!IsValidButton(id))
    { return false; }

    const auto& state = m_Buttons.at(id);
    return state == BUTTON_STATE_UP;
}

//-----------------------------------------------------------------------------
//      ボタンが押されたかどうか?
//-----------------------------------------------------------------------------
bool Tablet::IsKeyDown(uint16_t tabletId, uint16_t controlId) const
{
    if (!IsValidKey(tabletId, controlId))
    { return false; }

    const auto& state = m_Keys.at(tabletId).at(controlId);
    return state == KEY_STATE_DOWN;
}

//-----------------------------------------------------------------------------
//      ボタンが押されっぱなしかどうか?
//-----------------------------------------------------------------------------
bool Tablet::IsKeyHold(uint16_t tabletId, uint16_t controlId) const
{
    if (!IsValidKey(tabletId, controlId))
    { return false; }

    const auto& state = m_Keys.at(tabletId).at(controlId);
    return state == KEY_STATE_HOLD;
}

//-----------------------------------------------------------------------------
//      ボタンが離されたかどうか?
//-----------------------------------------------------------------------------
bool Tablet::IsKeyUp(uint16_t tabletId, uint16_t controlId) const
{
    if (!IsValidKey(tabletId, controlId))
    { return false; }

    const auto& state = m_Keys.at(tabletId).at(controlId);
    return state == KEY_STATE_UP;
}

//-----------------------------------------------------------------------------
//      拡張を検索します.
//-----------------------------------------------------------------------------
bool Tablet::FindExtenstion(uint32_t extension, uint32_t& index)
{
    for(UINT i=0u, tag = 0u; WTInfoA_(WTI_EXTENSIONS + i, EXT_TAG, &tag); ++i)
    {
        if (tag == extension)
        {
            index = i;
            return true;
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
//      拡張データを取得します.
//-----------------------------------------------------------------------------
template<typename T>
T Tablet::ExtGet
(
    uint32_t    extension,
    uint8_t     tabletId,
    uint8_t     controlId,
    uint8_t     functionId,
    uint16_t    property
)
{
    EXTPROPERTY prop = {};
    prop.version        = 0;
    prop.tabletIndex    = tabletId;
    prop.controlIndex   = controlId;
    prop.functionIndex  = functionId;
    prop.propertyID     = property;
    prop.reserved       = 0;
    prop.dataSize       = sizeof(T);

    if (WTExtGet_(m_hContext, extension, &prop))
    {
        return *reinterpret_cast<T*>(&prop.data[0]);
    }

    return T();
}

//-----------------------------------------------------------------------------
//      拡張データを設定します.
//-----------------------------------------------------------------------------
template<typename T>
bool Tablet::ExtSet
(
    uint32_t extension,
    uint8_t  tabletId,
    uint8_t  controlId,
    uint8_t  functionId,
    uint16_t property,
    T        value
)
{
    EXTPROPERTY prop = {};
    prop.version        = 0;
    prop.tabletIndex    = tabletId;
    prop.controlIndex   = controlId;
    prop.functionIndex  = functionId;
    prop.propertyID     = property;
    prop.reserved       = 0;
    prop.dataSize       = sizeof(T);
    *reinterpret_cast<T*>(&prop.data[0]) = value;
    return WTExtSet_(m_hContext, extension, &prop) >= S_OK;
}

//-----------------------------------------------------------------------------
//      有効なボタンかどうか?
//-----------------------------------------------------------------------------
bool Tablet::IsValidButton(uint16_t id) const
{ return (id >= 1) && (id <= kMaxButtonCount); }

//-----------------------------------------------------------------------------
//      有効なキーかどうか?
//-----------------------------------------------------------------------------
bool Tablet::IsValidKey(uint16_t tabletId, uint16_t controlId) const
{ 
    return (tabletId >= 0 && tabletId < m_DeviceCount)
        && (controlId >= 0 && controlId < m_Keys.at(tabletId).size()); 
}

//-----------------------------------------------------------------------------
//      ボタン状態を更新します.
//-----------------------------------------------------------------------------
void Tablet::UpdateButtonStates()
{
    for(auto&& itr : m_Buttons)
    {
        const auto id = itr.first;
        auto& state = itr.second;
        switch(state)
        {
        case BUTTON_STATE_DOWN:
            state = BUTTON_STATE_HOLD;
            break;

        case BUTTON_STATE_UP:
            state = BUTTON_STATE_NONE;
            break;
        }
    }

    const auto id = LOWORD(m_Packet.pkButtons);
    if (!IsValidButton(id))
    { return; }

    auto& state = m_Buttons.at(id);
    switch(HIWORD(m_Packet.pkButtons))
    {
    case 1:
        { state = BUTTON_STATE_UP; }
        break;

    case 2:
        { state = BUTTON_STATE_DOWN; }
        break;
    }
}

//-----------------------------------------------------------------------------
//      キー状態を更新します.
//-----------------------------------------------------------------------------
void Tablet::UpdateKeyStates()
{
    const auto tabletId = m_PacketEx.pkExpKeys.nTablet;
    const auto controlId = m_PacketEx.pkExpKeys.nControl;
    if (!IsValidKey(tabletId, controlId))
    { return; }

    auto& state = m_Keys.at(tabletId).at(controlId);
    switch(m_PacketEx.pkExpKeys.nState)
    {
    case 0:
        {
            if (state == KEY_STATE_UP)
            { state = KEY_STATE_NONE;}
            else if (state != KEY_STATE_NONE)
            { state = KEY_STATE_UP; }
        }
        break;

    case 1:
        {
            if (state == KEY_STATE_DOWN)
            { state = KEY_STATE_HOLD; }
            else if (state != KEY_STATE_HOLD)
            { state = KEY_STATE_DOWN; }
        }
        break;
    }
}

} // namespace asdx

#undef GET_PROC

#endif//ASDX_ENABLE_PENTAB