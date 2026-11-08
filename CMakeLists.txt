cmake_minimum_required(VERSION 3.20)
if(NOT TARGET ecaty_flecs_core)
  message(FATAL_ERROR "Configure the Unity project's Native directory, which composes Core and App.")
endif()
add_library(flecs_unity SHARED src/Application.cpp)
target_include_directories(flecs_unity PRIVATE include)
target_link_libraries(flecs_unity PRIVATE ecaty_flecs_core)
set_target_properties(flecs_unity PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
if(MSVC)
  set_property(TARGET flecs_unity PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  target_compile_options(flecs_unity PRIVATE /utf-8)
endif()
add_custom_command(TARGET flecs_unity POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${UNITY_PLUGIN_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:flecs_unity>" "${UNITY_PLUGIN_DIR}/$<TARGET_FILE_NAME:flecs_unity>")

if(MINGW)
  target_link_options(flecs_unity PRIVATE -static -static-libgcc -static-libstdc++)
endif()
