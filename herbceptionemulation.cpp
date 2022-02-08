#include <cmath>
#include <span>
#include "thirdparty/tbv/tbv.hpp"

struct InvalidValue {};

static tbv::result<void> doSqrt(std::span<double> values) {
   for (auto& v : values) {
      if (v < 0) return tbv::throw_value(std::make_error_code(std::errc::argument_out_of_domain));
      v = sqrt(v);
   }
   return {};
}

unsigned herbceptionEmulation(std::span<double> values, unsigned repeat) {
   unsigned failures = 0;
   for (unsigned index = 0; index != repeat; ++index) {
      if (doSqrt(values).has_error()) ++failures;
   }
   return failures;
}
