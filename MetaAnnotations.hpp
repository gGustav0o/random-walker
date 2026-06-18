#pragma once

#if defined(__GNUC__) || defined(__clang__)
#define _pure_(decl)       [[nodiscard]] inline static constexpr decl noexcept
#define _cold_             [[gnu::cold]]
#define _flatten_          [[gnu::flatten]]
#define _always_inline_    [[gnu::always_inline]] inline
#define _likely_(x)        __builtin_expect(!!(x), 1)
#define _unlikely_(x)      __builtin_expect(!!(x), 0)
#elif defined(_MSC_VER)    
#define _pure_(decl)       [[nodiscard]] static constexpr decl noexcept
#define _cold_             
#define _flatten_          
#define _always_inline_    __forceinline
#define _likely_(x)        (x)
#define _unlikely_(x)      (x)
#else                      
#define _pure_(decl)       [[nodiscard]] inline static constexpr decl noexcept
#define _cold_             
#define _flatten_          
#define _always_inline_    inline
#define _likely_(x)        (x)
#define _unlikely_(x)      (x)
#endif


#define _impure_(decl)             [[nodiscard]] static decl noexcept
#define _impure_throwing_(decl)    [[nodiscard]] static decl
#define _fx_(decl)                 static constexpr decl noexcept
#define _fx_throwing_(decl)        static constexpr decl
#define _consteval_                consteval
#define _unused_                   [[maybe_unused]]

#define _no_copy_(Type)         \
    Type(const Type&) = delete; \
    Type& operator=(const Type&) = delete;

#define _no_move_(Type)    \
    Type(Type&&) = delete; \
    Type& operator=(Type&&) = delete;

#define _no_copy_move_(Type) \
    _no_copy_(Type)          \
    _no_move_(Type)

#define _static_only_(Type) \
    Type() = delete;        \
    _no_copy_move_(Type)

#define _todo_     static_assert(false, "TODO")
#define _dbg_(msg) do { std::cerr << "[DBG] " << msg << '\n'; } while (0)
