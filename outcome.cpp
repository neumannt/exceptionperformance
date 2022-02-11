#include "thirdparty/outcome/outcome.hpp"
#include <cmath>
#include <span>

namespace outcome = outcome_v2_e261cebd;

#if 0
// This here is faster, but really minimal in expressiveness
enum class ErrorCode {
    InvalidValue
};
template<typename T>
using result = outcome::result<T, ErrorCode, outcome::policy::all_narrow>;
#define DOTHROW() return ErrorCode::InvalidValue
#else
// This is better comparable to the other approaches
template<typename T>
using result = outcome::result<T>;
#define DOTHROW() return std::make_error_code(std::errc::argument_out_of_domain)
#endif

static result<void> doSqrt(std::span<double> values) noexcept __attribute__((noinline));
static result<void> doSqrt(std::span<double> values) noexcept {
    for (auto &v: values) {
        if (v < 0) DOTHROW();
        v = sqrt(v);
    }
    return outcome::success();
}

unsigned outcomeResultSqrt(std::span<double> values, unsigned repeat) noexcept {
    unsigned failures = 0;
    for (unsigned index = 0; index != repeat; ++index) {
        if (result<void> r = doSqrt(values); !r)
            ++failures;
    }
    return failures;
}

static result<unsigned> doFib(unsigned n, unsigned maxDepth) noexcept __attribute((noinline, optimize("no-optimize-sibling-calls")));
static result<unsigned> doFib(unsigned n, unsigned maxDepth) noexcept {
    if (!maxDepth) DOTHROW();
    if (n <= 2) return 1;
    auto n2 = OUTCOME_TRYX(doFib(n - 2, maxDepth - 1));
    auto n1 = OUTCOME_TRYX(doFib(n - 1, maxDepth - 1));
    return n2 + n1;
}

unsigned outcomeResultFib(unsigned n, unsigned maxDepth) noexcept {
    if (result<unsigned> r = doFib(n, maxDepth))
        return r.value();
    else
        return 0;
}
