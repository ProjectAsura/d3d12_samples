//-----------------------------------------------------------------------------
// File : asdxEditParam.cpp
// Desc : Edit Parameter.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <edit/asdxEditParam.h>
#include <edit/asdxHistory.h>
#include <fnd/asdxMisc.h>
#include <res/asdxResTexture.h>


#ifdef ASDX_ENABLE_IMGUI
#include <imgui.h>
#endif//ASDX_ENABLE_IMGUI


#ifndef ASDX_UNUSED
#define ASDX_UNUSED(x) ((void)x)
#endif//ASDX_UNUSED


namespace {

///////////////////////////////////////////////////////////////////////////////
// ParamHistory
///////////////////////////////////////////////////////////////////////////////
template<typename T>
class ParamHistory : public asdx::IHistory
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
    ParamHistory(T* target, const T& value)
    : m_pTarget (target)
    , m_Curr    (value)
    , m_Prev    (*target)
    { /* DO_NOTHING */ }

    ParamHistory(T* target, const T& nextValue, const T& prevValue)
    : m_pTarget (target)
    , m_Curr    (nextValue)
    , m_Prev    (prevValue)
    { /* DO_NOTHING */ }

    //-------------------------------------------------------------------------
    //! @brief      やり直します.
    //-------------------------------------------------------------------------
    void Redo() override
    { *m_pTarget = m_Curr; }

    //-------------------------------------------------------------------------
    //! @brief      元に戻します.
    //-------------------------------------------------------------------------
    void Undo() override
    { *m_pTarget = m_Prev; }

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    T*      m_pTarget;  //!< 変更対象.
    T       m_Prev;     //!< 変更前の値.
    T       m_Curr;     //!< 変更後の値.

    //=========================================================================
    // private methods.
    //=========================================================================
    /* NOTHING */
};

///////////////////////////////////////////////////////////////////////////////
// TextureHistory class
///////////////////////////////////////////////////////////////////////////////
class TextureHistory : public asdx::IHistory
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
    //      コンストラクタです.
    //-------------------------------------------------------------------------
    TextureHistory
    (
        asdx::Texture*      pTexture,
        const std::string&  nextValue,
        const std::string&  prevValue
    )
    : m_pTexture    (pTexture)
    , m_NextPath    (nextValue)
    , m_PrevPath    (prevValue)
    { /* DO_NOTHING */ }

    //-------------------------------------------------------------------------
    //      やり直しを実行します.
    //-------------------------------------------------------------------------
    void Redo() override
    {
        if (m_pTexture == nullptr)
        { return; }

        if (m_NextPath == "" || m_NextPath.empty())
        {
            m_pTexture->Term();
        }
        else
        {
            asdx::ResTexture res;
            if (!res.LoadFromFileA(m_NextPath.c_str()))
            { return; }

            m_pTexture->Term();

            //if (!m_pTexture->Init(res))
            //{ return; }
        }
    }

    //-------------------------------------------------------------------------
    //      元に戻すを実行します.
    //-------------------------------------------------------------------------
    void Undo() override
    {
        if (m_pTexture == nullptr)
        { return; }

        if (m_PrevPath == "" || m_PrevPath.empty())
        {
            m_pTexture->Term();
        }
        else
        {
            asdx::ResTexture res;
            if (!res.LoadFromFileA(m_PrevPath.c_str()))
            { return; }

            m_pTexture->Term();

            //if (!m_pTexture->Init(res))
            //{ return; }
        }
    }

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    asdx::Texture*  m_pTexture  = nullptr;
    std::string     m_NextPath;
    std::string     m_PrevPath;

    //=========================================================================
    // private methods.
    //=========================================================================
    /* NOTHING */
};

} // namespace 


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// EditBool class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
EditBool::EditBool(bool value)
: m_Value(value)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      値を設定します.
//-----------------------------------------------------------------------------
void EditBool::SetValue(bool value, bool history)
{
    if (!history)
    {
        m_Value = value;
        return;
    }

    if (value == m_Value)
    { return; }

    HistoryMgr::Instance().Add(CreateHistory(value));
}

