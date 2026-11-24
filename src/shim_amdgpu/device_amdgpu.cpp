// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

#include "device_amdgpu.h"
#include "shim_amdgpu.h"

#include "core/common/query_requests.h"
#include "core/pcie/linux/pcidev.h"

#include <drm/amdgpu_drm.h>
#include <drm/drm.h>
#include <fcntl.h>
#include <map>
#include <memory>
#include <stdexcept>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {

namespace query = xrt_core::query;
using key_type = query::key_type;

static std::shared_ptr<xrt_core::pci::dev>
get_pdev(const xrt_core::device* device)
{
  auto pdev = xrt_core::pci::get_dev(device->get_device_id(), device->is_userpf());
  if (!pdev)
    throw xrt_core::generic_error(EINVAL, "Invalid device handle");
  return pdev;
}

// sysfs_fcn: thin wrapper that calls pdev->sysfs_get() and surfaces errors
// as query::sysfs_error.  Specialized for std::string below.
template <typename ValueType>
struct sysfs_fcn
{
  static ValueType
  get(const std::shared_ptr<xrt_core::pci::dev>& dev, const char* subdev, const char* entry)
  {
    std::string err;
    ValueType value;
    dev->sysfs_get(subdev, entry, err, value, static_cast<ValueType>(-1));
    if (!err.empty())
      throw query::sysfs_error(err);
    return value;
  }
};

template <>
struct sysfs_fcn<std::string>
{
  static std::string
  get(const std::shared_ptr<xrt_core::pci::dev>& dev, const char* subdev, const char* entry)
  {
    std::string err;
    std::string value;
    dev->sysfs_get(subdev, entry, err, value);
    if (!err.empty())
      throw query::sysfs_error(err);
    return value;
  }
};

// Query implementation structs

struct bdf
{
  using result_type = query::pcie_bdf::result_type;

  static result_type
  get(const xrt_core::device* device, key_type)
  {
    auto pdev = get_pdev(device);
    return std::make_tuple(pdev->m_domain, pdev->m_bus, pdev->m_dev, pdev->m_func);
  }
};

struct pcie_id
{
  using result_type = query::pcie_id::result_type;

  static result_type
  get(const xrt_core::device* device, key_type)
  {
    auto pdev = get_pdev(device);
    result_type id;
    id.device_id = sysfs_fcn<uint16_t>::get(pdev, "", "device");
    id.revision_id = sysfs_fcn<uint8_t>::get(pdev, "", "revision");
    return id;
  }
};

struct pcie_device
{
  using result_type = query::pcie_device::result_type;

  static result_type
  get(const xrt_core::device* device, key_type key)
  {
    return pcie_id::get(device, key).device_id;
  }
};

struct instance
{
  using result_type = query::instance::result_type;

  static result_type
  get(const xrt_core::device* device, key_type)
  {
    return get_pdev(device)->m_instance;
  }
};

static std::string
amdgpu_family_string(uint32_t family)
{
  switch (family) {
  case AMDGPU_FAMILY_SI:
    return "Radeon (GFX6)";
  case AMDGPU_FAMILY_CI:
    return "Radeon (GFX7)";
  case AMDGPU_FAMILY_KV:
    return "Radeon (GFX7, Kaveri)";
  case AMDGPU_FAMILY_VI:
    return "Radeon (GFX8)";
  case AMDGPU_FAMILY_CZ:
    return "Radeon (GFX8, Carrizo)";
  case AMDGPU_FAMILY_AI:
    return "Radeon (GFX9)";
  case AMDGPU_FAMILY_RV:
    return "Radeon (GFX9, Raven)";
  case AMDGPU_FAMILY_NV:
    return "Radeon (GFX10, RDNA2)";
  case AMDGPU_FAMILY_VGH:
    return "Radeon (GFX10, Van Gogh)";
  case AMDGPU_FAMILY_GC_11_0_0:
  case AMDGPU_FAMILY_GC_11_0_1:
    return "Radeon (GFX11, RDNA3)";
  case AMDGPU_FAMILY_GC_11_5_0:
    return "Radeon (GFX11.5, RDNA3.5)";
  default:
    return "Radeon Graphics";
  }
}

struct rom_vbnv
{
  using result_type = query::rom_vbnv::result_type;

