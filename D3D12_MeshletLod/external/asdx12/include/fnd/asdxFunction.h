//-----------------------------------------------------------------------------
// File : asdxFunction.h
// Desc : Fixed Size Function.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cassert>
#include <utility>
#include <new>


namespace asdx {

//=============================================================================
// Forward Declarations.
//=============================================================================
template<typename Func, size_t MaxSize=16, size_t Align=4>
class Function;

///////////////////////////////////////////////////////////////////////////////
// Function class
///////////////////////////////////////////////////////////////////////////////
template<typename ReturnType, typename... Args, size_t MaxSize, size_t Align>
class Function<ReturnType(Args...), MaxSize, Align>
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    /* NOTHING */

public:
    //=========================================================================
    // Type Definition.
    //=========================================================================
    using result_type = ReturnType;

    //=========================================================================
    // public variables.
    //=========================================================================
    /* NOTHING */

    //=========================================================================
    // public methods.
    //=========================================================================

    Function()
    { /* DO_NOTHING */ }

    ~Function()
    { reset(); }

    Function(std::nullptr_t)
    { /* DO_NOTHING */ }

    Function(Function const& value)
    { copy(value); }

    Function(Function& value)
    { copy(value); }

    Function(Function&& value)
    { move(std::move(value)); }

    template<typename Functor>
    Function(Functor&& f)
    { create(std::forward<Functor>(f)); }

    Function& operator = (std::nullptr_t)
    {
        reset();
        return *this;
    }

    Function& operator = (Function const& value)
    {
        assign(value);
        return *this;
    }

    Function& operator = (Function& value)
    {
        assign(value);
        return *this;
    }

    Function& operator = (Function&& value)
    {
        assign(std::move(value));
        return *this;
    }

    template<typename Functor>
    Function& operator = (Functor&& f)
    {
        assign(std::forward<Functor>(f));
        return *this;
    }

    void assign(Function const& value)
    {
        reset();
        copy(value);
    }

    void assign(Function& value)
    {
        reset();
        copy(value);
    }

    void assign(Function&& value)
    {
        reset();
        move(std::move(value));
    }

    template<typename Functor>
    void assign(Functor&& f)
    {
        reset();
        create(std::forward<Functor>(f));
    }

    void reset()
    {
        if (m_Base == nullptr)
        { return; }

        m_Base->Dispose(m_Storage);
        m_Base = nullptr;
    }

    explicit operator bool() const
    { return m_Base != nullptr; }

    ReturnType operator()(Args... args)
    {
        assert(m_Base != nullptr);
        return m_Base->Invoke(std::forward<Args>(args)...);
    }

    void swap(Function& other)
    {
        auto temp = std::move(other);
        other = std::move(*this);
        *this = std::move(temp);
    }

    friend void swap(Function& lhs, Function& rhs)
    { lhs.swap(rhs); }

    friend bool operator == (std::nullptr_t, Function const& action)
    { return nullptr == action.m_Base; }

    friend bool operator == (Function const& action, std::nullptr_t)
    { return action.m_Base == nullptr; }

    friend bool operator != (std::nullptr_t, Function const& action)
    { return nullptr != action.m_Base; }

    friend bool operator != (Function const& action, std::nullptr_t)
    { return action.m_Base != nullptr; }

private:
    struct Base
    {
        virtual ~Base() {}
        virtual ReturnType Invoke (Args&& ...) = 0;
        virtual void       Copy   (void*)      = 0;
        virtual void       Move   (void*)      = 0;
        virtual void       Dispose(void*)      = 0;
    };

    template<typename Functor>
    struct Derived : public Base
    {
        Functor func;

        Derived(Functor f)
        : func(std::move(f))
        { /* DO_NOTHING */ }

        ReturnType Invoke(Args&& ... args) override
        { return func(std::forward<Args>(args)...); }

        void Copy(void* dest) override
        { new (dest) Functor(func); }

        void Move(void* dest) override
        { new (dest) Functor(std::move(func)); }

        void Dispose(void* dest) override
        { static_cast<Functor*>(dest)->~Functor(); }
    };

    alignas(Align) uint8_t  m_Storage[MaxSize] = {};
    Base*                   m_Base             = nullptr;

    template<typename Functor>
    void create(Functor&& f)
    {
        static_assert(sizeof(Functor) <= MaxSize);
        m_Base = new(m_Storage) Derived<Functor>(std::forward<Functor>(f));
    }

    void copy(Function const& value)
    {
        value.m_Base->Copy(m_Storage);
        m_Base = value.m_Base;
    }

    void move(Function&& value)
    {
        value.m_Base->Move(m_Storage);
        m_Base = value.m_Base;
        value.reset();
    }

    // 以下, アクセス禁止.
    template<typename F, size_t S, size_t A> Function             (Function<F, S, A> const&)    = delete;
    template<typename F, size_t S, size_t A> Function             (Function<F, S, A>&)          = delete;
    template<typename F, size_t S, size_t A> Function             (Function<F, S, A>&&)         = delete;
    template<typename F, size_t S, size_t A> Function& operator = (Function<F, S, A> const&)    = delete;
    template<typename F, size_t S, size_t A> Function& operator = (Function<F, S, A>&)          = delete;
    template<typename F, size_t S, size_t A> Function& operator = (Function<F, S, A>&&)         = delete;
    template<typename F, size_t S, size_t A> void      assign     (Function<F, S, A> const&)    = delete;
    template<typename F, size_t S, size_t A> void      assign     (Function<F, S, A>&)          = delete;
    template<typename F, size_t S, size_t A> void      assign     (Function<F, S, A>&&)         = delete;
};

template<typename... Args>
using Action = Function<void(Args...)>;

} // namespace asdx
