dnl config.m4 for extension ason

PHP_ARG_ENABLE([ason],
  [whether to enable ason support],
  [AS_HELP_STRING([--enable-ason],
    [Enable ason support])],
  [no])

if test "$PHP_ASON" != "no"; then
  dnl Check for C++ compiler
  PHP_REQUIRE_CXX()

  dnl Add C++17 flags
  CXXFLAGS="$CXXFLAGS -std=c++17 -O3 -DNDEBUG"

  dnl Detect architecture and add SIMD optimization flags
  case $host_alias in
    *x86_64* | *amd64* )
      CXXFLAGS="$CXXFLAGS -march=native -msse2 -mavx2"
      ;;
    *aarch64* | *arm64* )
      CXXFLAGS="$CXXFLAGS -march=native"
      ;;
    * )
      dnl Fallback for other architectures, no specific SIMD flags
      ;;
  esac

  PHP_ADD_LIBRARY(stdc++, 1, ASON_SHARED_LIBADD)
  PHP_SUBST(ASON_SHARED_LIBADD)

  PHP_NEW_EXTENSION(ason, ason_php.cpp, $ext_shared,, $CXXFLAGS)

  PHP_ADD_MAKEFILE_FRAGMENT
fi
