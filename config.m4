dnl config.m4 for extension asun

PHP_ARG_ENABLE([asun],
  [whether to enable asun support],
  [AS_HELP_STRING([--enable-asun],
    [Enable asun support])],
  [no])

if test "$PHP_ASUN" != "no"; then
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

  PHP_ADD_LIBRARY(stdc++, 1, ASUN_SHARED_LIBADD)
  PHP_SUBST(ASUN_SHARED_LIBADD)

  PHP_NEW_EXTENSION(asun, asun_php.cpp, $ext_shared,, $CXXFLAGS)

  PHP_ADD_MAKEFILE_FRAGMENT
fi
