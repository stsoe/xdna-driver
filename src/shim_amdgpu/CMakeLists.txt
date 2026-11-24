# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

set(AMDGPU_TARGET xrt_driver_amdgpu)
set(XRT_CORE_TARGET xrt_core)
set(XRT_COREUTIL_TARGET xrt_coreutil)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} AMDGPU_SOURCES)

add_library(${AMDGPU_TARGET} SHARED ${AMDGPU_SOURCES})

set_target_properties(${AMDGPU_TARGET} PROPERTIES
  VERSION ${XRT_PLUGIN_VERSION_STRING}
  SOVERSION ${XRT_SOVERSION}
  )

target_compile_definitions(${AMDGPU_TARGET} PRIVATE
  # required so that i/f defined in ishim.h is consistent with native xrt implementation
  XRT_ENABLE_AIE
  XRT_AIE_BUILD
  XRT_BUILD
  XRT_ENABLE_USDT
  $<$<CONFIG:Debug>:XDNA_SHIM_DEBUG>
  )

target_compile_options(${AMDGPU_TARGET} PRIVATE
  "-fPIC"
  "-Wall"
  "-Werror"
  )

target_include_directories(${AMDGPU_TARGET} PRIVATE
  ${XRT_SOURCE_DIR}/runtime_src
  ${XRT_SOURCE_DIR}/runtime_src/core/include
  ${XRT_SOURCE_DIR}/runtime_src/core/common/gsl/include
  ${XRT_BINARY_DIR}/gen
  )

target_link_libraries(${AMDGPU_TARGET} PRIVATE
  xrt_core
  ${XRT_CORE_TARGET}
  ${XRT_COREUTIL_TARGET}
  )

target_link_options(${AMDGPU_TARGET} PRIVATE
  "-Wl,-z,defs"
  )

install(TARGETS ${AMDGPU_TARGET}
  LIBRARY DESTINATION ${XDNA_PKG_LIB_DIR}
  NAMELINK_SKIP
  COMPONENT ${XDNA_COMPONENT}
  )

install(TARGETS ${AMDGPU_TARGET} DESTINATION ${XDNA_BIN_DIR}/${XDNA_PKG_LIB_DIR})