//-----------------------------------------------------------------------------
//      グループヒストリー用のヒストリーを作成します.
//-----------------------------------------------------------------------------
IHistory* EditBool::CreateHistory(bool value)
{ return new ParamHistory<bool>(&m_Value, value); }

//-----------------------------------------------------------------------------
//      値を取得します.
//-----------------------------------------------------------------------------
bool EditBool::GetValue() const
{ return m_Value; }

//-----------------------------------------------------------------------------
//      チェックボックスを描画します.
//-----------------------------------------------------------------------------
void EditBool::DrawCheckbox(const char* tag)
{
#ifdef ASDX_ENABLE_IMGUI
    auto prev = m_Value;
    if (ImGui::Checkbox(tag, &m_Value))
    { HistoryMgr::Instance().Add(new ParamHistory<bool>(&m_Value, m_Value, prev), false); }
#else
    ASDX_UNUSED(tag);
#endif
}


///////////////////////////////////////////////////////////////////////////////
// EditInt class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
EditInt::EditInt(int value)
: m_Value   (value)
, m_Prev    (value)
, m_Dragged (false)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      値を設定します.
//-----------------------------------------------------------------------------
void EditInt::SetValue(int value, bool history)
{
    if (!history)
    {
        m_Value = value;
        return;
    }

    if (m_Value == value)
    { return; }

    HistoryMgr::Instance().Add(CreateHistory(value));
}

//-----------------------------------------------------------------------------
//      値を取得します.
//-----------------------------------------------------------------------------
int EditInt::GetValue() const
{ return m_Value; }

//-----------------------------------------------------------------------------
//      グループヒストリー用のヒストリーを作成します.
//-----------------------------------------------------------------------------
IHistory* EditInt::CreateHistory(int value)
{ return new ParamHistory<int>(&m_Value, m_Value, value); }

//-----------------------------------------------------------------------------
//      スライダーを描画します.
//-----------------------------------------------------------------------------
void EditInt::DrawSlider(const char* tag, int step, int mini, int maxi)
{
#ifdef ASDX_ENABLE_IMGUI
    auto prev = m_Value;
    auto flag = ImGui::DragInt(tag, &m_Value, float(step), mini, maxi);

    if (!ImGui::IsMouseDragging(0) && !ImGui::IsMouseDown(0))
    {
        if (!m_Dragged && !ImGui::IsItemActive())
        {
            // 一度もドラッグされておらず，アイテムもいじられていない場合.
            m_Prev = m_Value;
        }
        else if (m_Dragged)
        {
            // マウスドラッグ終了時.
            HistoryMgr::Instance().Add(new ParamHistory<int>(&m_Value, m_Value, m_Prev), false);
            m_Dragged = false;
        }
        else if (flag)
        {
            // キーボード入力.
            HistoryMgr::Instance().Add(new ParamHistory<int>(&m_Value, m_Value, m_Prev), false);
        }
    }
    else if (ImGui::IsItemActive())
    {
        m_Dragged = ImGui::IsMouseDragging(0);
    }
#else
    ASDX_UNUSED(tag);
    ASDX_UNUSED(step);
    ASDX_UNUSED(mini);
    ASDX_UNUSED(maxi);
#endif
}

//-----------------------------------------------------------------------------
//      コンボボックスを描画します.
//-----------------------------------------------------------------------------
void EditInt::DrawCombo(const char* tag, int count, const char** items)
{
#ifdef ASDX_ENABLE_IMGUI
    auto value = m_Value;
    if (ImGui::Combo(tag, &value, items, count))
    {
        HistoryMgr::Instance().Add(new ParamHistory<int>(&m_Value, value, m_Prev));
    }
#else
    ASDX_UNUSED(tag);
    ASDX_UNUSED(count);
    ASDX_UNUSED(items);
#endif
}

//-----------------------------------------------------------------------------
//      コンボボックスを描画します.
//-----------------------------------------------------------------------------
void EditInt::DrawCombo(const char* tag, bool (*items_getter)(void* data, int idx, const char** out_text), int count)
{
#ifdef ASDX_ENABLE_IMGUI
    auto value = m_Value;
    if (ImGui::Combo(tag, &value, items_getter, &value, count))
    {
        HistoryMgr::Instance().Add(new ParamHistory<int>(&m_Value, value, m_Prev));
    }
#else
    ASDX_UNUSED(tag);
    ASDX_UNUSED(items_getter);
#endif
}


