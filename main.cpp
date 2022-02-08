#include <atomic>
#include <charconv>
#include <chrono>
#include <iostream>
#include <span>
#include <thread>
#include <vector>

using namespace std;

unsigned exceptions(span<double> values, unsigned repeat);
unsigned leafResult(span<double> values, unsigned repeat);
unsigned herbceptionEmulation(span<double> values, unsigned repeat);

using TestedFunction = unsigned (*)(span<double>, unsigned);

// Perform one run with a certain error probability
static unsigned doTest(TestedFunction func, unsigned errorRate, unsigned seed) {
   // A weak but fast PRNG is good enough for this. Use xorshift.
   // We seed it with the thread it to get deterministic behavior
   auto random = [state = (seed << 1) | 1]() mutable {
      uint64_t x = state;
      x ^= x >> 12;
      x ^= x << 25;
      x ^= x >> 27;
      state = x;
      return x * 0x2545F4914F6CDD1DULL;
   };

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

// Perform the test using n threads
static unsigned doTestMultithreaded(TestedFunction func, unsigned errorRate, unsigned threadCount) {
   if (threadCount <= 1) return doTest(func, errorRate, 0);

   vector<thread> threads;
   atomic<unsigned> maxDuration{0};
   threads.reserve(threadCount);
   for (unsigned index = 0; index != threadCount; ++index) {
      threads.push_back(thread([index, func, errorRate, &maxDuration]() {
         unsigned duration = doTest(func, errorRate, index);
         unsigned current = maxDuration.load();
         while ((duration > current) && (!maxDuration.compare_exchange_weak(current, duration))) {}
      }));
   };
   for (auto& t : threads) t.join();
   return maxDuration.load();
}

static void runTests(const char* name, TestedFunction func, span<const unsigned> threadCounts) {
   cout << "testing " << name << " using";
   for (auto c : threadCounts) cout << " " << c;
   cout << " threads" << endl;

   const unsigned failureRates[] = {0, 1, 10, 100};
   for (unsigned fr : failureRates) {
      cout << "failure rate " << (static_cast<double>(fr) / 10.0) << "%:";
      for (auto tc : threadCounts)
         cout << " " << doTestMultithreaded(func, fr, tc);
      cout << endl;
   }
}

static vector<unsigned> buildThreadCounts(unsigned maxCount) {
   vector<unsigned> threadCounts{1};
   while(threadCounts.back() < maxCount) threadCounts.push_back(min(threadCounts.back() * 2, maxCount));
   return threadCounts;
}

static vector<unsigned> interpretThreadCounts(string_view desc) {
   vector<unsigned> threadCounts;
   auto add = [&](string_view desc) {
      unsigned c=0;
      from_chars(desc.data(), desc.data()+desc.length(),c);
      if (c) threadCounts.push_back(c);
   };
   while (desc.find(' ')!=string_view::npos) {
      auto split = desc.find(' ');
      add(desc.substr(0,split));
      desc=desc.substr(split+1);
   }
   add(desc);
   return threadCounts;
}

pair<const char*, TestedFunction> tests[] = {{"exceptions", &exceptions}, {"LEAF", &leafResult}, {"herbeceptionemulation", &herbceptionEmulation}};

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
            if (t.first == o) {
               runTests(t.first, t.second, threadCounts);
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
      for (auto& t : tests)
         runTests(t.first, t.second, threadCounts);
   }
}
