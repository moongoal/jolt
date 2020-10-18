#ifndef JLT_UTIL_HPP
#define JLT_UTIL_HPP

#include <cstdint>
#include <utility>
#include <jolt/api.hpp>

#define JLT_TO_STRING(x) #x
#define JLT_QUOTE(x) JLT_TO_STRING(x)

namespace jolt {
    template<typename T>
    JLT_NODISCARD constexpr T choose(T const a, T const b, bool const condition) {
        const uint64_t mask = ~static_cast<uint64_t>(0) + condition;

        return static_cast<T>((static_cast<uint64_t>(b) & mask) | (static_cast<uint64_t>(a) & ~mask));
    }

    template<typename T>
    JLT_NODISCARD constexpr T *choose(T *const a, T *const b, bool const condition) {
        return reinterpret_cast<T *>(
          choose(reinterpret_cast<uintptr_t>(a), reinterpret_cast<uintptr_t>(b), condition));
    }

    JLT_NODISCARD void *align_raw_ptr(void *const ptr, size_t const alignment) JLTAPI;

    template<typename T>
    JLT_NODISCARD constexpr T max(T const a, T const b) {
        return choose(a, b, a > b);
    }

    template<typename T>
    JLT_NODISCARD constexpr T min(T const a, T const b) {
        return choose(a, b, a < b);
    }

    template<typename T>
    class Assignable {
      public:
        using value_type = T;
        using reference = T &;
        using const_reference = T const &;
        using rvalue_reference = T &&;

      private:
        value_type m_value;

      public:
        JLT_NODISCARD Assignable(reference value) : m_value{value} {}
        JLT_NODISCARD Assignable(const_reference value) : m_value{value} {}
        JLT_NODISCARD Assignable(rvalue_reference value) : m_value{std::forward(value)} {}

        JLT_NODISCARD reference get() { return m_value; }
        JLT_NODISCARD const_reference get() const { return m_value; }

        JLT_NODISCARD operator reference() { return m_value; }
        JLT_NODISCARD operator rvalue_reference() { return std::move(m_value); }
        JLT_NODISCARD operator const_reference() const { return m_value; }

        Assignable &operator=(const_reference value) {
            m_value.~value_type();

            // jltconstruct(&m_value, value);
            new(&m_value) value_type{value};

            return *this;
        }

        Assignable &operator=(const Assignable &value) {
            m_value.~value_type();

            // jltconstruct(&m_value, value.get());
            new(&m_value) value_type{value.get()};

            return *this;
        }

        JLT_NODISCARD bool operator==(const_reference other) const { return m_value == other; }

        JLT_NODISCARD bool operator==(const Assignable &other) const { return m_value == other.get(); }
    };
} // namespace jolt

#endif /* JLT_UTIL_HPP */
