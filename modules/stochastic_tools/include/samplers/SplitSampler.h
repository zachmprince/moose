//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "Sampler.h"
#include "DistributedData.h"

class SplitSampler : public Sampler
{
public:
  static InputParameters validParams();

  SplitSampler(const InputParameters & parameters);

protected:
  virtual Real computeSample(dof_id_type row_index, dof_id_type col_index) override final;
  virtual void sampleSetUp() override final;

  /// This function is called in sampleSetUp to allow derived classes to recompute _split_indices
  virtual void recomputeSplitIndices() {}

  /// Sampler to split
  Sampler & _sampler;

  /// Local matrix of samples
  DenseMatrix<Real> _matrix;

  /// Sample indices to choose from main sampler
  std::vector<dof_id_type> _split_indices;

  /// PerfGraph timer
  const PerfID _perf_sample_setup;
  const PerfID _perf_compute_sample;
};
