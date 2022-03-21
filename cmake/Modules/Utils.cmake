function(target_common TARGET)
  target_include_directories(${TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
  )
  set_target_properties(${TARGET} PROPERTIES LINKER_LANGUAGE CXX)
endfunction()

function(add_install TARGET)
  add_custom_target(
    ${TARGET}-install
    DEPENDS ${ARGN}
    COMMAND  "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=${TARGET}
        -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
  )

endfunction()

function(target_install TARGET DESTINATION)
  cmake_parse_arguments(ARG "" "" "INSTALL_DEPS" ${ARGN})
  install(
    TARGETS ${TARGET}
    DESTINATION ${DESTINATION}
    COMPONENT ${TARGET}
    )

  add_install(${TARGET} ${TARGET} ${ARG_INSTALL_DEPS})
endfunction()


function(target_executable TARGET)
  cmake_parse_arguments(ARG "" "" "INSTALL_DEPS" ${ARGN})

  target_common(${TARGET})
  target_install(${TARGET} bin INSTALL_DEPS ${ARG_INSTALL_DEPS})

endfunction()

# for dlopened stuff...
function(target_module TARGET)
  target_common(${TARGET})
  target_install(${TARGET} lib64 INSTALL_DEPS ${ARG_INSTALL_DEPS})
endfunction()