  static result_type
  get(const xrt_core::device* device, key_type)
  {
    auto pdev = get_pdev(device);
    auto node = std::string("/dev/dri/renderD") + std::to_string(pdev->m_instance);
    int fd = open(node.c_str(), O_RDWR | O_CLOEXEC);
    if (fd < 0)
      return "Radeon Graphics";

    drm_amdgpu_info_device dev_info = {};
    drm_amdgpu_info req = {};
    req.return_pointer = reinterpret_cast<uint64_t>(&dev_info);
    req.return_size = sizeof(dev_info);
    req.query = AMDGPU_INFO_DEV_INFO;

    int rc = ioctl(fd, DRM_IOCTL_AMDGPU_INFO, &req);
    close(fd);
    if (rc < 0)
      return "Radeon Graphics";

    return amdgpu_family_string(dev_info.family);
  }
};

struct default_value
{
  static std::any
  get(const xrt_core::device*, key_type key)
  {
    switch (key) {
    case key_type::device_class:
      return query::device_class::type::ryzen;
    case key_type::is_ready:
      return query::is_ready::result_type(true);
    default:
      throw query::no_such_key(key);
    }
  }
};

// sysfs_get: query::request subclass that reads a named sysfs attribute.
template <typename QueryRequestType>
struct sysfs_get : virtual QueryRequestType
{
  const char* subdev;
  const char* entry;

  sysfs_get(const char* s, const char* e)
    : subdev(s)
    , entry(e)
  {}

  std::any
  get(const xrt_core::device* device) const
  {
    return sysfs_fcn<typename QueryRequestType::result_type>
      ::get(get_pdev(device), subdev, entry);
  }
};

// function0_get: query::request subclass backed by a plain static getter.
template <typename QueryRequestType, typename Getter>
struct function0_get : virtual QueryRequestType
{
  using QueryRequestType::get;

  std::any
  get(const xrt_core::device* device) const
  {
    return Getter::get(device, QueryRequestType::key);
  }
};

static std::map<query::key_type, std::unique_ptr<query::request>> query_tbl;

template <typename QueryRequestType>
static void
emplace_sysfs_get(const char* subdev, const char* entry)
{
  query_tbl.emplace(QueryRequestType::key,
    std::make_unique<sysfs_get<QueryRequestType>>(subdev, entry));
}

template <typename QueryRequestType, typename Getter>
static void
emplace_func0_request()
{
  query_tbl.emplace(QueryRequestType::key,
    std::make_unique<function0_get<QueryRequestType, Getter>>());
}

static void
initialize_query_table()
{
  emplace_sysfs_get<query::pcie_vendor>              ("", "vendor");
  emplace_sysfs_get<query::pcie_subsystem_vendor>    ("", "subsystem_vendor");
  emplace_sysfs_get<query::pcie_subsystem_id>        ("", "subsystem_device");
  emplace_func0_request<query::pcie_bdf,             bdf>();
  emplace_func0_request<query::pcie_id,              pcie_id>();
  emplace_func0_request<query::pcie_device,          pcie_device>();
  emplace_func0_request<query::instance,             instance>();
  emplace_func0_request<query::rom_vbnv,             rom_vbnv>();
  emplace_func0_request<query::device_class,         default_value>();
  emplace_func0_request<query::is_ready,             default_value>();
}

struct X
{
  X() { initialize_query_table(); }
} x;

} // namespace

namespace shim_amdgpu {

device_amdgpu::
device_amdgpu(const pdev_amdgpu& pdev, handle_type shim_handle, id_type device_id)
  : noshim<xrt_core::device_pcie>{shim_handle, device_id, !pdev.m_is_mgmt}
  , m_pdev(pdev)
  , m_pcidev_handle(xrt_core::pci::get_dev(device_id, is_userpf()))
{}

device_amdgpu::
~device_amdgpu()
{}

const pdev_amdgpu&
device_amdgpu::
get_pdev() const
{
  return m_pdev;
}

void
device_amdgpu::
close_device()
{
  auto s = reinterpret_cast<shim*>(get_device_handle());
  if (s)
    delete s;
  // Deleting the shim drops the last reference to this device — do not
  // touch any member after this point.
}

const query::request&
device_amdgpu::
lookup_query(query::key_type query_key) const
{
  auto it = query_tbl.find(query_key);
  if (it == query_tbl.end())
    throw query::no_such_key(query_key);
  return *(it->second);
}

} // namespace shim_amdgpu
