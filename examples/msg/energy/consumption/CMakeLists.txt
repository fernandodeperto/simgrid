set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(energy_consumption energy_consumption.c)

### Add definitions for compile
target_link_libraries(energy_consumption simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/energy_consumption.tesh
  PARENT_SCOPE
  )

set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_consumption.xml
  PARENT_SCOPE
  )

set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/energy_consumption.c
  PARENT_SCOPE
  )

set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
