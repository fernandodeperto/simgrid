### Make Libs

add_library(simgrid SHARED ${simgrid_sources})

if(MSVC)
  set_target_properties(simgrid  PROPERTIES 
       COMPILE_FLAGS "/D_XBT_DLL_EXPORT /DDLL_EXPORT" 
       VERSION ${libsimgrid_version} )
else()
  set_target_properties(simgrid  PROPERTIES 
       COMPILE_FLAGS "-D_XBT_DLL_EXPORT -DDLL_EXPORT" 
       LINK_FLAGS "-shared" 
       VERSION ${libsimgrid_version} 
       PREFIX "lib" SUFFIX ".dll" 
       IMPORT_PREFIX "lib" IMPORT_SUFFIX ".dll")

  set(SIMGRID_DEP "-lm")

  if (HAVE_PTHREAD)
    set(SIMGRID_DEP "${SIMGRID_DEP} -lpthread")
  endif()
endif()

target_link_libraries(simgrid 	${SIMGRID_DEP})

