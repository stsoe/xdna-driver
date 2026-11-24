// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

#ifndef DRV_AMDGPU_H
#define DRV_AMDGPU_H

#include "core/pcie/linux/pcidrv.h"

namespace shim_amdgpu {

class drv_amdgpu : public xrt_core::pci::drv
{
public:
  std::string
  name() const override;

  bool
  is_user() const override;

  std::string
  dev_node_prefix() const override;

  std::string
  dev_node_dir() const override;

  std::string
  sysfs_dev_node_dir() const override;

  std::shared_ptr<xrt_core::pci::dev>
  create_pcidev(const std::string& sysfs) const override;
};

} // namespace shim_amdgpu

#endif