///////////////////////////////////////////////////////////////////////////////
// EditFloat class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
EditFloat::EditFloat(float value)
: m_Value  (value)
, m_Prev   (0.0f)
, m_Dragged(false)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      値を設定します.
//-----------------------------------------------------------------------------
void EditFloat::SetValue(float value, bool history)
{
    if (!history)
    {
        m_Value = value;
        return;
    }

    if (m_Value == value)
    { return; }

    HistoryMgr::Instance().Add(CreateHistory(value));
}

//-----------------------------------------------------------------------------
//      値を取得します.
//-----------------------------------------------------------------------------
float EditFloat::GetValue() const
{ return m_Value; }

//-----------------------------------------------------------------------------
//      グループヒストリー用のヒストリーを作成します.
//-----------------------------------------------------------------------------
IHistory* EditFloat::CreateHistory(float value)
{ return new ParamHistory<float>(&m_Value, value); }

//-----------------------------------------------------------------------------
//      スライダーを描画します.
//-----------------------------------------------------------------------------
void EditFloat::DrawSlider(const char* tag, float step, float mini, float maxi)
{
#ifdef ASDX_ENABLE_IMGUI
    auto prev = m_Value;
    auto flag = ImGui::DragFloat(tag, &m_Value, step, mini, maxi, "%.5f");

    if (!ImGui::IsMouseDragging(0) && !ImGui::IsMouseDown(0))
    {
        if (!m_Dragged && !ImGui::IsItemActive())
        {
            // 一度もドラッグされておらず，アイテムもいじられていない場合.
            m_Prev = m_Value;
        }
        else if (m_Dragged)
        {
            // マウスドラッグ終了時.
            HistoryMgr::Instance().Add(new ParamHistory<float>(&m_Value, m_Value, m_Prev), false);
            m_Dragged = false;
        }
        else if (flag)
        {
            // キーボード入力.
            HistoryMgr::Instance().Add(new ParamHistory<float>(&m_Value, m_Value, m_Prev), false);
        }
    }
    else if (ImGui::IsItemActive())
    {
        m_Dragged = ImGui::IsMouseDragging(0);
    }
#else
    ASDX_UNUSED(tag);
    ASDX_UNUSED(step);
    ASDX_UNUSED(mini);
    ASDX_UNUSED(maxi);
#endif
}


///////////////////////////////////////////////////////////////////////////////
// EditFloat2 class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
EditFloat2::EditFloat2(float x, float y)
: m_Value  (x, y)
, m_Prev   (x, y)
, m_Dragged(false)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      値を設定します.
//-----------------------------------------------------------------------------
void EditFloat2::SetValue(const Vector2& value, bool history)
{
    if (!history)
    {
        m_Value = value;
        return;
    }

    if (m_Value == value)
    { return; }

    HistoryMgr::Instance().Add(CreateHistory(value));
}

//-----------------------------------------------------------------------------
//      値を取得します.
//-----------------------------------------------------------------------------
const Vector2& EditFloat2::GetValue() const
{ return m_Value; }

//-----------------------------------------------------------------------------
//      グループヒストリー用のヒストリーを作成します.
//-----------------------------------------------------------------------------
IHistory* EditFloat2::CreateHistory(const Vector2& value)
{ return new ParamHistory<Vector2>(&m_Value, value); }

//-----------------------------------------------------------------------------
//      スライダーを描画します.
//-----------------------------------------------------------------------------
void EditFloat2::DrawSlider(const char* tag, float step, float mini, float maxi)
{
#ifdef ASDX_ENABLE_IMGUI
    auto prev = m_Value;
    auto flag = ImGui::DragFloat2(tag, m_Value, step, mini, maxi);

    if (!ImGui::IsMouseDragging(0) && !ImGui::IsMouseDown(0))
    {
        if (!m_Dragged && !ImGui::IsItemActive())
        {
            // 一度もドラッグされておらず，アイテムもいじられていない場合.
            m_Prev = m_Value;
        }
        else if (m_Dragged)
        {
            // マウスドラッグ終了時.
            HistoryMgr::Instance().Add(new ParamHistory<Vector2>(&m_Value, m_Value, m_Prev), false);
            m_Dragged = false;
        }
        else if (flag)
        {
            // キーボード入力.
            HistoryMgr::Instance().Add(new ParamHistory<Vector2>(&m_Value, m_Value, m_Prev), false);
        }
    }
    else if (ImGui::IsItemActive())
    {
        m_Dragged = ImGui::IsMouseDragging(0);
    }
#else
    ASDX_UNUSED(tag);
    ASDX_UNUSED(step);
    ASDX_UNUSED(mini);
    ASDX_UNUSED(maxi);
#endif
}


