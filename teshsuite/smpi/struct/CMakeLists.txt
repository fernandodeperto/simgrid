if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  add_executable(struct_test struct_test.c)
  target_link_libraries(struct_test simgrid)
endif()


set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/struct.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/struct_test.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}

  PARENT_SCOPE
  )
