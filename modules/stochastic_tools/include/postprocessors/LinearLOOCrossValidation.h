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
class LinearLOOCrossValidation : public GeneralPostprocessor,
                                 public SamplerInterface,
                                 public SurrogateModelInterface
{
public:
  static InputParameters validParams();
  LinearLOOCrossValidation(const InputParameters & parameters);

  virtual void initialSetup() override;
  virtual void execute() override;

  virtual void initialize() override;
  virtual void finalize() override;

  virtual PostprocessorValue getValue() override { return _rsme; };

protected:
  /// Leave-one-out cross validated RSME
  PostprocessorValue _rsme;

  /// Sampler for evaluating surrogate models
  Sampler & _sampler;

  ///@{
  /// Surrogate model
  const PolynomialRegressionSurrogate * _pr_model = nullptr;
  const PolynomialChaos * _pc_model = nullptr;
  ///@}

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
};
