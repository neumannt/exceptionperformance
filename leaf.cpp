#include "thirdparty/leaf/leaf.hpp"
#include <cmath>
#include <span>

namespace leaf = boost::leaf;

struct InvalidValue {};

static leaf::result<void> doSqrt(std::span<double> values) {
   for (auto& v : values) {
      if (v < 0) return leaf::new_error(InvalidValue{});
      v = sqrt(v);
   }
   return {};
}

unsigned leafResult(std::span<double> values, unsigned repeat) {
   unsigned failures = 0;
   for (unsigned index = 0; index != repeat; ++index) {
      leaf::try_handle_some([&]() -> leaf::result<void> {
         BOOST_LEAF_CHECK(doSqrt(values));
         return {}; },
                            [&](InvalidValue) {
                               ++failures;
                            });
   }
   return failures;
}