///////////////////////////////////////////////////////////////////////////////
// EditFloat3 class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
EditFloat3::EditFloat3(float x, float y, float z)
: m_Value   (x, y, z)
, m_Prev    (x, y, z)
, m_Dragged (false)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      値を設定します.
//-----------------------------------------------------------------------------
void EditFloat3::SetValue(const Vector3& value, bool history)
{
    if (!history)
    {
        m_Value = value;
        return;
    }

    if (m_Value == value)
    { return; }

    HistoryMgr::Instance().Add(CreateHistory(value));
}

//-----------------------------------------------------------------------------
//      値を取得します.
//-----------------------------------------------------------------------------
const Vector3& EditFloat3::GetValue() const
{ return m_Value; }

//-----------------------------------------------------------------------------
//      グループヒストリー用のヒストリーを作成します.
//-----------------------------------------------------------------------------
IHistory* EditFloat3::CreateHistory(const Vector3& value)
{ return new ParamHistory<Vector3>(&m_Value, value); }

//-----------------------------------------------------------------------------
//      スライダーを描画します.
//-----------------------------------------------------------------------------
void EditFloat3::DrawSlider(const char* tag, float step, float mini, float maxi)
{
#ifdef ASDX_ENABLE_IMGUI
    auto prev = m_Value;
    auto flag = ImGui::DragFloat3(tag, m_Value, step, mini, maxi);

    if (!ImGui::IsMouseDragging(0) && !ImGui::IsMouseDown(0))
    {
        if (!m_Dragged && !ImGui::IsItemActive())
        {
            // 一度もドラッグされておらず，アイテムもいじられていない場合.
            m_Prev = m_Value;
        }
        else if (m_Dragged)
        {
            // マウスドラッグ終了時.
            HistoryMgr::Instance().Add(new ParamHistory<Vector3>(&m_Value, m_Value, m_Prev), false);
            m_Dragged = false;
        }
        else if (flag)
        {
            // キーボード入力.
            HistoryMgr::Instance().Add(new ParamHistory<Vector3>(&m_Value, m_Value, m_Prev), false);
        }
    }
    else if (ImGui::IsItemActive())
    {
        m_Dragged = ImGui::IsMouseDragging(0);
    }
#else
    ASDX_UNUSED(tag);
    ASDX_UNUSED(step);
    ASDX_UNUSED(mini);
    ASDX_UNUSED(maxi);
#endif
}


///////////////////////////////////////////////////////////////////////////////
// EditFloat4 class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
EditFloat4::EditFloat4(float x, float y, float z, float w)
: m_Value   (x, y, z, w)
, m_Prev    (x, y, z, w)
, m_Dragged (false)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      値を設定します.
//-----------------------------------------------------------------------------
void EditFloat4::SetValue(const Vector4& value, bool history)
{
    if (!history)
    {
        m_Value = value;
        return;
    }

    if (m_Value == value)
    { return; }

    HistoryMgr::Instance().Add(CreateHistory(value));
}

//-----------------------------------------------------------------------------
//      値を取得します.
//-----------------------------------------------------------------------------
const Vector4& EditFloat4::GetValue() const
{ return m_Value; }

//-----------------------------------------------------------------------------
//      グループヒストリー用のヒストリーを作成します.
//-----------------------------------------------------------------------------
IHistory* EditFloat4::CreateHistory(const Vector4& value)
{ return new ParamHistory<Vector4>(&m_Value, value); }

