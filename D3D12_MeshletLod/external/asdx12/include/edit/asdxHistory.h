//-----------------------------------------------------------------------------
// File : asdxHistory.h
// Desc : History Module.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <list>
#include <vector>
#include <functional>
#include <fnd/asdxSpinLock.h>


namespace asdx {

//-----------------------------------------------------------------------------
// Type Definition
//-----------------------------------------------------------------------------
using Action = std::function<void(void)>;


///////////////////////////////////////////////////////////////////////////////
// IEventListener interface
///////////////////////////////////////////////////////////////////////////////
struct IEventListener
{
    virtual ~IEventListener() {}
    virtual void OnNotify() = 0;
};

///////////////////////////////////////////////////////////////////////////////
// IHistory interface
///////////////////////////////////////////////////////////////////////////////
struct IHistory
{
    virtual ~IHistory() {}
    virtual void Redo() = 0;
    virtual void Undo() = 0;
};

///////////////////////////////////////////////////////////////////////////////
// EventHandler class
///////////////////////////////////////////////////////////////////////////////
class EventHandler
{
public:
    EventHandler();
    ~EventHandler();

    void Invoke();
    EventHandler& operator += (IEventListener* listener);
    EventHandler& operator -= (IEventListener* listener);

private:
    std::list<IEventListener*>  m_Listeners;
};

///////////////////////////////////////////////////////////////////////////////
// History class
///////////////////////////////////////////////////////////////////////////////
class History : public IHistory
{
public:
    History(Action redo, Action undo);
    ~History();
    void Redo() override;
    void Undo() override;

private:
    Action  m_Redo;
    Action  m_Undo;
};

///////////////////////////////////////////////////////////////////////////////
// GroupHistory class
///////////////////////////////////////////////////////////////////////////////
class GroupHistory : public IHistory
{
public:
    EventHandler UndoExecuted;
    EventHandler RedoExecuted;

    GroupHistory();
    ~GroupHistory();

    void Add     (IHistory* item);
    void Clear   ();
    bool IsEmpty () const;
    int  GetCount() const;
    void Redo    () override;
    void Undo    () override;

private:
    std::list<IHistory*>    m_Histories;
};

///////////////////////////////////////////////////////////////////////////////
// HistoryMgr class
///////////////////////////////////////////////////////////////////////////////
class HistoryMgr
{
public:
    EventHandler UndoExecuted;
    EventHandler RedoExecuted;

    static HistoryMgr& Instance();

    bool Init           (int capacity);
    void Term           ();
    void Add            (IHistory* item, bool redo = true);
    void Clear          ();
    void Redo           ();
    void Undo           ();
    int  GetCurrent     () const;
    int  GetRedoCount   () const;
    int  GetUndoCount   () const;
    bool CanRedo        () const;
    bool CanUndo        () const;
    bool IsEmpty        () const;
    bool IsInit         () const;

private:
    static HistoryMgr       s_Instance;
    std::vector<IHistory*>  m_Histories;
    int                     m_Current;
    int                     m_Capacity;
    bool                    m_Init;
    SpinLock                m_SpinLock;

    HistoryMgr ();
    ~HistoryMgr();
};


} // namespace asdx
