// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

#include "drv_amdgpu.h"
#include "pdev_amdgpu.h"
#include "core/pcie/linux/system_linux.h"

#include <dirent.h>
#include <stdexcept>
#include <string>

namespace {

// Scan BDF's DRM sysfs directory for a renderD node and return its
// instance number (e.g. renderD128 → 128).  Throws std::invalid_argument
// when no matching node is found so that scan_devices skips the device.
static uint32_t
find_render_instance(const std::string& sysfs_name)
{
  const std::string drm_dir = "/sys/bus/pci/devices/" + sysfs_name + "/drm";
  static constexpr const char* prefix = "renderD";
  static constexpr size_t prefix_len = 7; // strlen("renderD")

  auto dp = opendir(drm_dir.c_str());
  if (!dp)
    throw std::invalid_argument("No DRM sysfs dir for " + sysfs_name);

  std::string node;
  while (auto entry = readdir(dp)) {
    std::string d{entry->d_name};
    if (d.compare(0, prefix_len, prefix) == 0) {
      node = std::move(d);
      break;
    }
  }
  closedir(dp);

  if (node.empty())
    throw std::invalid_argument("No renderD node for " + sysfs_name);

  return static_cast<uint32_t>(std::stoul(node.substr(prefix_len)));
}

struct X
{
  X() { xrt_core::pci::register_driver(std::make_shared<shim_amdgpu::drv_amdgpu>()); }
} x;

} // namespace

namespace shim_amdgpu {

std::string
drv_amdgpu::
name() const
{
  return "amdgpu";
}

bool
drv_amdgpu::
is_user() const
{
  return true;
}

std::string
drv_amdgpu::
dev_node_prefix() const
{
  return "renderD";
}

std::string
drv_amdgpu::
dev_node_dir() const
{
  return "dri";
}

std::string
drv_amdgpu::
sysfs_dev_node_dir() const
{
  return "drm";
}

std::shared_ptr<xrt_core::pci::dev>
drv_amdgpu::
create_pcidev(const std::string& sysfs) const
{
  auto pdev = std::make_shared<pdev_amdgpu>(shared_from_this(), sysfs);
  pdev->m_instance = find_render_instance(sysfs);
  pdev->m_is_ready = true;
  return pdev;
}

} // namespace shim_amdgpu