//-----------------------------------------------------------------------------
//      スライダーを描画します.
//-----------------------------------------------------------------------------
void EditFloat4::DrawSlider(const char* tag, float step, float mini, float maxi)
{
#ifdef ASDX_ENABLE_IMGUI
    auto prev = m_Value;
    auto flag = ImGui::DragFloat4(tag, m_Value, step, mini, maxi);

    if (!ImGui::IsMouseDragging(0) && !ImGui::IsMouseDown(0))
    {
        if (!m_Dragged && !ImGui::IsItemActive())
        {
            // 一度もドラッグされておらず，アイテムもいじられていない場合.
            m_Prev = m_Value;
        }
        else if (m_Dragged)
        {
            // マウスドラッグ終了時.
            HistoryMgr::Instance().Add(new ParamHistory<Vector4>(&m_Value, m_Value, m_Prev), false);
            m_Dragged = false;
        }
        else if (flag)
        {
            // キーボード入力.
            HistoryMgr::Instance().Add(new ParamHistory<Vector4>(&m_Value, m_Value, m_Prev), false);
        }
    }
    else if (ImGui::IsItemActive())
    {
        m_Dragged = ImGui::IsMouseDragging(0);
    }
#else
    ASDX_UNUSED(tag);
    ASDX_UNUSED(step);
    ASDX_UNUSED(mini);
    ASDX_UNUSED(maxi);
#endif
}


///////////////////////////////////////////////////////////////////////////////
// EditColor3 class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
EditColor3::EditColor3(float r, float g, float b)
: m_Value   (r, g, b)
, m_Prev    (r, g, b)
, m_Dragged (false)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      値を設定します.
//-----------------------------------------------------------------------------
void EditColor3::SetValue(const Vector3& value, bool history)
{
    if (!history)
    {
        m_Value = value;
        return;
    }

    if (m_Value == value)
    { return; }

    HistoryMgr::Instance().Add(CreateHistory(value));
}

//-----------------------------------------------------------------------------
//      値を取得します.
//-----------------------------------------------------------------------------
const Vector3& EditColor3::GetValue() const
{ return m_Value; }

//-----------------------------------------------------------------------------
//      グループヒストリー用のヒストリーを作成します.
//-----------------------------------------------------------------------------
IHistory* EditColor3::CreateHistory(const Vector3& value)
{ return new ParamHistory<Vector3>(&m_Value, value); }

//-----------------------------------------------------------------------------
//      スライダーを描画します.
//-----------------------------------------------------------------------------
void EditColor3::DrawPicker(const char* tag)
{
#ifdef ASDX_ENABLE_IMGUI
    auto prev = m_Value;
    auto flag = ImGui::ColorEdit3(tag, m_Value, ImGuiColorEditFlags_Float);

    if (!ImGui::IsMouseDragging(0) && !ImGui::IsMouseDown(0))
    {
        if (!m_Dragged && !ImGui::IsItemActive())
        {
            // 一度もドラッグされておらず，アイテムもいじられていない場合.
            m_Prev = m_Value;
        }
        else if (m_Dragged)
        {
            // マウスドラッグ終了時.
            HistoryMgr::Instance().Add(new ParamHistory<Vector3>(&m_Value, m_Value, m_Prev), false);
            m_Dragged = false;
        }
        else if (flag)
        {
            // キーボード入力.
            HistoryMgr::Instance().Add(new ParamHistory<Vector3>(&m_Value, m_Value, m_Prev), false);
        }
    }
    else if (ImGui::IsItemActive())
    {
        m_Dragged = ImGui::IsMouseDragging(0);
    }
#else
    ASDX_UNUSED(tag);
#endif
}


///////////////////////////////////////////////////////////////////////////////
// EditColor4 class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
EditColor4::EditColor4(float r, float g, float b, float a)
: m_Value   (r, g, b, a)
, m_Prev    (r, g, b, a)
, m_Dragged (false)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      値を設定します.
//-----------------------------------------------------------------------------
void EditColor4::SetValue(const Vector4& value, bool history)
{
    if (!history)
    {
        m_Value = value;
        return;
    }

    if (m_Value == value)
    { return; }

    HistoryMgr::Instance().Add(CreateHistory(value));
}

//-----------------------------------------------------------------------------
//      値を取得します.
//-----------------------------------------------------------------------------
const Vector4& EditColor4::GetValue() const
{ return m_Value; }

