# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  [[CMakeFiles\voltro3_autogen.dir\AutogenUsed.txt]]
  [[CMakeFiles\voltro3_autogen.dir\ParseCache.txt]]
  [[CMakeFiles\voltro_runtime_autogen.dir\AutogenUsed.txt]]
  [[CMakeFiles\voltro_runtime_autogen.dir\ParseCache.txt]]
  "voltro3_autogen"
  "voltro_runtime_autogen"
  )
endif()
