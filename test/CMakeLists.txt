# =============================================================================
# GTest Setup (shared by dalang_tests and the sanitizer trip-wires)
# =============================================================================
# find_package(GTest CONFIG REQUIRED) # need to add `gtest` in vcpkg.json
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
  SYSTEM)
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE) # don't let gtest's install rules pollute generated_lib/
FetchContent_MakeAvailable(googletest)

# surpress warnings from gtest libraries
foreach(t gtest gtest_main gmock gmock_main)
  if(TARGET ${t})
    target_compile_options(${t} PRIVATE -w)
  endif()
endforeach()

include(GoogleTest)
# Discover tests at ctest time, not build time: under Windows ASan the test exe needs a runtime
# DLL that's only deployed during the build, so a build-time test run can fail to start.
set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)

# =============================================================================
# dalang tests. The generator is tested with FakeRunner (test_utils.hpp) in place of git, plus a
# few tests that run real git against a local repository, skipped when git isn't installed.
# =============================================================================
if(BUILD_APP)
  add_executable(dalang_tests test_core.cpp test_generator.cpp)
  target_link_libraries(dalang_tests PRIVATE dalang_internal GTest::gmock GTest::gtest_main)
  if(ENABLE_SANITIZERS)
    target_enable_sanitizers(dalang_tests)
  endif()
  gtest_discover_tests(dalang_tests)
endif()

# =============================================================================
# Sanitizer trip-wires — verify each sanitizer actually CATCHES its bug class.
# Built only when ENABLE_SANITIZERS (so they inherit the sanitizer flags from
# cmake/sanitizer_analyzer.cmake). Each is a ctest test that PASSES when the
# sanitizer's diagnostic appears in the output — if a trip-wire runs clean, the
# sanitizer silently isn't working and the test goes red.
# =============================================================================
if(ENABLE_SANITIZERS)
  add_executable(sanitizer_tests sanitizers/sanitizer_tests.cpp)
  target_link_libraries(sanitizer_tests PRIVATE GTest::gtest GTest::gtest_main)
  # The trip-wires need the sanitizer flags themselves; also deploys the Windows ASan DLL.
  target_enable_sanitizers(sanitizer_tests)
  # Compile in only the death tests whose sanitizer is actually active in this build.
  if(MSVC)
    # Windows (MSVC/clang-cl): only ASan exists — no UBSan baseline, no LSan.
    if(ENABLE_ASAN)
      target_compile_definitions(sanitizer_tests PRIVATE SANITIZER_ASAN)
    endif()
  else()
    # GCC/Clang: UBSan is always the baseline, plus one of ASan(+LSan)/TSan/MSan.
    target_compile_definitions(sanitizer_tests PRIVATE SANITIZER_UBSAN)
    if(ENABLE_ASAN)
      target_compile_definitions(sanitizer_tests PRIVATE SANITIZER_ASAN SANITIZER_LSAN)
    elseif(ENABLE_TSAN)
      find_package(Threads REQUIRED)
      target_link_libraries(sanitizer_tests PRIVATE Threads::Threads)
      target_compile_definitions(sanitizer_tests PRIVATE SANITIZER_TSAN)
    elseif(ENABLE_MSAN)
      target_compile_definitions(sanitizer_tests PRIVATE SANITIZER_MSAN)
    endif()
  endif()
  # TSan/MSan default to report-and-continue; force termination on the first finding so the
  # EXPECT_DEATH tests observe a death. (ASan/UBSan already halt.)
  gtest_discover_tests(sanitizer_tests PROPERTIES
    ENVIRONMENT "TSAN_OPTIONS=halt_on_error=1;MSAN_OPTIONS=halt_on_error=1")
endif()
