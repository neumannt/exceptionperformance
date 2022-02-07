#include <cmath>
#include <span>

struct InvalidValue {};

static void doSqrt(std::span<double> values) {
   for (auto& v : values) {
      if (v < 0) throw InvalidValue{};
      v = sqrt(v);
   }
}

unsigned exceptions(std::span<double> values, unsigned repeat) {
   unsigned failures = 0;
   for (unsigned index = 0; index != repeat; ++index) {
      try {
         doSqrt(values);
      } catch (const InvalidValue& v) { ++failures; }
   }
   return failures;
}
