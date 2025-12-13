add_executable(util_tests
  util_tests.cpp
)

target_link_libraries(util_tests PRIVATE poker_solver_core)

if(MSVC)
  target_compile_options(util_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(util_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

add_test(NAME util_tests COMMAND util_tests)
