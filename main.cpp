#include <atomic>
#include <charconv>
#include <chrono>
#include <iostream>
#include <span>
#include <thread>
#include <vector>

using namespace std;

unsigned exceptionsSqrt(span<double> values, unsigned repeat);
unsigned exceptionsFib(unsigned n, unsigned maxDepth);
unsigned leafResultSqrt(span<double> values, unsigned repeat);
unsigned leafResultFib(unsigned n, unsigned maxDepth);
unsigned herbceptionEmulationSqrt(span<double> values, unsigned repeat);
unsigned herbceptionEmulationFib(unsigned n, unsigned maxDepth);
unsigned herbceptionsSqrt(span<double> values, unsigned repeat);
unsigned herbceptionsFib(unsigned n, unsigned maxDepth);

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

// Perform one run with a certain error probability
static unsigned doTest(TestedFunctionSqrt func, unsigned errorRate, unsigned seed) {
   Random random(seed);

   // Prepare an array of values
   array<double, 100> values;
   values.fill(1);

   // Execute the function n times and measure the runtime
   auto start = std::chrono::steady_clock::now();
   constexpr unsigned repeat = 10000, innerRepeat = 10;
   unsigned result = 0;
   for (unsigned index = 0; index != repeat; ++index) {
      // Cause a failure with a certain probability
      if ((random() % 1000) < errorRate) values[10] = -1;

      // Call the function itself
      result += func(values, innerRepeat);

      // Reset the invalid entry
      values[10] = 1;
   }
   if (result > innerRepeat * repeat)
      cerr << "invalid result!" << endl;
   auto stop = std::chrono::steady_clock::now();

   return std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
};

// Perform one run with a certain error probability
static unsigned doTest(TestedFunctionFib func, unsigned errorRate, unsigned seed) {
   Random random(seed);

   // Execute the function n times and measure the runtime
   auto start = std::chrono::steady_clock::now();
   constexpr unsigned repeat = 10000;
   constexpr unsigned depth = 20, expected = 6765;
   unsigned result = 0;
   for (unsigned index = 0; index != repeat; ++index) {
      // Cause a failure with a certain probability
      unsigned maxDepth = depth + 1;
      if ((random() % 1000) < errorRate) maxDepth = depth - 1;

      // Call the function itself
      result += (func(depth, maxDepth) == expected);
   }
   if (!result)
      cerr << "invalid result!" << endl;
   auto stop = std::chrono::steady_clock::now();

   return std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
};

// Perform the test using n threads
template <class T>
static unsigned doTestMultithreaded(T func, unsigned errorRate, unsigned threadCount) {
   if (threadCount <= 1) return func(errorRate, 0);

   vector<thread> threads;
   atomic<unsigned> maxDuration{0};
   threads.reserve(threadCount);
   for (unsigned index = 0; index != threadCount; ++index) {
      threads.push_back(thread([index, func, errorRate, &maxDuration]() {
         unsigned duration = func(errorRate, index);
         unsigned current = maxDuration.load();
         while ((duration > current) && (!maxDuration.compare_exchange_weak(current, duration))) {}
      }));
   };
   for (auto& t : threads) t.join();
   return maxDuration.load();
}

static void runTests(const vector<tuple<const char*, TestedFunctionSqrt, TestedFunctionFib>>& tests, span<const unsigned> threadCounts) {
   auto announce = [threadCounts](const char* name) {
      cout << "testing " << name << " using";
      for (auto c : threadCounts) cout << " " << c;
      cout << " threads" << endl;
   };

   const unsigned failureRates[] = {0, 1, 10, 100};

   cout << "Testing unwinding performance: sqrt computation with occasional errors" << endl
        << endl;
   for (auto& t : tests) {
      announce(get<0>(t));
      for (unsigned fr : failureRates) {
         cout << "failure rate " << (static_cast<double>(fr) / 10.0) << "%:";
         for (auto tc : threadCounts)
            cout << " " << doTestMultithreaded([func = get<1>(t)](unsigned errorRate, unsigned id) { return doTest(func, errorRate, id); }, fr, tc);
         cout << endl;
      }
   }
   cout << endl;

   cout << "Testing invocation overhead: recursive fib with occasional errors" << endl
        << endl;
   for (auto& t : tests) {
      announce(get<0>(t));
      for (unsigned fr : failureRates) {
         cout << "failure rate " << (static_cast<double>(fr) / 10.0) << "%:";
         for (auto tc : threadCounts)
            cout << " " << doTestMultithreaded([func = get<2>(t)](unsigned errorRate, unsigned id) { return doTest(func, errorRate, id); }, fr, tc);
         cout << endl;
      }
   }
   cout << endl;
}

static vector<unsigned> buildThreadCounts(unsigned maxCount) {
   vector<unsigned> threadCounts{1};
   while (threadCounts.back() < maxCount) threadCounts.push_back(min(threadCounts.back() * 2, maxCount));
   return threadCounts;
}

static vector<unsigned> interpretThreadCounts(string_view desc) {
   vector<unsigned> threadCounts;
   auto add = [&](string_view desc) {
      unsigned c = 0;
      from_chars(desc.data(), desc.data() + desc.length(), c);
      if (c) threadCounts.push_back(c);
   };
   while (desc.find(' ') != string_view::npos) {
      auto split = desc.find(' ');
      add(desc.substr(0, split));
      desc = desc.substr(split + 1);
   }
   add(desc);
   return threadCounts;
}

vector<tuple<const char*, TestedFunctionSqrt, TestedFunctionFib>> tests = {{"exceptions", &exceptionsSqrt, &exceptionsFib}, {"LEAF", &leafResultSqrt, &leafResultFib}, {"herbceptionemulation", &herbceptionEmulationSqrt, &herbceptionEmulationFib}, {"herbceptions", &herbceptionsSqrt, &herbceptionsFib}};

int main(int argc, char* argv[]) {
   vector<unsigned> threadCounts = buildThreadCounts(thread::hardware_concurrency() / 2); // assuming half are hyperthreads. We can override that below
   bool explicitRun = false;
   for (int index = 1; index < argc; ++index) {
      string_view o = argv[index];
      if ((o == "--threads") && (index + 1 < argc)) {
         threadCounts = interpretThreadCounts(argv[++index]);
      } else {
         bool found = false;
         for (auto& t : tests)
            if (get<0>(t) == o) {
               runTests({t}, threadCounts);
               found = true;
               break;
            }
         if (!found) {
            cout << "unknown method " << o << endl;
            return 1;
         }
         explicitRun = true;
      }
   }
   if (!explicitRun) {
      runTests(tests, threadCounts);
   }
}
