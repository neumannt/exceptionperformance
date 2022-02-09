C++ exception performance experiment
------------------------------------

This project compares different ways
to implement exception handling in C++, namely:

- traditional C++ exceptions
- std::expected
- Boost::LEAF
- Herbceptions, using a pure C++ approximation
- Herbceptions, using inline assembly
- Boost::Outcome

It considers two scenarios. The sqrt scenario
perform a relatively expensive computation that
might throw from time to time. This primarily
stresses the unwinder. The fib scenario perform
thousands of recursive calls, and thus measures
the calling overhead of the different approaches.

