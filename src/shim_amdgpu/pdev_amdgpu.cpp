// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

#include "pdev_amdgpu.h"
#include "device_amdgpu.h"
#include "shim_amdgpu.h"

namespace shim_amdgpu {

pdev_amdgpu::
pdev_amdgpu(std::shared_ptr<const xrt_core::pci::drv> drv, const std::string& sysfs)
  : xrt_core::pci::dev(std::move(drv), sysfs)
{}

xrt_core::device::handle_type
pdev_amdgpu::
create_shim(xrt_core::device::id_type id) const
{
  auto s = new shim(id);
  return static_cast<xrt_core::device::handle_type>(s);
}

std::shared_ptr<xrt_core::device>
pdev_amdgpu::
create_device(xrt_core::device::handle_type handle, xrt_core::device::id_type id) const
{
  return std::make_shared<device_amdgpu>(*this, handle, id);
}

} // namespace shim_amdgpu
