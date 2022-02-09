#include "thirdparty/expected/Expected.h"
#include <cmath>
#include <span>

using namespace std::experimental;

struct InvalidValue {};

static expected<void, InvalidValue> doSqrt(std::span<double> values) {
   for (auto& v : values) {
      if (v < 0) return unexpected<InvalidValue>(InvalidValue{});
      v = sqrt(v);
   }
   return {};
}

unsigned expectedSqrt(std::span<double> values, unsigned repeat) {
   unsigned failures = 0;
   for (unsigned index = 0; index != repeat; ++index) {
      if (!doSqrt(values)) ++failures;
   }
   return failures;
}

static expected<unsigned, InvalidValue> doFib(unsigned n, unsigned maxDepth) __attribute((noinline, optimize("no-optimize-sibling-calls")));
static expected<unsigned, InvalidValue> doFib(unsigned n, unsigned maxDepth) {
   if (!maxDepth) return unexpected<InvalidValue>(InvalidValue{});
   if (n <= 2) return 1;
   auto n2 = doFib(n - 2, maxDepth - 1);
   if (!n2) return unexpected<InvalidValue>(n2.error());
   auto n1 = doFib(n - 1, maxDepth - 1);
   if (!n1) return unexpected<InvalidValue>(n1.error());
   return n2.value() + n1.value();
}

unsigned expectedFib(unsigned n, unsigned maxDepth) {
   auto r = doFib(n, maxDepth);
   if (!r) return 0;
   return r.value();
}
