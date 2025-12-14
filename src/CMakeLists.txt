add_library(poker_solver_core INTERFACE)
target_include_directories(poker_solver_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(poker_solver_core INTERFACE cxx_std_17)

add_executable(poker_solver_cli
  cli/main.cpp
)

target_compile_definitions(poker_solver_cli PRIVATE PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(poker_solver_cli PRIVATE poker_solver_core Threads::Threads)

if(MSVC)
  target_compile_options(poker_solver_cli PRIVATE /W4 /permissive- /WX-)
else()
  target_compile_options(poker_solver_cli PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

set_target_properties(poker_solver_cli PROPERTIES
  OUTPUT_NAME solver
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
