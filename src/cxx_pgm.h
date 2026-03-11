// Licensed under the MIT License. See LICENSE.txt file in the project root.

////////
// some C++ abstraction for PROGMEM
//
// wrapper template Pgm<T> is data in PROGMEM (flash) and protects direct access.
// the address of a Pgm<T> is a Pgm_ptr<T>, which can be derefereced
// and reults in correct flash reads.
//
// Gen_ptr<T> is a pointer to either RAM or flash (actually a 15bit pointer plus 1bit tag).
//
#pragma once

#include <avr/pgmspace.h>
#include <inttypes.h>

#include <type_traits>

template<typename T> class Pgm;
template<typename T> class Pgm_ref;

template<typename T>
class Pgm_ptr
{
private:
  T const *_ptr;

  static T _pgm_read(T const *xaddr) noexcept
  {
    uint8_t const *const addr = reinterpret_cast<uint8_t const *>(xaddr);
    union X
    {
      std::remove_const_t<T> t;
      uint8_t a[sizeof(T)];
      uint16_t b[sizeof(T) / 2];
      uint32_t c[sizeof(T) / 4];
    };

    X r;
    switch (sizeof(T))
      {
      case 1:
        r.a[0] = pgm_read_byte(addr);
        return r.t;
      case 2:
        r.b[0] = pgm_read_word(addr);
        return r.t;
      case 3:
        r.b[0] = pgm_read_word(addr);
        r.b[2] = pgm_read_byte(addr + 2);
        return r.t;
      case 4:
        r.c[0] = pgm_read_dword(addr);
        return r.t;
      default:
        static_assert(sizeof(T) <= sizeof(uint32_t), "objects in PGM too big (> 4bytes)");
        return r.t;
      }
  }

  template<typename IDX>
  constexpr T _get(IDX &&idx) const noexcept
  {
    return _pgm_read(_ptr + idx);
  }

public:
  template<typename O> friend class Pgm_ptr;
  friend class Pgm<T>;
  friend class Pgm_ref<T>;

  constexpr Pgm_ptr() = default;
  constexpr explicit Pgm_ptr(T const *ptr) noexcept : _ptr(ptr) {}
  constexpr Pgm_ptr(nullptr_t) noexcept : _ptr(nullptr) {}
  constexpr explicit operator bool () const noexcept { return _ptr != nullptr; }
  constexpr explicit operator nullptr_t () const noexcept { return reinterpret_cast<nullptr_t>(_ptr); }

  template<typename O, typename = std::enable_if_t<std::is_convertible<O*, T*>::value>>
  constexpr Pgm_ptr(Pgm_ptr<O> const &o) noexcept : _ptr(o._ptr) {}

  template<typename O>
  constexpr explicit operator Pgm_ptr<O> () const noexcept
  { return Pgm_ptr<O>(static_cast<O const *>(_ptr)); }

  template<typename O>
  constexpr Pgm_ptr<O> reinterpret() const noexcept
  { return Pgm_ptr<O>(reinterpret_cast<O const *>(_ptr)); }

  constexpr T const *pgm_addr() const noexcept { return _ptr; }

  constexpr T get() const noexcept
  { return _get(0); }

  constexpr T operator * () const noexcept { return _get(0); }

  template<typename IDX>
  constexpr T operator [] (IDX index) const noexcept { return _get(index); }


  constexpr Pgm_ptr &operator ++ () noexcept [[always_inline]]
  {
    ++_ptr;
    return *this;
  }

  constexpr Pgm_ptr operator ++ (int) noexcept [[always_inline]]
  {
    Pgm_ptr tmp = *this;
    ++_ptr;
    return tmp;
  }

  constexpr Pgm_ptr &operator += (int offs) noexcept [[always_inline]]
  {
    _ptr += offs;
    return *this;
  }


  constexpr Pgm_ptr &operator -- () noexcept [[always_inline]]
  {
    --_ptr;
    return *this;
  }

  constexpr Pgm_ptr operator -- (int) noexcept [[always_inline]]
  {
    Pgm_ptr tmp = *this;
    --_ptr;
    return tmp;
  }

