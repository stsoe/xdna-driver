// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

#ifndef SHIM_AMDGPU_H
#define SHIM_AMDGPU_H

#include "core/common/system.h"

namespace shim_amdgpu {

class shim
{
public:
  shim(xrt_core::device::id_type id) : m_device(xrt_core::get_userpf_device(this, id))
  {}

  ~shim()
  {}

private:
  std::shared_ptr<xrt_core::device> m_device;
};

} // namespace shim_amdgpu

#endif
