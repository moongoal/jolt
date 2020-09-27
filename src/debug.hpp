#ifndef JLT_ASSERT_H
#define JLT_ASSERT_H

#ifdef NDEBUG
    #define jltassert(x) ((void)0)
#else
    #define jltassert(x)                                                                           \
        do {                                                                                       \
            if(!(x)) {                                                                             \
                __asm__ volatile("int $0x03");                                                     \
            }                                                                                      \
        } while(false)
#endif // NDEBUG

#include <debug/console.hpp>

#endif /* JLT_ASSERT_H */
