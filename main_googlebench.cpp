#include <span>
#include <thread>
#include <benchmark/benchmark.h>

using namespace std;

unsigned baselineSqrt(span<double> values, unsigned repeat);
unsigned baselineFib(unsigned n, unsigned maxDepth);
unsigned exceptionsSqrt(span<double> values, unsigned repeat);
unsigned exceptionsFib(unsigned n, unsigned maxDepth);
unsigned leafResultSqrt(span<double> values, unsigned repeat) noexcept;
unsigned leafResultFib(unsigned n, unsigned maxDepth) noexcept;
unsigned expectedSqrt(span<double> values, unsigned repeat) noexcept;
unsigned expectedFib(unsigned n, unsigned maxDepth) noexcept;
unsigned herbceptionEmulationSqrt(span<double> values, unsigned repeat) noexcept;
unsigned herbceptionEmulationFib(unsigned n, unsigned maxDepth) noexcept;
unsigned herbceptionsSqrt(span<double> values, unsigned repeat) noexcept;
unsigned herbceptionsFib(unsigned n, unsigned maxDepth) noexcept;
unsigned outcomeResultSqrt(span<double> values, unsigned repeat) noexcept;
unsigned outcomeResultFib(unsigned n, unsigned maxDepth) noexcept;

using TestedFunctionSqrt = unsigned (*)(span<double>, unsigned);
using TestedFunctionFib = unsigned (*)(unsigned, unsigned);

// A weak but fast PRNG is good enough for this. Use xorshift.
// We seed it with the thread id to get deterministic behavior
struct Random {
   uint64_t state;
   Random(uint64_t seed) : state((seed << 1) | 1) {}

   uint64_t operator()() {
      uint64_t x = state;
      x ^= x >> 12;
      x ^= x << 25;
      x ^= x >> 27;
      state = x;
      return x * 0x2545F4914F6CDD1DULL;
   }
};

static void BM_sqrt(benchmark::State& state, TestedFunctionSqrt func) {
   constexpr unsigned repeat = 10000;
   constexpr unsigned innerRepeat = 10;

   // Prepare an array of values
   array<double, 100> values;
   values.fill(1);

   Random random(state.thread_index());
   unsigned errorRate = state.range(0);

   double realTimeElapsed = 0.0;
   for (auto _ : state) {
      unsigned result = 0;
      auto start = std::chrono::steady_clock::now();
      for (unsigned index = 0; index != repeat; ++index) {
         // Cause a failure with a certain probability
         if ((random() % 1000) < errorRate) values[10] = -1;

         // Call the function itself
         result += func(values, innerRepeat);

         // Reset the invalid entry
         values[10] = 1;
      }
      auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start).count();
      realTimeElapsed += elapsed;

      state.SetIterationTime(elapsed);
      if (result > (innerRepeat * repeat))
         state.SkipWithError("invalid result!");
   }

   // Report the elapsed real time separately to ease parsing benchmark results. This value is exactly equivalent to threads * Time
   state.counters["real_time_elapsed"] = benchmark::Counter(realTimeElapsed * 1000, benchmark::Counter::kAvgIterations);
}

static void BM_fib(benchmark::State& state, TestedFunctionFib func) {
   constexpr unsigned repeat = 10000;
   constexpr unsigned depth = 15, expected = 610;

   Random random(state.thread_index());
   unsigned errorRate = state.range(0);

   double realTimeElapsed = 0.0;
   for (auto _ : state) {
      unsigned result = 0;
      auto start = std::chrono::steady_clock::now();
      for (unsigned index = 0; index != repeat; ++index) {
         // Cause a failure with a certain probability
         unsigned maxDepth = depth + 1;
         if ((random() % 1000) < errorRate) maxDepth = depth - 2;

         // Call the function itself
         result += (func(depth, maxDepth) == expected);
      }
      auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start).count();
      realTimeElapsed += elapsed;

      state.SetIterationTime(elapsed);
      if (!result)
         state.SkipWithError("invalid result!");
   }

   // Report the elapsed real time separately to ease parsing benchmark results. This value is exactly equivalent to threads * Time
   state.counters["real_time_elapsed"] = benchmark::Counter(realTimeElapsed * 1000, benchmark::Counter::kAvgIterations);
}

int main(int argc, char** argv) {
   auto configureBenchmark = [](auto* benchmark, const auto& failureRates) {
      benchmark->ThreadRange(1, thread::hardware_concurrency() / 2);
      benchmark->UseManualTime();
      benchmark->Unit(benchmark::kMillisecond);
      for (auto failureRate : failureRates)
         benchmark->Arg(failureRate);
   };

   constexpr array<unsigned, 4> failureRates = {0, 1, 10, 100};
   constexpr array<tuple<const char*, TestedFunctionSqrt, TestedFunctionFib>, 6> tests = {
      tuple{"exceptions", &exceptionsSqrt, &exceptionsFib},
      tuple{"LEAF", &leafResultSqrt, &leafResultFib},
      tuple{"std::expected", &expectedSqrt, &expectedFib},
      tuple{"herbceptionemulation", &herbceptionEmulationSqrt, &herbceptionEmulationFib},
      tuple{"herbceptions", &herbceptionsSqrt, &herbceptionsFib},
      tuple{"outcome", &outcomeResultSqrt, &outcomeResultFib}};

   configureBenchmark(benchmark::RegisterBenchmark("SQRT_baseline", BM_sqrt, &baselineSqrt), array<unsigned, 1>{0});
   for (auto test : tests) {
      string name = string("SQRT_") + get<0>(test);
      configureBenchmark(benchmark::RegisterBenchmark(name.c_str(), BM_sqrt, get<1>(test)), failureRates);
   }

   configureBenchmark(benchmark::RegisterBenchmark("FIB_baseline", BM_fib, &baselineFib), array<unsigned, 1>{0});
   for (auto test : tests) {
      string name = string("FIB_") + get<0>(test);
      configureBenchmark(benchmark::RegisterBenchmark(name.c_str(), BM_fib, get<2>(test)), failureRates);
   }

   benchmark::Initialize(&argc, argv);
   benchmark::RunSpecifiedBenchmarks();
}