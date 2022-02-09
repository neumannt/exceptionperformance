#include <cmath>
#include <exception>
#include <span>

static void doSqrt(std::span<double> values) noexcept {
   for (auto& v : values) {
      if (v < 0) std::terminate();
      v = sqrt(v);
   }
}

unsigned baselineSqrt(std::span<double> values, unsigned repeat) {
   unsigned failures = 0;
   for (unsigned index = 0; index != repeat; ++index)
      doSqrt(values);
   return failures;
}

static unsigned doFib(unsigned n, unsigned maxDepth) noexcept __attribute__((noinline, optimize("no-optimize-sibling-calls")));
static unsigned doFib(unsigned n, unsigned maxDepth) noexcept {
   if (!maxDepth) std::terminate();
   if (n <= 2) return 1;
   return doFib(n - 2, maxDepth - 1) + doFib(n - 1, maxDepth - 1);
}

unsigned baselineFib(unsigned n, unsigned maxDepth) {
   return doFib(n, maxDepth);
}
