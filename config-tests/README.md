Configuration tests
===================

Libpqxx comes with support for two different build systems: the GNU autotools,
and CMake.

We need to teach both of these to test things like "does this compiler
environment support `std::to_chars` for floating-point types?"

In both build systems we test these things by trying to compile a particular
snippet of code, found in this directry, and seeing whether that succeeds.

To avoid duplicating those snippets for multiple build systems, we put them
here.  Both the autotools configuration and the CMake configuration can refer to
them that way.  We generate autoconf and cmake configuration automatically to
inject those checks, avoiding tedious repetition.

Some of the checks are based on C++20 feature test macros.  We generate those
automatically using `tools/generate_cxx_checks.py`.

It took a bit of nasty magic to read a C++ source file into m4 for the autoconf
side and treat it as a string literal, without macro expansion.  There is every
chance that I missed something, so be prepared for tests failing for unexpected
reasons!  Some C++ syntax may end up having an unforeseen meaning in m4, and
screw up the handling of the code snippet.  Re-configure, and read your logs
carefully after editing these snippets.
