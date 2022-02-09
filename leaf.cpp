#include "thirdparty/leaf/leaf.hpp"
#include <cmath>
#include <span>

namespace leaf = boost::leaf;

struct InvalidValue {};

static leaf::result<void> doSqrt(std::span<double> values) noexcept {
   for (auto& v : values) {
      if (v < 0) return leaf::new_error(InvalidValue{});
      v = sqrt(v);
   }
   return {};
}

unsigned leafResultSqrt(std::span<double> values, unsigned repeat) noexcept {
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

static leaf::result<unsigned> doFib(unsigned n, unsigned maxDepth) noexcept {
   if (!maxDepth) return leaf::new_error(InvalidValue{});
   if (n <= 2) return 1;
   BOOST_LEAF_AUTO(n2, doFib(n - 2, maxDepth - 1));
   BOOST_LEAF_AUTO(n1, doFib(n - 1, maxDepth - 1));
   return n2 + n1;
}

unsigned leafResultFib(unsigned n, unsigned maxDepth) noexcept {
   unsigned result = ~0u;
   leaf::try_handle_some([&]() -> leaf::result<void> {
         BOOST_LEAF_AUTO(v, doFib(n, maxDepth));
         result = v;
         return {}; },
                         [&](InvalidValue) {
                            result = 0;
                         });
   return result;
}
