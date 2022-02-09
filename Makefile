CXX?=g++

all: bin/runtests

bin/%.o: %.cpp
	@mkdir -p bin
	$(CXX) -O3 -std=c++20 -c -W -Wall $(CXXFLAGS-$(basename $@)) -o$@ $<

bin/runtests: bin/main.o bin/exceptions.o bin/leaf.o bin/expected.o bin/herbceptionemulation.o bin/herbceptions.o
	$(CXX) -o$@ $^

CXXFLAGS-bin/leaf:=-w -fno-exceptions -BOOST_LEAF_CFG_DIAGNOSTICS=0
CXXFLAGS-bin/herbceptionemulation:=-fno-exceptions
CXXFLAGS-bin/herbceptions:=-fno-exceptions


