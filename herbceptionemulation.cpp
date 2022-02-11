#include "thirdparty/tbv/tbv.hpp"
#include <cmath>
#include <span>

struct InvalidValue {};

static tbv::result<void> doSqrt(std::span<double> values) noexcept __attribute__((noinline));
static tbv::result<void> doSqrt(std::span<double> values) noexcept {
   for (auto& v : values) {
      if (v < 0) return tbv::throw_value(std::make_error_code(std::errc::argument_out_of_domain));
      v = sqrt(v);
   }
   return {};
}

unsigned herbceptionEmulationSqrt(std::span<double> values, unsigned repeat) noexcept {
   unsigned failures = 0;
   for (unsigned index = 0; index != repeat; ++index) {
      if (doSqrt(values).has_error()) ++failures;
   }
   return failures;
}

static tbv::result<unsigned> doFib(unsigned n, unsigned maxDepth) noexcept __attribute((noinline, optimize("no-optimize-sibling-calls")));
static tbv::result<unsigned> doFib(unsigned n, unsigned maxDepth) noexcept {
   if (!maxDepth) return tbv::throw_value(std::make_error_code(std::errc::argument_out_of_domain));
   if (n <= 2) return 1;
   return TRY(doFib(n - 2, maxDepth - 1)) + TRY(doFib(n - 1, maxDepth - 1));
}

unsigned herbceptionEmulationFib(unsigned n, unsigned maxDepth) noexcept {
   auto v = doFib(n, maxDepth);
   return v.has_error() ? 0 : v.value();
}
