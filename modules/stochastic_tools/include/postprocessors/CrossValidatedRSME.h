//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "GeneralPostprocessor.h"

#include "SamplerInterface.h"
#include "SurrogateModelInterface.h"
#include "Sampler.h"

#include "PolynomialRegressionSurrogate.h"
#include "PolynomialChaos.h"

/**
 * Compute several goodness of fit metrics for supplied surrogate results or models.
 */
class CrossValidatedRSME : public GeneralPostprocessor,
                           public SamplerInterface,
                           public SurrogateModelInterface
{
public:
  static InputParameters validParams();
  CrossValidatedRSME(const InputParameters & parameters);

  virtual void initialSetup() override;
  virtual void execute() override;

  virtual void initialize() override;
  virtual void finalize() override;

  virtual PostprocessorValue getValue() override { return _rsme; };

protected:
  /// Cross validated RSME
  PostprocessorValue _rsme;

  /// Sampler for evaluating surrogate models
  Sampler & _sampler;

  /// Cross validated models
  std::vector<const SurrogateModel *> _models;

  /// Testing indices for each model
  std::vector<std::set<dof_id_type>> _test_indices;

  /// Number of points in each test set
  std::vector<dof_id_type> _n_points;

  /// Whether the supplied VPPs are distributed
  bool _is_distributed;

  /// Data containing observed values
  const VectorPostprocessorValue * _data = nullptr;

private:
  ///@{
  /// PrefGraph timers
  const PerfID _perf_initial_setup;
  const PerfID _perf_execute;
  ///@}

  /// Square mean error for each model
  std::vector<Real> _sme;
};