  constexpr Pgm_ptr &operator -= (int offs) noexcept [[always_inline]]
  {
    _ptr -= offs;
    return *this;
  }

  constexpr Pgm_ptr operator - (int offs) const noexcept [[always_inline]]
  {
    return Pgm_ptr(_ptr - offs);
  }

  constexpr Pgm_ptr operator + (int offs) const noexcept [[always_inline]]
  {
    return Pgm_ptr(_ptr + offs);
  }
};

template<typename T>
class Pgm_ref
{
private:
  T const &_o;

public:
  constexpr explicit Pgm_ref(T const &o) : _o(o) {}
  Pgm_ref(Pgm_ref const &) = default;
  Pgm_ref(Pgm_ref &&) = default;
  Pgm_ref &operator = (Pgm_ref const &) = delete;
  Pgm_ref &operator = (Pgm_ref &&) = delete;

  constexpr Pgm_ptr<T> operator & () const noexcept { return Pgm_ptr<T>(&_o); }
  constexpr T const *pgm_addr() const noexcept { return &_o; }
  constexpr T read() const noexcept { return Pgm_ptr<T>::_pgm_read(&_o); }
  constexpr operator T () const noexcept { return read(); }
};

template<typename T>
class Pgm
{
private:
  T _o;

public:
  constexpr Pgm() = default;
  constexpr explicit Pgm(T const &o) : _o(o) {}
  Pgm(Pgm const &) = delete;
  Pgm(Pgm &&) = delete;
  Pgm &operator = (Pgm const &) = delete;
  Pgm &operator = (Pgm &&) = delete;

  constexpr Pgm_ptr<T> operator & () const noexcept { return Pgm_ptr<T>(&_o); }
  constexpr T read() const noexcept { return Pgm_ptr<T>::_pgm_read(&_o); }
  constexpr operator T () const noexcept { return read(); }
} PROGMEM;

template<typename T, unsigned SIZE>
class Pgm<T[SIZE]>
{
private:
  T _o[SIZE];

public:

  constexpr Pgm() = default;
  constexpr Pgm(T const (&b)[SIZE]) noexcept { for (unsigned i = 0; i < SIZE; ++i) _o[i] = b[i];  }

  Pgm(Pgm const &) = delete;
  Pgm(Pgm &&) = delete;
  Pgm &operator = (Pgm const &) = delete;
  Pgm &operator = (Pgm &&) = delete;

  constexpr Pgm_ptr<T> operator & () const noexcept { return Pgm_ptr<T>(_o); }

  template<typename INDEX>
  constexpr Pgm_ref<std::remove_extent_t<T>> operator [] (INDEX idx) const noexcept
  { return Pgm_ref<T>(_o[idx]); }
} PROGMEM;


template<typename T>
class Gen_ptr
{
private:
  uintptr_t _ptr;

  explicit constexpr Gen_ptr(T *p) noexcept : _ptr(reinterpret_cast<uintptr_t>(p)) {}
  explicit constexpr Gen_ptr(T *p, bool) noexcept : _ptr(reinterpret_cast<uintptr_t>(p) + 0x8000) {}
  explicit constexpr Gen_ptr(intptr_t p) noexcept : _ptr(p) {}

  constexpr T *_addr() const noexcept { return reinterpret_cast<T *>(_ptr & ~0x8000); }

  template<typename IDX>
  constexpr std::remove_extent_t<T> _get(IDX &&idx) const noexcept
  {
    return is_pgm() ? *Pgm_ptr<T>(_addr() + idx) : *(_addr() + idx);
  }

public:
  template<typename O>
  friend class Gen_ptr;

  constexpr Gen_ptr() = default;
  constexpr Gen_ptr(nullptr_t) noexcept : _ptr(0) {}
  constexpr explicit operator bool () const noexcept { return _ptr != 0; }
  constexpr explicit operator nullptr_t () const noexcept { return reinterpret_cast<nullptr_t>(_ptr); }