//-----------------------------------------------------------------------------
//      グループヒストリー用のヒストリーを作成します.
//-----------------------------------------------------------------------------
IHistory* EditColor4::CreateHistory(const Vector4& value)
{ return new ParamHistory<Vector4>(&m_Value, value); }

//-----------------------------------------------------------------------------
//      スライダーを描画します.
//-----------------------------------------------------------------------------
void EditColor4::DrawPicker(const char* tag)
{
#ifdef ASDX_ENABLE_IMGUI
    auto prev = m_Value;
    auto flag = ImGui::ColorEdit4(tag, m_Value, ImGuiColorEditFlags_Float);

    if (!ImGui::IsMouseDragging(0) && !ImGui::IsMouseDown(0))
    {
        if (!m_Dragged && !ImGui::IsItemActive())
        {
            // 一度もドラッグされておらず，アイテムもいじられていない場合.
            m_Prev = m_Value;
        }
        else if (m_Dragged)
        {
            // マウスドラッグ終了時.
            HistoryMgr::Instance().Add(new ParamHistory<Vector4>(&m_Value, m_Value, m_Prev), false);
            m_Dragged = false;
        }
        else if (flag)
        {
            // キーボード入力.
            HistoryMgr::Instance().Add(new ParamHistory<Vector4>(&m_Value, m_Value, m_Prev), false);
        }
    }
    else if (ImGui::IsItemActive())
    {
        m_Dragged = ImGui::IsMouseDragging(0);
    }
#else
    ASDX_UNUSED(tag);
#endif
}

#if 0
///////////////////////////////////////////////////////////////////////////////
// EditTexture class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
EditTexture::EditTexture(const std::string& value)
: m_Path(value)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
EditTexture::~EditTexture()
{ Term(); }

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void EditTexture::Term()
{ m_Texture.Term(); }

//-----------------------------------------------------------------------------
//      パスを設定します.
//-----------------------------------------------------------------------------
void EditTexture::SetPath(const std::string& value, bool history)
{
    if (!history)
    {
        m_Path = value;
        return;
    }

    if (m_Path == value)
    { return; }

    HistoryMgr::Instance().Add(CreateHistory(value));
}

//-----------------------------------------------------------------------------
//      パスを取得します.
//-----------------------------------------------------------------------------
const std::string& EditTexture::GetPath() const
{ return m_Path; }

//-----------------------------------------------------------------------------
//      グループヒストリー用のヒストリーを作成します.
//-----------------------------------------------------------------------------
IHistory* EditTexture::CreateHistory(const std::string& next)
{ return new TextureHistory(&m_Texture, next, m_Path); }

//-----------------------------------------------------------------------------
//      コントールを描画します.
//-----------------------------------------------------------------------------
void EditTexture::DrawControl
(
    const char* label,
    const char* defaultPath,
    uint32_t    width,
    uint32_t    height
)
{
#if ASDX_ENABLE_IMGUI
    ImGui::PushID(label);
    {
        auto view = m_Texture.GetView();
        if (view != nullptr)
        {
            ImTextureID texture = (void*)view;
            ImGui::Image(texture, ImVec2(float(width), float(height)));

            if (ImGui::IsItemHovered())
            { ImGui::SetTooltip("%s", m_Path.c_str()); }
        }
        else
        { ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), u8"NO TEXTURE"); }

        if (ImGui::Button(u8"Setting"))
        {
            std::string path;
            if (OpenFileDlg(
                "Texture(*.tga, *.dds)\0*.tga;*.dds\0\0",
                path, defaultPath))
            { SetPath(path.c_str(), true); }
        }

        if (view != nullptr)
        {
            ImGui::SameLine();
            if (ImGui::Button(u8"Delete"))
            { SetPath("", true); }
        }
    }
    ImGui::PopID();
#else
    ASDX_UNUSED(label);
    ASDX_UNUSED(defaultPath);
    ASDX_UNUSED(width);
    ASDX_UNUSED(height);
#endif
}

//-----------------------------------------------------------------------------
//      ビューを取得します.
//-----------------------------------------------------------------------------
IShaderResourceView* EditTexture::GetView() const
{ return m_Texture.GetView(); }
#endif

} // namespace asdx


