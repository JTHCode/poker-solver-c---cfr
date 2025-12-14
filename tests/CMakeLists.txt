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

add_executable(actions_tests
  actions_tests.cpp
)

target_link_libraries(actions_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(actions_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(actions_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

add_test(NAME actions_tests COMMAND actions_tests)

add_executable(tree_tests
  tree_tests.cpp
)

target_link_libraries(tree_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(tree_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(tree_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

add_test(NAME tree_tests COMMAND tree_tests)

add_executable(cfr_tests
  cfr_tests.cpp
)

target_link_libraries(cfr_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(cfr_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(cfr_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

add_test(NAME cfr_tests COMMAND cfr_tests)

add_executable(nlhe_cfr_tests
  nlhe_cfr_tests.cpp
)

target_link_libraries(nlhe_cfr_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(nlhe_cfr_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(nlhe_cfr_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

add_test(NAME nlhe_cfr_tests COMMAND nlhe_cfr_tests)

add_executable(scenario_tests
  scenario_tests.cpp
)

target_compile_definitions(scenario_tests PRIVATE PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(scenario_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(scenario_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(scenario_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

add_test(NAME scenario_tests COMMAND scenario_tests)

add_executable(jsonl_writer_tests
  jsonl_writer_tests.cpp
)

target_link_libraries(jsonl_writer_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(jsonl_writer_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(jsonl_writer_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

add_test(NAME jsonl_writer_tests COMMAND jsonl_writer_tests)

add_executable(subtree_expansion_tests
  subtree_expansion_tests.cpp
)

target_link_libraries(subtree_expansion_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(subtree_expansion_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(subtree_expansion_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

add_test(NAME subtree_expansion_tests COMMAND subtree_expansion_tests)

add_executable(dedupe_tests
  dedupe_tests.cpp
)

target_compile_definitions(dedupe_tests PRIVATE PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(dedupe_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(dedupe_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(dedupe_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

add_test(NAME dedupe_tests COMMAND dedupe_tests)
