cmake_minimum_required(VERSION 3.20)
if(NOT TARGET flecs_unity_core)
  message(FATAL_ERROR "Configure the Unity project's Native directory, which composes Core and App.")
endif()
add_library(flecs_unity SHARED src/Application.cpp)
target_include_directories(flecs_unity PRIVATE include)
target_link_libraries(flecs_unity PRIVATE flecs_unity_core)
set_target_properties(flecs_unity PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
if(WIN32)
  # MinGW would name it libflecs_unity.dll; Unity and the smokes load flecs_unity.dll.
  set_target_properties(flecs_unity PROPERTIES PREFIX "")
endif()
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

if(ANDROID)
  # Ninja on Windows shortens the NDK path (it contains spaces) and the short clang++ name
  # links in C mode, so name the static C++ runtime explicitly.
  target_link_libraries(flecs_unity PRIVATE c++_static c++abi)
endif()
