//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "AddCrossValidationAction.h"

class KFold : public AddCrossValidationAction
{
public:
  static InputParameters validParams();

  KFold(const InputParameters & params);

protected:
  /// See if the trainer's sampler is sorted (should shuffle indices if this is the case)
  static bool isSamplerSorted(const std::string & type);

  /// Number of train-test splits
  virtual dof_id_type numSets() const override;
  /// Starting indices for each testing set
  virtual std::vector<dof_id_type> testIndices(dof_id_type i) const override;
  /// Number of rows for each testing set
  virtual std::vector<dof_id_type> testNumRow(dof_id_type i) const override;

private:
  /// Number of folds (k), i.e. number of train-test sets
  const unsigned int _num_folds;
  /// Whether or not sampler indices need to be shuffled (usually the case for deterministic sampling)
  const bool _shuffle;
  /// Number of points in each test set
  std::vector<dof_id_type> _test_nrow;
  /// Index of first point in test set
  std::vector<dof_id_type> _test_index;
  /// Shuffled indices (only defined and used if _shuffle == true)
  std::vector<dof_id_type> _shuffled_indices;
};
