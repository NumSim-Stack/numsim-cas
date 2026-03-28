set(_config_args)
if(DEFINED TEST_CONFIG AND NOT TEST_CONFIG STREQUAL "")
  list(APPEND _config_args --config "${TEST_CONFIG}")
endif()

set(_install_prefix "${NUMSIM_CAS_BINARY_DIR}/package_smoke/install")
set(_consumer_build_dir "${NUMSIM_CAS_BINARY_DIR}/package_smoke/build")
set(_consumer_source_dir "${NUMSIM_CAS_SOURCE_DIR}/tests/package_smoke")

file(REMOVE_RECURSE "${_install_prefix}" "${_consumer_build_dir}")
file(MAKE_DIRECTORY "${_install_prefix}" "${_consumer_build_dir}")

execute_process(
  COMMAND "${CMAKE_COMMAND}" --install "${NUMSIM_CAS_BINARY_DIR}" --prefix "${_install_prefix}" ${_config_args}
  RESULT_VARIABLE _install_result
)
if(NOT _install_result EQUAL 0)
  message(FATAL_ERROR "Package smoke test failed during install step")
endif()

set(_configure_args
  -S "${_consumer_source_dir}"
  -B "${_consumer_build_dir}"
  "-DCMAKE_PREFIX_PATH=${_install_prefix}"
)
if(DEFINED TEST_CONFIG AND NOT TEST_CONFIG STREQUAL "")
  list(APPEND _configure_args "-DCMAKE_BUILD_TYPE=${TEST_CONFIG}")
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" ${_configure_args}
  RESULT_VARIABLE _configure_result
)
if(NOT _configure_result EQUAL 0)
  message(FATAL_ERROR "Package smoke test failed during downstream configure")
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${_consumer_build_dir}" ${_config_args}
  RESULT_VARIABLE _build_result
)
if(NOT _build_result EQUAL 0)
  message(FATAL_ERROR "Package smoke test failed during downstream build")
endif()

set(_consumer_exe "${_consumer_build_dir}/numsim_cas_package_smoke")
if(DEFINED TEST_CONFIG AND NOT TEST_CONFIG STREQUAL "")
  if(EXISTS "${_consumer_build_dir}/${TEST_CONFIG}/numsim_cas_package_smoke"
     OR EXISTS "${_consumer_build_dir}/${TEST_CONFIG}/numsim_cas_package_smoke.exe")
    set(_consumer_exe "${_consumer_build_dir}/${TEST_CONFIG}/numsim_cas_package_smoke")
  endif()
endif()
if(CMAKE_HOST_WIN32)
  string(APPEND _consumer_exe ".exe")
endif()

execute_process(
  COMMAND "${_consumer_exe}"
  RESULT_VARIABLE _run_result
)
if(NOT _run_result EQUAL 0)
  message(FATAL_ERROR "Package smoke test failed during downstream run")
endif()
