#ifndef JLT_UTIL_HPP
#define JLT_UTIL_HPP

#include <cstdint>
#include <utility>
#include <jolt/api.hpp>

#define JLT_TO_STRING(x) #x
#define JLT_QUOTE(x) JLT_TO_STRING(x)

namespace jolt {
    template<typename T>
    constexpr T choose(T const a, T const b, bool const condition) {
        const uint64_t mask = ~static_cast<uint64_t>(0) + condition;

        return static_cast<T>((static_cast<uint64_t>(b) & mask) | (static_cast<uint64_t>(a) & ~mask));
    }

    template<typename T>
    constexpr T *choose(T *const a, T *const b, bool const condition) {
        return reinterpret_cast<T *>(
          choose(reinterpret_cast<uintptr_t>(a), reinterpret_cast<uintptr_t>(b), condition));
    }

    void *align_raw_ptr(void *const ptr, size_t const alignment) JLTAPI;

    template<typename T>
    constexpr T max(T const a, T const b) {
        return choose(a, b, a > b);
    }

    template<typename T>
    constexpr T min(T const a, T const b) {
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
        Assignable(reference value) : m_value{value} {}
        Assignable(const_reference value) : m_value{value} {}
        Assignable(rvalue_reference value) : m_value{std::forward(value)} {}

        reference get() { return m_value; }
        const_reference get() const { return m_value; }

        operator reference() { return m_value; }
        operator rvalue_reference() { return std::move(m_value); }
        operator const_reference() const { return m_value; }

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

        bool operator==(const_reference other) const { return m_value == other; }

        bool operator==(const Assignable &other) const { return m_value == other.get(); }
    };
} // namespace jolt

#endif /* JLT_UTIL_HPP */
