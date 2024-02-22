/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <ATen/ATen.h>
#include <ATen/core/op_registration/op_registration.h>
#include <torch/library.h>
#include "torch/types.h"

static at::Tensor qlinear_channelwise(
    at::Tensor x,
    at::Tensor weight,
    at::Tensor bias,
    at::Tensor input_scale,
    at::Tensor weight_scale,
    at::Tensor weight_zero_point,
    at::Tensor relu) {
  // quantized linear function with
  // activation: per-tensor quantization
  // weight: per-tensor quantization
  return x;
}

static at::Tensor qlinear_quant(
    at::Tensor x,
    at::Tensor weight,
    at::Tensor bias,
    at::Tensor input_scale,
    at::Tensor weight_scale,
    at::Tensor weight_zero_point,
    at::Tensor relu) {
  at::Tensor X = x.contiguous();
  const float* x_data = X.data_ptr<float>();
  const int M = X.sizes()[0];
  const int K = X.sizes()[1];
  at::Tensor I_S = input_scale.contiguous();
  const float* input_scale_data = I_S.data_ptr<float>();

  std::vector<uint8_t> X_int8_vec =
      std::vector<uint8_t>(M * (std::vector<uint8_t>::size_type)(K));
  for (int m = 0; m < M; m++) {
    const float inv_scale = 1.0f / input_scale_data[m];
    for (int k = 0; k < K; k++) {
      int32_t val = int32_t(inv_scale * x_data[m * K + k]) + 127;
      X_int8_vec[m * K + k] = uint8_t(std::max(0, std::min(val, UINT8_MAX)));
    }
  }

  auto Y = at::from_blob(
      X_int8_vec.data(), {M, K}, at::TensorOptions().dtype(torch::kUInt8));
  return Y;
}

static at::Tensor qlinear_qparams(
    at::Tensor x,
    at::Tensor weight,
    at::Tensor bias,
    at::Tensor input_scale,
    at::Tensor weight_scale,
    at::Tensor weight_zero_point,
    at::Tensor relu) {
  assert(x.options().dtype() == at::kHalf);
  assert(weight.options().dtype() == at::kQInt8);
  assert(bias.options().dtype() == at::kFloat);
  assert(input_scale.options().dtype() == at::kFloat);
  assert(weight_scale.options().dtype() == at::kFloat);
  assert(weight_zero_point.options().dtype() == at::kQUInt8);
  return x;
}

TORCH_LIBRARY_FRAGMENT(fbgemm, m) {
  m.def(
      "qlinear_channelwise(Tensor x, Tensor weight, Tensor "
      "bias, Tensor input_scale, Tensor weight_scale, Tensor "
      "weight_zero_point, Tensor relu) -> Tensor");
  m.impl(
      "qlinear_channelwise",
      torch::dispatch(c10::DispatchKey::CPU, TORCH_FN(qlinear_channelwise)));

  m.def(
      "qlinear_quant(Tensor x, Tensor weight, Tensor "
      "bias, Tensor input_scale, Tensor weight_scale, Tensor "
      "weight_zero_point, Tensor relu) -> Tensor");

  m.impl(
      "qlinear_quant",
      torch::dispatch(c10::DispatchKey::CPU, TORCH_FN(qlinear_quant)));

  m.def(
      "qlinear_qparams(Tensor x, Tensor weight, Tensor "
      "bias, Tensor input_scale, Tensor weight_scale, Tensor "
      "weight_zero_point, Tensor relu) -> Tensor");

  m.impl(
      "qlinear_qparams",
      torch::dispatch(c10::DispatchKey::CPU, TORCH_FN(qlinear_qparams)));
}
