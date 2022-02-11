CXX?=g++

all: bin/runtests bin/runtests_googlebench

bin/%.o: %.cpp
	@mkdir -p bin
	$(CXX) -O3 -std=c++20 -c -W -Wall $(CXXFLAGS-$(basename $@)) -o$@ $<

bin/runtests: bin/main.o bin/exceptions.o bin/leaf.o bin/expected.o bin/herbceptionemulation.o bin/herbceptions.o bin/outcome.o bin/baseline.o
	$(CXX) -o$@ $^

bin/benchmark/src/libbenchmark.a:
	@mkdir -p bin/benchmark
	cmake -E chdir bin/benchmark cmake -DBENCHMARK_ENABLE_TESTING=OFF -DBENCHMARK_ENABLE_EXCEPTIONS=OFF -DCMAKE_BUILD_TYPE=Release ../../thirdparty/benchmark
	cmake --build bin/benchmark --config Release --target benchmark

bin/runtests_googlebench: bin/main_googlebench.o bin/exceptions.o bin/leaf.o bin/expected.o bin/herbceptionemulation.o bin/herbceptions.o bin/outcome.o bin/baseline.o bin/benchmark/src/libbenchmark.a
	$(CXX) -o$@ $^ $(LDFLAGS-$(basename $@))

CXXFLAGS-bin/leaf:=-w -fno-exceptions -O2 -DNDEBUG -DBOOST_LEAF_CFG_DIAGNOSTICS=0 -DBOOST_LEAF_CFG_CAPTURE=0
CXXFLAGS-bin/herbceptionemulation:=-fno-exceptions
CXXFLAGS-bin/herbceptions:=-fno-exceptions
CXXFLAGS-bin/outcome:=-fno-exceptions
CXXFLAGS-bin/baseline:=-fno-exceptions
CXXFLAGS-bin/main_googlebench:=-Ithirdparty/benchmark/include
LDFLAGS-bin/runtests_googlebench:=-Lbin/benchmark/src -lbenchmark
