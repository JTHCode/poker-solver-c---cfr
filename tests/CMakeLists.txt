add_executable(util_tests
  util_tests.cpp
)

target_link_libraries(util_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(util_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(util_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

add_test(NAME util_tests COMMAND util_tests)

add_executable(board_tests
  board_tests.cpp
)

target_link_libraries(board_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(board_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(board_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

add_test(NAME board_tests COMMAND board_tests)

add_executable(ranges_tests
  ranges_tests.cpp
)

target_compile_definitions(ranges_tests PRIVATE PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(ranges_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(ranges_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(ranges_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

add_test(NAME ranges_tests COMMAND ranges_tests)
