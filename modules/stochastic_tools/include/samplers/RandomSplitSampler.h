//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "SplitSampler.h"

class RandomSplitSampler : public SplitSampler
{
public:
  static InputParameters validParams();

  RandomSplitSampler(const InputParameters & parameters);

protected:
  /// Could have new random numbers, so recomputing
  virtual void recomputeSplitIndices() override;

  /// Number of rows to extract
  const dof_id_type & _nrows;
  /// Whether to or not there are repeated entries
  const bool & _allow_repeat;
};
