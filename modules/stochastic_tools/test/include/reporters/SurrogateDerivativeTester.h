//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

// MOOSE includes
#include "StochasticReporter.h"
#include "SurrogateModelInterface.h"

/**
 * A tool for output Sampler data.
 */
template <typename ResponseType>
class SurrogateDerivativeTesterTempl : public StochasticReporter, SurrogateModelInterface
{
public:
  static InputParameters validParams();

  SurrogateDerivativeTesterTempl(const InputParameters & parameters);
  virtual void initialize() override {}
  virtual void execute() override;
  virtual void finalize() override {}

protected:
  void evaluateFiniteDifference(const SurrogateModel & model,
                                const std::vector<Real> & x,
                                std::vector<ResponseType> & fd_dydx) const;
  /// Model to evaluate derivative
  const SurrogateModel & _model;
  /// Sampler for evaluating surrogate model
  Sampler & _sampler;
  /// Vectors containing results of sampling model derivative
  std::vector<std::vector<ResponseType>> & _values;
  /// Vectors containing results of sampling model with finite difference
  std::vector<std::vector<ResponseType>> & _fd_values;
};

typedef SurrogateDerivativeTesterTempl<Real> SurrogateDerivativeTester;
typedef SurrogateDerivativeTesterTempl<std::vector<Real>> VectorSurrogateDerivativeTester;
