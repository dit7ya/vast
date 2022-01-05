//    _   _____   __________
//   | | / / _ | / __/_  __/     Visibility
//   | |/ / __ |_\ \  / /          Across
//   |___/_/ |_/___/ /_/       Space and Time
//
// SPDX-FileCopyrightText: (c) 2021 The VAST Contributors
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "vast/expression.hpp"
#include "vast/transform.hpp"

namespace vast {

// Selects mathcing rows from the input
class select_step : public transform_step {
public:
  select_step(std::string expr);

  /// Applies the transformation to a record batch with a corresponding vast
  /// layout.
  [[nodiscard]] caf::error
  add(vast::id offset, type layout,
      std::shared_ptr<arrow::RecordBatch> batch) override;

  /// Retrieves the result of the transformation.
  [[nodiscard]] caf::expected<batch_vector> finish() override;

private:
  caf::expected<vast::expression> expression_;
  batch_vector transformed_;
};

} // namespace vast
