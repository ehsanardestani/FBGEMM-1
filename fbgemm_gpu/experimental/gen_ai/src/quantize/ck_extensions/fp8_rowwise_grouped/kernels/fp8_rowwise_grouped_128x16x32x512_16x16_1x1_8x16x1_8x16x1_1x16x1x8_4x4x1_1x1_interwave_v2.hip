/*
* Copyright (c) Meta Platforms, Inc. and affiliates.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "fp8_rowwise_grouped_common.h"

std::vector<at::Tensor>
fp8_rowwise_grouped_128x16x32x512_16x16_1x1_8x16x1_8x16x1_1x16x1x8_4x4x1_1x1_interwave_v2(
  at::TensorList XQ,
  at::TensorList WQ,
  at::TensorList x_scale,
  at::TensorList w_scale,
  at::Tensor kernel_args,
  std::vector<at::Tensor> Y) {
  // Check if this input needs to be padded.
  bool pad = false;
  for (int i = 0; i < XQ.size(); i++) {
    int K = XQ[i].size(1);
    if (K % 512 != 0) {
      pad = true;
    }
  }
  if (pad) {
    using DeviceGemmInstance = DeviceGemmHelper<
      128,
      16,
      32,
      512,
      16,
      16,
      1,
      1,
      S<8, 16, 1>,
      S<8, 16, 1>,
      S<1, 16, 1, 8>,
      S<4, 4, 1>,
      1,
      1,
      ck::BlockGemmPipelineScheduler::Interwave,
      ck::BlockGemmPipelineVersion::v2,
      ck::tensor_operation::device::GemmSpecialization::KPadding>;
    // Run kernel instance.
    return f8f8bf16_rowwise_grouped_impl<DeviceGemmInstance>(
      XQ, WQ, x_scale, w_scale, kernel_args, Y);
  } else {
    using DeviceGemmInstance = DeviceGemmHelper<
      128,
      16,
      32,
      512,
      16,
      16,
      1,
      1,
      S<8, 16, 1>,
      S<8, 16, 1>,
      S<1, 16, 1, 8>,
      S<4, 4, 1>,
      1,
      1,
      ck::BlockGemmPipelineScheduler::Interwave,
      ck::BlockGemmPipelineVersion::v2,
      ck::tensor_operation::device::GemmSpecialization::Default>;
    // Run kernel instance.
    return f8f8bf16_rowwise_grouped_impl<DeviceGemmInstance>(
      XQ, WQ, x_scale, w_scale, kernel_args, Y);
  }
}
