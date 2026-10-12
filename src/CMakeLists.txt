# dalang_core: everything except the CLI and the interactive form, so tests can link it.
# dalang: the command-line executable.

# ---- Build info -------------------------------------------------------------
# Embeds templates.toml and the version into a generated header, so the binary works without
# any files next to it. Re-run configure when templates.toml changes.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/templates.toml")
file(READ "${PROJECT_SOURCE_DIR}/templates.toml" DALANG_TEMPLATES_TOML)
configure_file(dalang/build_info.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/generated/dalang/build_info.hpp" @ONLY)

# ---- Core library -----------------------------------------------------------
add_library(dalang_core STATIC)
target_sources(dalang_core
    PRIVATE
        dalang/generator.cpp
        dalang/git.cpp
        dalang/process.cpp
        dalang/templates.cpp
)
target_include_directories(dalang_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/generated
)
target_compile_features(dalang_core PUBLIC cxx_std_23)
# Error handling uses toml::parse_result instead of exceptions.
target_compile_definitions(dalang_core PRIVATE TOML_EXCEPTIONS=0)
target_link_libraries(dalang_core
    PUBLIC
        fmt::fmt
    PRIVATE
        reproc++
        tomlplusplus::tomlplusplus
)
configure_target(dalang_core)

# ---- Executable -------------------------------------------------------------
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
    PRIVATE
        main.cpp
        interactive.cpp
)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        dalang_core
        CLI11::CLI11
        ftxui::component
)
configure_target(${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

if(ENABLE_SANITIZERS)
  target_enable_sanitizers(dalang_core)
  target_enable_sanitizers(${PROJECT_NAME})
endif()
