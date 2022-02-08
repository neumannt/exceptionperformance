#include <cmath>
#include <span>

struct InvalidValue {};

static void doSqrt(std::span<double> values) {
   for (auto& v : values) {
      if (v < 0) throw InvalidValue{};
      v = sqrt(v);
   }
}

unsigned exceptionsSqrt(std::span<double> values, unsigned repeat) {
   unsigned failures = 0;
   for (unsigned index = 0; index != repeat; ++index) {
      try {
         doSqrt(values);
      } catch (const InvalidValue& v) { ++failures; }
   }
   return failures;
}

 // prevent the compile from recognizing and compiling away the fib logic
static unsigned doFib(unsigned n, unsigned maxDepth) __attribute((noinline, optimize("-O1")));

static unsigned doFib(unsigned n, unsigned maxDepth) {
   if (!maxDepth) throw InvalidValue();
   if (n <= 2) return 1;
   return doFib(n - 2, maxDepth - 1) + doFib(n - 1, maxDepth - 1);
}

unsigned exceptionsFib(unsigned n, unsigned maxDepth) {
   try {
      return doFib(n, maxDepth);
   } catch (const InvalidValue&) { return 0; }
}
