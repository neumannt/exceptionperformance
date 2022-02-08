#ifndef H_tbv
#define H_tbv
//---------------------------------------------------------------------------
// Emulation code to simulate throw-by-value (wg21.link/p0709)
// Without compiler support we can only crudely approximate it, but it
// aims to provide low overhead error value support
//
// SPDX-License-Identifier: MIT
// (c) 2020 Thomas Neumann
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice (including the next
// paragraph) shall be included in all copies or substantial portions of the
// Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//---------------------------------------------------------------------------
#include <system_error>
#include <variant>
//---------------------------------------------------------------------------
namespace tbv {
//---------------------------------------------------------------------------
namespace detail {
//---------------------------------------------------------------------------
/// Do we have a tagged pointer?
constexpr inline bool isErrorPointer(uintptr_t v) { return v&1; }
//---------------------------------------------------------------------------
/// Tag a pointer
inline uintptr_t tagErrorPointer(const std::error_category& e) { return reinterpret_cast<uintptr_t>(&e)|1; }
//---------------------------------------------------------------------------
/// Untag a pointer
inline const std::error_category& untagErrorPointer(uintptr_t v) { return *reinterpret_cast<const std::error_category*>(v&(~static_cast<uintptr_t>(1))); }
//---------------------------------------------------------------------------
/// A helper struct for faster code in the error path. This help to avoid reconstructing the std::error_code for the inline cases
struct errorresult { void* p1,*p2; };
//---------------------------------------------------------------------------
/// Construct a std::error_code from an errorresult
inline std::error_code convert(errorresult e) { union { void* p; int i; } u; u.p=e.p1; return std::error_code(u.i, untagErrorPointer(reinterpret_cast<uintptr_t>(e.p2))); }
//---------------------------------------------------------------------------
/// A result value that is either a value or an error_code. Will be specialized
/// for various types to allow for a more efficient result passing
template <class T, int mode = (std::is_trivial<T>::value && (sizeof(T) <= sizeof(void*))) ? 1 : (sizeof(T) <= sizeof(void*)) ? 2 : 0> class resultimpl
{
   private:
   using storage = std::variant<T, std::error_code>;

   /// The content
   storage content;

   public:
   /// Constructor
   resultimpl(T v) noexcept(noexcept(storage(std::move(v)))) : content(std::move(v)) {}
   /// Constructor
   resultimpl(std::error_code e) noexcept : content(e) {}
   /// Constructor
   resultimpl(errorresult e) noexcept : content(e) {}
   /// Copy constructor
   resultimpl(const resultimpl&) = default;
   /// Move constructor
   resultimpl(resultimpl&&) = default;

   /// Assignment
   resultimpl& operator=(const resultimpl&) = default;
   /// Assignment
   resultimpl& operator=(resultimpl&&) = default;

   /// Do we have a value?
   explicit operator bool() const noexcept { return content.index() == 0; }
   /// Do we have a value?
   bool has_value() const noexcept { return content.index() == 0; }
   /// Do we have an error?
   bool has_error() const noexcept { return content.index() == 1; }

   /// Get the value
   const T& value() const noexcept(noexcept(std::get<0>(content))) { return std::get<0>(content); }
   /// Get the value
   T& value() noexcept(noexcept(std::get<0>(content))) { return std::get<0>(content); }
   /// Release the value
   T release() && noexcept(noexcept(T(std::get<0>(content)))) { return std::move(std::get<0>(content)); }
   /// Get the error code
   std::error_code error() const noexcept { return std::get<1>(content); }
   /// Get the error code in the best format to return it
   std::error_code error_return() const noexcept { return std::get<1>(content); }

   /// Swap the content
   void swap(resultimpl& other) noexcept(noexcept(content.swap(other.content))) { content.swap(other.content); }
};
//---------------------------------------------------------------------------
/// Helper class for trivial types. Aims to be passed as two pointers
template <class T> class resultimpl<T, 1> {
   private:
   /// The content, first half
   union { void* ptr1; T v; int ev; } c1;
   /// The content, second half
   union { void* ptr2; uintptr_t v; } c2;

   public:
   /// Constructor
   constexpr resultimpl(T v) noexcept : c1{.v=v}, c2{nullptr} {}
   /// Constructor
   resultimpl(std::error_code e) noexcept : c1{.ev=e.value()}, c2{.v =tagErrorPointer(e.category())} {}
   /// Constructor
   constexpr resultimpl(errorresult e) noexcept : c1{e.p1}, c2{e.p2} {}
   /// Move constructor
   resultimpl(resultimpl&&) = default;

   /// Assignment
   resultimpl& operator=(resultimpl&&) = default;

   /// Do we have a value?
   explicit operator bool() const noexcept { return !isErrorPointer(c2.v); }
   /// Do we have a value?
   bool has_value() const noexcept { return !isErrorPointer(c2.v); }
   /// Do we have an error?
   bool has_error() const noexcept { return isErrorPointer(c2.v); }

   /// Get the value
   T value() const noexcept { return c1.v; }
   /// Get the value
   T& value() noexcept { return c1.v; }
   /// Release the value
   T release() && noexcept { return c1.v; }
   /// Get the error code
   std::error_code error() const noexcept { return std::error_code(c1.ev, untagErrorPointer(c2.v)); }
   /// Get the error code in the best format to return it
   constexpr errorresult error_return() const noexcept { return errorresult{c1.ptr1, c2.ptr2}; }
};
//---------------------------------------------------------------------------
/// Helper class for non-trivial types that can be fit into one pointer
template <class T> class resultimpl<T, 2> {
   private:
   /// The content, first half
   union { void* ptr1; char v[sizeof(void*)]; int ev; } c1;
   /// The content, second half
   union { void* ptr2; uintptr_t v; } c2;

   /// Reference the object
   const T& ref() const noexcept { return *reinterpret_cast<const T*>(c1.v); }
   /// Reference the object
   T& ref() noexcept { return *reinterpret_cast<T*>(c1.v); }

   public:
   /// Constructor
   constexpr resultimpl(T v) noexcept(noexcept(T(std::move(v)))) : c2{nullptr} { new (c1.v) T(std::move(v)); }
   /// Constructor
   resultimpl(std::error_code e) noexcept : c1{.ev=e.value()}, c2{.v =tagErrorPointer(e.category())} {}
   /// Constructor
   constexpr resultimpl(errorresult e) noexcept : c1{e.p1}, c2{e.p2} {}
   /// Move constructor
   resultimpl(resultimpl&& o) noexcept(noexcept(T(std::move(o.c1.v1)))) : c2{o.c2.ptr2} { if (o) new (c1.v) T(std::move(o.ref())); else c1.ptr1=o.c1.ptr1; }
   /// Destructor
   ~resultimpl() { if (has_value()) ref().T::~T(); }

   /// Assignment
   resultimpl& operator=(resultimpl&& o) noexcept(noexcept(ref()=move(o.ref())) && noexcept(T(std::move(o.ref())))) { if (&o!=this) { if (o) { if (has_value()) ref()=std::move(o.ref()); else new (c1.v) T(std::move(o.ref())); } else { if (has_value()) ref().T::~T(); c1.ptr1=o.c1.ptr1; } c2.ptr2=o.c2.ptr2; } return *this; }

   /// Do we have a value?
   explicit operator bool() const noexcept { return !isErrorPointer(c2.v); }
   /// Do we have a value?
   bool has_value() const noexcept { return !isErrorPointer(c2.v); }
   /// Do we have an error?
   bool has_error() const noexcept { return isErrorPointer(c2.v); }

   /// Get the value
   const T& value() const noexcept { return ref(); }
   /// Get the value
   T& value() noexcept { return ref(); }
   /// Release the value
   T release() noexcept(noexcept(T(std::move(ref())))) { return std::move(ref()); }
   /// Get the error code
   std::error_code error() const noexcept { return std::error_code(c1.ev, untagErrorPointer(c2.v)); }
   /// Get the error code in the best format to return it
   constexpr errorresult error_return() const noexcept { return errorresult{c1.ptr1, c2.ptr2}; }
};
//---------------------------------------------------------------------------
}
//---------------------------------------------------------------------------
/// A result value that is either a value or an error_code. Will be specialized
/// for various types to allow for a more efficient result passing
template <class T> class [[nodiscard]] result : public detail::resultimpl<T>
{
   public:
   using detail::resultimpl<T>::resultimpl;
};
//---------------------------------------------------------------------------
/// A void result
template<> class [[nodiscard]] result<void> {
   private:
   /// The content, first half
   union { void* ptr1; int ev; } c1;
   /// The content, second half
   union { void* ptr2; uintptr_t v; } c2;

   public:
   /// Constructor
   constexpr result() noexcept : c1{nullptr}, c2{nullptr} { }
   /// Constructor
   result(std::error_code e) noexcept : c1{.ev=e.value()}, c2{.v=detail::tagErrorPointer(e.category())} { }
   /// Constructor
   constexpr result(detail::errorresult e) noexcept : c1{e.p1}, c2{e.p2} {}
   /// Move constructor
   result(result&&) = default;

   /// Assignment
   result& operator=(result&&) = default;

   /// Do we have a value?
   explicit operator bool() const noexcept { return !detail::isErrorPointer(c2.v); }
   /// Do we have a value?
   bool has_value() const noexcept { return !detail::isErrorPointer(c2.v); }
   /// Do we have an error?
   bool has_error() const noexcept { return detail::isErrorPointer(c2.v); }
   /// Release the value
   constexpr void release() noexcept { }

   /// Get the error code
   std::error_code error() const noexcept { return std::error_code(c1.ev, detail::untagErrorPointer(c2.v)); }
   /// Get the error code in the best format to return it
   constexpr detail::errorresult error_return() const noexcept { return detail::errorresult{c1.ptr1, c2.ptr2}; }
};
//---------------------------------------------------------------------------
/// Helper to throw/return an error
inline detail::errorresult throw_value [[nodiscard]] (std::error_code e) { return detail::errorresult{reinterpret_cast<void*>(static_cast<uintptr_t>(e.value())), reinterpret_cast<void*>(detail::tagErrorPointer(e.category()))}; }
//---------------------------------------------------------------------------
#ifndef TBV_NOMACROS
// Convenience macros
#define TRY(x) ({ auto _tbv_r = (x); if (_tbv_r.has_error()) [[unlikely]] return _tbv_r.error_return(); std::move(_tbv_r).release(); })
#endif
//---------------------------------------------------------------------------
}
//---------------------------------------------------------------------------
#endif
