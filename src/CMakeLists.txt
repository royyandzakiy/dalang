# Embeds templates.toml and the version into a generated header, so the binary needs no files
# next to it. Re-runs configure when templates.toml changes.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/templates.toml")
file(READ "${PROJECT_SOURCE_DIR}/templates.toml" DALANG_TEMPLATES_TOML)
configure_file(dalang/build_info.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/generated/dalang/build_info.hpp" @ONLY)

# Core code compiles once, here, as an OBJECT library. Linking it puts its objects into the
# executable and the unit tests; usage requirements are PUBLIC so both inherit them.
add_library(dalang_internal OBJECT dalang/core.cpp)

target_include_directories(
  dalang_internal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} # dalang/*.hpp
                         ${CMAKE_CURRENT_BINARY_DIR}/generated # dalang/build_info.hpp
)
target_compile_features(dalang_internal PUBLIC cxx_std_23)
target_compile_definitions(dalang_internal PUBLIC TOML_EXCEPTIONS=0) # errors via toml::parse_result
target_link_libraries(dalang_internal PUBLIC fmt::fmt reproc++ tomlplusplus::tomlplusplus)
configure_target(dalang_internal)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE dalang_internal CLI11::CLI11 ftxui::component)
configure_target(${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

if(ENABLE_SANITIZERS)
  target_enable_sanitizers(dalang_internal)
  target_enable_sanitizers(${PROJECT_NAME})
endif()
