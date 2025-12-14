add_executable(util_tests
  util_tests.cpp
)

target_link_libraries(util_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(util_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(util_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG)
endif()

add_test(NAME util_tests COMMAND util_tests)

add_executable(board_tests
  board_tests.cpp
)

target_link_libraries(board_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(board_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(board_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG)
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
  target_compile_options(ranges_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG)
endif()

add_test(NAME ranges_tests COMMAND ranges_tests)

add_executable(actions_tests
  actions_tests.cpp
)

target_link_libraries(actions_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(actions_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(actions_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG)
endif()

add_test(NAME actions_tests COMMAND actions_tests)

add_executable(tree_tests
  tree_tests.cpp
)

target_link_libraries(tree_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(tree_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(tree_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG)
endif()

add_test(NAME tree_tests COMMAND tree_tests)

add_executable(tree_builder_tests
  tree_builder_tests.cpp
)

target_link_libraries(tree_builder_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(tree_builder_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(tree_builder_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG)
endif()

add_test(NAME tree_builder_tests COMMAND tree_builder_tests)

add_executable(kuhn_metrics_tests
  kuhn_metrics_tests.cpp
)

target_link_libraries(kuhn_metrics_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(kuhn_metrics_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(kuhn_metrics_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG)
endif()

add_test(NAME kuhn_metrics_tests COMMAND kuhn_metrics_tests)

add_executable(nlhe_metrics_tests
  nlhe_metrics_tests.cpp
)

target_link_libraries(nlhe_metrics_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(nlhe_metrics_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(nlhe_metrics_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG)
endif()

add_test(NAME nlhe_metrics_tests COMMAND nlhe_metrics_tests)

add_executable(quality_report
  quality_report.cpp
)

target_link_libraries(quality_report PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(quality_report PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(quality_report PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG)
endif()

add_executable(cfr_tests
  cfr_tests.cpp
)

target_link_libraries(cfr_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(cfr_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(cfr_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG)
endif()

add_test(NAME cfr_tests COMMAND cfr_tests)

add_executable(nlhe_cfr_tests
  nlhe_cfr_tests.cpp
)

target_link_libraries(nlhe_cfr_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(nlhe_cfr_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(nlhe_cfr_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG)
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
  target_compile_options(scenario_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG)
endif()

add_test(NAME scenario_tests COMMAND scenario_tests)

add_executable(jsonl_writer_tests
  jsonl_writer_tests.cpp
)

target_link_libraries(jsonl_writer_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(jsonl_writer_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(jsonl_writer_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG)
endif()

add_test(NAME jsonl_writer_tests COMMAND jsonl_writer_tests)

add_executable(subtree_expansion_tests
  subtree_expansion_tests.cpp
)

target_link_libraries(subtree_expansion_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(subtree_expansion_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(subtree_expansion_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG)
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
  target_compile_options(dedupe_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG)
endif()

add_test(NAME dedupe_tests COMMAND dedupe_tests)

add_executable(hand_evaluator_tests
  hand_evaluator_tests.cpp
)

target_link_libraries(hand_evaluator_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(hand_evaluator_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(hand_evaluator_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG)
endif()

add_test(NAME hand_evaluator_tests COMMAND hand_evaluator_tests)

add_executable(cli_smoke_tests
  cli_smoke_tests.cpp
)

target_compile_definitions(cli_smoke_tests PRIVATE SOLVER_PATH="${CMAKE_BINARY_DIR}/bin/solver")
target_link_libraries(cli_smoke_tests PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(cli_smoke_tests PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(cli_smoke_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG)
endif()

add_test(NAME cli_smoke_tests COMMAND cli_smoke_tests)