  template<typename O, typename = std::enable_if_t<std::is_convertible<O*, T*>::value>>
  constexpr Gen_ptr(Gen_ptr<O> const &o) noexcept : _ptr(o._ptr) {}

  template<typename O, typename = std::enable_if_t<std::is_convertible<O*, T*>::value>>
  constexpr Gen_ptr(Pgm_ptr<O> const &o) noexcept : Gen_ptr(o.pgm_addr(), true)  {}

  constexpr static Gen_ptr pgm(T *ptr) noexcept { return Gen_ptr(ptr, true); }
  constexpr static Gen_ptr ram(T *ptr) noexcept { return Gen_ptr(ptr); }

  constexpr bool is_pgm() const noexcept { return _ptr & 0x8000; }

  constexpr std::remove_extent_t<T> get() const noexcept
  { return _get(0); }

  constexpr std::remove_extent_t<T> operator * () const noexcept { return _get(0); }

  template<typename IDX>
  constexpr std::remove_extent_t<T> operator [] (IDX index) const noexcept { return _get(index); }

  constexpr Gen_ptr &operator ++ () noexcept [[always_inline]]
  {
    _ptr += sizeof(T);
    return *this;
  }

  constexpr Gen_ptr operator ++ (int) noexcept [[always_inline]]
  {
    Gen_ptr tmp = *this;
    _ptr += sizeof(T);
    return tmp;
  }

  constexpr Gen_ptr &operator += (int offs) noexcept [[always_inline]]
  {
    _ptr += (sizeof(T) * offs);
    return *this;
  }


  constexpr Gen_ptr &operator -- () noexcept [[always_inline]]
  {
    _ptr -= sizeof(T);
    return *this;
  }

  constexpr Gen_ptr operator -- (int) noexcept [[always_inline]]
  {
    Gen_ptr tmp = *this;
    _ptr -= sizeof(T);
    return tmp;
  }

  constexpr Gen_ptr &operator -= (int offs) noexcept [[always_inline]]
  {
    _ptr -= (sizeof(T) * offs);
    return *this;
  }

  constexpr Gen_ptr operator - (int offs) const noexcept [[always_inline]]
  {
    return Gen_ptr(_ptr - (sizeof(T) * offs));
  }

  constexpr Gen_ptr operator + (int offs) const noexcept [[always_inline]]
  {
    return Gen_ptr(_ptr + (sizeof(T) * offs));
  }

  template<typename O>
  constexpr explicit operator Gen_ptr<O> () const noexcept
  { return Gen_ptr<O>(_ptr); }

  constexpr T *addr() const noexcept { return reinterpret_cast<T *>(_ptr & ~0x8000); }
} __attribute__((packed));

template<typename O>
constexpr Gen_ptr<O> pgm_ptr(O *o) noexcept { return Gen_ptr<O>::pgm(o); }

template<typename O>
constexpr Gen_ptr<O> ram_ptr(O *o) noexcept { return Gen_ptr<O>::ram(o); }

template<typename O, typename From>
constexpr Gen_ptr<O> gen_ptr_recast(Gen_ptr<From> f) noexcept
{
  return f.template reinterpret<O>();
}

template<typename O, typename From>
constexpr Pgm_ptr<O> gen_ptr_recast(Pgm_ptr<From> f) noexcept
{
  return f.template reinterpret<O>();
}

template<typename O, typename From>
constexpr O *gen_ptr_recast(From *f) noexcept
{
  return reinterpret_cast<O *>(f);
}

template<typename O, typename From>
constexpr Gen_ptr<O> gen_ptr_cast(Gen_ptr<From> f) noexcept
{
  return static_cast<Gen_ptr<O>>(f);
}

template<typename O, typename From>
constexpr Pgm_ptr<O> gen_ptr_cast(Pgm_ptr<From> f) noexcept
{
  return static_cast<Pgm_ptr<O>>(f);
}

template<typename O, typename From>
constexpr O *gen_ptr_cast(From *f) noexcept
{
  return static_cast<O *>(f);
}