#ifdef ASDX_ENABLE_TINYXML2
namespace asdx {

//-----------------------------------------------------------------------------
//      XMLエレメントを生成します.
//-----------------------------------------------------------------------------
tinyxml2::XMLElement* Serialize(tinyxml2::XMLDocument* doc, const char* tag, const EditBool& control)
{
    auto value = control.GetValue();
    auto element = doc->NewElement(tag);
    element->SetAttribute("value", value);
    return element;
}

//-----------------------------------------------------------------------------
//      XMLエレメントを生成します.
//-----------------------------------------------------------------------------
tinyxml2::XMLElement* Serialize(tinyxml2::XMLDocument* doc, const char* tag, const EditInt& control)
{
    auto value = control.GetValue();
    auto element = doc->NewElement(tag);
    element->SetAttribute("value", value);
    return element;
}

//-----------------------------------------------------------------------------
//      XMLエレメントを生成します.
//-----------------------------------------------------------------------------
tinyxml2::XMLElement* Serialize(tinyxml2::XMLDocument* doc, const char* tag, const EditFloat& control)
{
    auto value = control.GetValue();
    auto element = doc->NewElement(tag);
    element->SetAttribute("value", value);
    return element;
}

//-----------------------------------------------------------------------------
//      XMLエレメントを生成します.
//-----------------------------------------------------------------------------
tinyxml2::XMLElement* Serialize(tinyxml2::XMLDocument* doc, const char* tag, const EditFloat2& control)
{
    auto value = control.GetValue();
    auto element = doc->NewElement(tag);
    element->SetAttribute("x", value.x);
    element->SetAttribute("y", value.y);
    return element;
}

//-----------------------------------------------------------------------------
//      XMLエレメントを生成します.
//-----------------------------------------------------------------------------
tinyxml2::XMLElement* Serialize(tinyxml2::XMLDocument* doc, const char* tag, const EditFloat3& control)
{
    auto value = control.GetValue();
    auto element = doc->NewElement(tag);
    element->SetAttribute("x", value.x);
    element->SetAttribute("y", value.y);
    element->SetAttribute("z", value.z);
    return element;
}

//-----------------------------------------------------------------------------
//      XMLエレメントを生成します.
//-----------------------------------------------------------------------------
tinyxml2::XMLElement* Serialize(tinyxml2::XMLDocument* doc, const char* tag, const EditFloat4& control)
{
    auto value = control.GetValue();
    auto element = doc->NewElement(tag);
    element->SetAttribute("x", value.x);
    element->SetAttribute("y", value.y);
    element->SetAttribute("z", value.z);
    element->SetAttribute("w", value.w);
    return element;
}

//-----------------------------------------------------------------------------
//      XMLエレメントを生成します.
//-----------------------------------------------------------------------------
tinyxml2::XMLElement* Serialize(tinyxml2::XMLDocument* doc, const char* tag, const EditColor3& control)
{
    auto value = control.GetValue();
    auto element = doc->NewElement(tag);
    element->SetAttribute("r", value.x);
    element->SetAttribute("g", value.y);
    element->SetAttribute("b", value.z);
    return element;
}

//-----------------------------------------------------------------------------
//      XMLエレメントを生成します.
//-----------------------------------------------------------------------------
tinyxml2::XMLElement* Serialize(tinyxml2::XMLDocument* doc, const char* tag, const EditColor4& control)
{
    auto value = control.GetValue();
    auto element = doc->NewElement(tag);
    element->SetAttribute("r", value.x);
    element->SetAttribute("g", value.y);
    element->SetAttribute("b", value.z);
    element->SetAttribute("a", value.w);
    return element;
}

#if 0
//-----------------------------------------------------------------------------
//      XMLエレメントを生成します.
//-----------------------------------------------------------------------------
tinyxml2::XMLElement* Serialize(tinyxml2::XMLDocument* doc, const char* tag, const EditTexture& control)
{
    auto element = doc->NewElement(tag);
    element->SetAttribute("path", control.GetPath().c_str());
    return element;
}
#endif

//-----------------------------------------------------------------------------
//      XMLエレメントを解析します.
//-----------------------------------------------------------------------------
void Deserialize(tinyxml2::XMLElement* element, const char* tag, EditBool& control)
{
    auto e = element->FirstChildElement(tag);
    if (e == nullptr)
    { return; }

    auto value = e->BoolAttribute("value");
    control = EditBool(value);
}

//-----------------------------------------------------------------------------
//      XMLエレメントを解析します.
//-----------------------------------------------------------------------------
void Deserialize(tinyxml2::XMLElement* element, const char* tag, EditInt& control)
{
    auto e = element->FirstChildElement(tag);
    if (e == nullptr)
    { return; }

    auto value = e->IntAttribute("value");
    control = EditInt(value);
}

//-----------------------------------------------------------------------------
//      XMLエレメントを解析します.
//-----------------------------------------------------------------------------
void Deserialize(tinyxml2::XMLElement* element, const char* tag, EditFloat& control)
{
    auto e = element->FirstChildElement(tag);
    if (e == nullptr)
    { return; }

    auto value = e->FloatAttribute("value");
    control = EditFloat(value);
}

//-----------------------------------------------------------------------------
//      XMLエレメントを解析します.
//-----------------------------------------------------------------------------
void Deserialize(tinyxml2::XMLElement* element, const char* tag, EditFloat2& control)
{
    auto e = element->FirstChildElement(tag);
    if (e == nullptr)
    { return; }

    Vector2 value;
    value.x = e->FloatAttribute("x");
    value.y = e->FloatAttribute("y");
    control = EditFloat2(value.x, value.y);
}

//-----------------------------------------------------------------------------
//      XMLエレメントを解析します.
//-----------------------------------------------------------------------------
void Deserialize(tinyxml2::XMLElement* element, const char* tag, EditFloat3& control)
{
    auto e = element->FirstChildElement(tag);
    if (e == nullptr)
    { return; }

    Vector3 value;
    value.x = e->FloatAttribute("x");
    value.y = e->FloatAttribute("y");
    value.z = e->FloatAttribute("z");
    control = EditFloat3(value.x, value.y, value.z);
}

//-----------------------------------------------------------------------------
//      XMLエレメントを解析します.
//-----------------------------------------------------------------------------
void Deserialize(tinyxml2::XMLElement* element, const char* tag, EditFloat4& control)
{
    auto e = element->FirstChildElement(tag);
    if (e == nullptr)
    { return; }

    Vector4 value;
    value.x = e->FloatAttribute("x");
    value.y = e->FloatAttribute("y");
    value.z = e->FloatAttribute("z");
    value.w = e->FloatAttribute("w");
    control = EditFloat4(value.x, value.y, value.z, value.w);
}

//-----------------------------------------------------------------------------
//      XMLエレメントを解析します.
//-----------------------------------------------------------------------------
void Deserialize(tinyxml2::XMLElement* element, const char* tag, EditColor3& control)
{
    auto e = element->FirstChildElement(tag);
    if (e == nullptr)
    { return; }

    Vector3 value;
    value.x = e->FloatAttribute("r");
    value.y = e->FloatAttribute("g");
    value.z = e->FloatAttribute("b");
    control = EditColor3(value.x, value.y, value.z);
}

//-----------------------------------------------------------------------------
//      XMLエレメントを解析します.
//-----------------------------------------------------------------------------
void Deserialize(tinyxml2::XMLElement* element, const char* tag, EditColor4& control)
{
    auto e = element->FirstChildElement(tag);
    if (e == nullptr)
    { return; }

    Vector4 value;
    value.x = e->FloatAttribute("r");
    value.y = e->FloatAttribute("g");
    value.z = e->FloatAttribute("b");
    value.w = e->FloatAttribute("a");
    control = EditColor4(value.x, value.y, value.z, value.w);
}

#if 0
//-----------------------------------------------------------------------------
//      XMLエレメントを解析します.
//-----------------------------------------------------------------------------
void Deserialize(tinyxml2::XMLElement* element, const char* tag, EditTexture& control)
{
    auto e = element->FirstChildElement(tag);
    if (e == nullptr)
    { return; }

    auto path = e->Attribute("path");
    control.SetPath(path);
}
#endif

} // namespace asdx
#endif//ASDX_ENABLE_TINYXML2
