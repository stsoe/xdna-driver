// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

#ifndef DEVICE_AMDGPU_H
#define DEVICE_AMDGPU_H

#include "pdev_amdgpu.h"
#include "core/common/ishim.h"
#include "core/common/shim/buffer_handle.h"
#include "core/common/shim/hwctx_handle.h"
#include "core/pcie/linux/pcidev.h"
#include "xrt/xrt_hw_context.h"
#include "xrt/xrt_uuid.h"

namespace shim_amdgpu {

class device_amdgpu : public xrt_core::noshim<xrt_core::device_pcie>
{
  const pdev_amdgpu& m_pdev;
  // Keep pdev alive for the lifetime of this device object.
  std::shared_ptr<xrt_core::pci::dev> m_pcidev_handle;

  const xrt_core::query::request&
  lookup_query(xrt_core::query::key_type query_key) const override;

public:
  device_amdgpu(const pdev_amdgpu& pdev, handle_type shim_handle, id_type device_id);
  ~device_amdgpu();

  const pdev_amdgpu&
  get_pdev() const;

  void
  close_device() override;

  // noshim does not stub these new-API pure-virtuals; throw explicitly.
  std::unique_ptr<xrt_core::buffer_handle>
  alloc_bo(size_t size, uint64_t flags) override
  { throw xrt_core::ishim::not_supported_error(__func__); }

  std::unique_ptr<xrt_core::buffer_handle>
  alloc_bo(void* userptr, size_t size, uint64_t flags) override
  { throw xrt_core::ishim::not_supported_error(__func__); }

  std::unique_ptr<xrt_core::hwctx_handle>
  create_hw_context(const xrt::uuid&,
                    const xrt::hw_context::cfg_param_type&,
                    xrt::hw_context::access_mode) const override
  { throw xrt_core::ishim::not_supported_error(__func__); }
};

} // namespace shim_amdgpu

#endif
