#ifndef JLT_TEST_HPP
#define JLT_TEST_HPP

#include <string_view>
#include <vector>
#include <iostream>
#include "util.hpp"

#ifdef assert
    #undef assert
#endif // assert

#define assert(pred)                                                                               \
    do {                                                                                           \
        if(!(pred))                                                                                \
            fail();                                                                                \
    } while(false)

#define assert2(pred, msg)                                                                         \
    do {                                                                                           \
        if(!(pred))                                                                                \
            fail_with_message(msg);                                                                \
    } while(false)

#define fail()                                                                                     \
    do {                                                                                           \
        set_result(jolt::test::test_result::FAILURE);                                              \
        return;                                                                                    \
    } while(false)

#define fail_with_message(msg)                                                                     \
    do {                                                                                           \
        set_result(jolt::test::test_result::FAILURE);                                              \
        set_failure_message(msg);                                                                  \
        return;                                                                                    \
    } while(false)

#define ignore()                                                                                   \
    do {                                                                                           \
        set_result(jolt::test::test_result::IGNORE);                                               \
        return;                                                                                    \
    } while(false)

#define TEST2(test_func_name, is_ignored)                                                          \
    class test_function_impl_##test_func_name : public jolt::test::test_function {                 \
      public:                                                                                      \
        test_function_impl_##test_func_name(std::string_view test_name, bool ignore = false) :     \
          test_function{test_name, ignore} {}                                                      \
        void test_func() override;                                                                 \
    } test_function_inst_##test_func_name{JLT_TO_STRING(test_func_name), is_ignored};              \
    void test_function_impl_##test_func_name::test_func()

#define TEST(test_func_name) TEST2(test_func_name, false)
#define TEST_IGNORE(test_func_name) TEST2(test_func_name, true)

#define SETUP                                                                                      \
    struct setup_function_impl : public jolt::test::setup_function {                               \
        setup_function_impl() : setup_function{} {}                                                \
        void run() override;                                                                       \
    } setup_function_inst;                                                                         \
    void setup_function_impl::run()

#define CLEANUP                                                                                    \
    struct cleanup_function_impl : public jolt::test::cleanup_function {                           \
        cleanup_function_impl() : cleanup_function{} {}                                            \
        void run() override;                                                                       \
    } cleanup_function_inst;                                                                       \
    void cleanup_function_impl::run()

namespace jolt {
    namespace test {
        enum class test_result { NOT_RUN, SUCCESS, FAILURE, IGNORE };

        class test_function {
            std::string_view m_name;
            std::string m_failure_message;
            test_result m_result;

          public:
            test_function(std::string_view test_name, bool ignore = false);

            [[maybe_unused]] virtual void test_func() = 0;

            void run() {
                if(result() == test_result::NOT_RUN) {
                    set_result(test_result::SUCCESS);
                    test_func();
                }
            }

            void set_result(test_result result) noexcept { m_result = result; }
            void set_failure_message(std::string msg) noexcept { m_failure_message = msg; }
            [[nodiscard]] test_result result() const noexcept { return m_result; }
            [[nodiscard]] std::string_view name() const noexcept { return m_name; }
            [[nodiscard]] const std::string &failure_message() const noexcept {
                return m_failure_message;
            }
        };

        struct setup_function {
            setup_function();
            virtual void run() = 0;
        };

        struct cleanup_function {
            cleanup_function();
            virtual void run() = 0;
        };

        class test_case {
            std::vector<test_function *> m_tests;
            setup_function *m_setup;
            cleanup_function *m_cleanup;

          public:
            test_case() : m_tests{}, m_setup{nullptr} {}
            test_case(const test_case &other) = delete;

            void register_test(test_function &test) { m_tests.push_back(&test); }
            void register_setup(setup_function &setup) { m_setup = &setup; }
            void register_cleanup(cleanup_function &cleanup) { m_cleanup = &cleanup; }

            bool run() {
                bool overall_result = true;
                const std::vector<test_function *>::const_iterator it_end = m_tests.end();

                if(m_setup) {
                    std::cerr << "Setting up..." << std::endl;
                    m_setup->run();
                }

                std::cerr << "Running " << m_tests.size() << " tests..." << std::endl;
                std::cerr << "*********************************************************************"
                          << std::endl;

                for(auto it = m_tests.begin(); it != it_end; it++) {
                    test_function &test = **it;
                    test_result const result = run_test_func(test);
                    overall_result &= (result != test_result::FAILURE);
                }

                if(m_cleanup) {
                    std::cerr << "Tearing down..." << std::endl;
                    m_cleanup->run();
                }

                return overall_result;
            }

            test_result run_test_func(test_function &test) {
                std::string_view test_name = test.name();

                std::cerr << test_name << "... ";

                test.run();
                test_result result = test.result();

                switch(result) {
                    case test_result::SUCCESS:
                        std::cerr << "PASS";
                        break;

                    case test_result::FAILURE: {
                        const std::string &failure_message = test.failure_message();

                        std::cerr << "FAIL";

                        if(!failure_message.empty()) {
                            std::cerr << ": " << failure_message;
                        }

                        break;
                    }

                    case test_result::IGNORE:
                        std::cerr << "-IGNORED-";
                        break;

                    case test_result::NOT_RUN:
                        std::cerr << "NOT RUN";
                        break;

                    default:
                        std::cerr << "INVALID TEST RESULT VALUE " << (int)result;
                }

                std::cerr << std::endl;

                return result;
            }
        } test_case_instance;

        test_function::test_function(std::string_view test_name, bool ignore) :
          m_name{test_name}, m_result{ignore ? test_result::IGNORE : test_result::NOT_RUN} {
            test_case_instance.register_test(*this);
        }

        setup_function::setup_function() { test_case_instance.register_setup(*this); }
        cleanup_function::cleanup_function() { test_case_instance.register_cleanup(*this); }
    } // namespace test
} // namespace jolt

int main() { return 1 - jolt::test::test_case_instance.run(); }

#endif // JLT_TEST_HPP
