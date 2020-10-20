#ifndef JLT_HASH_HASH_HPP
#define JLT_HASH_HASH_HPP

#include <cstdint>
#include <jolt/api.hpp>
#include <jolt/util.hpp>

namespace jolt {
    namespace hash {
        using hash_t = uint64_t;

        struct XXHash {
            JLT_NODISCARD static JLTAPI hash_t hash(const void *const data, size_t const size);
        };

        struct Identity {
            template<typename T>
            JLT_NODISCARD static hash_t hash(const T *const data, size_t const size) {
                return (hash_t)(*data);
            }
        };

        template<typename H = XXHash>
        struct ObjectHash {
            template<typename T>
            JLT_NODISCARD static hash_t hash(const T *const object, size_t const) {
                return object->template hash<H>();
            }

            template<typename T>
            JLT_NODISCARD static hash_t hash(const Assignable<T> *const object, size_t const) {
                return object->get().template hash<H>();
            }
        };
    } // namespace hash
} // namespace jolt

#endif /* JLT_HASH_HASH_HPP */
