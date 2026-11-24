// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

#ifndef PDEV_AMDGPU_H
#define PDEV_AMDGPU_H

#include "core/pcie/linux/pcidev.h"

namespace shim_amdgpu {

class pdev_amdgpu : public xrt_core::pci::dev
{
public:
  pdev_amdgpu(std::shared_ptr<const xrt_core::pci::drv> drv, const std::string& sysfs);

  xrt_core::device::handle_type
  create_shim(xrt_core::device::id_type id) const override;

  std::shared_ptr<xrt_core::device>
  create_device(xrt_core::device::handle_type handle, xrt_core::device::id_type id) const override;
};

} // namespace shim_amdgpu

#endif
