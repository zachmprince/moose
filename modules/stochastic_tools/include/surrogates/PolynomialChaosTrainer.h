//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "SamplerTrainer.h"
#include "PolynomialQuadrature.h"
#include "QuadratureSampler.h"
#include "MultiDimPolynomialGenerator.h"

#include "Distribution.h"

class PolynomialChaosTrainer : public SamplerTrainer
{
public:
  static InputParameters validParams();
  PolynomialChaosTrainer(const InputParameters & parameters);
  virtual void initialSetup() override;

protected:
  virtual void preTrain() override;
  virtual void train() override;
  virtual void postTrain() override;

private:
  // TODO: Move as much of these to constructor initialization

  /// QuadratureSampler pointer, necessary for applying quadrature weights
  const QuadratureSampler * _quad_sampler = nullptr;

  // The following items are stored using declareModelData for use as a trained model.

  /// Maximum polynomial order. The sum of 1D polynomial orders does not go above this value.
  const unsigned int & _order;

  /// Total number of parameters/dimensions
  unsigned int & _ndim;

  /// Total number of coefficient (defined by size of _tuple)
  std::size_t & _ncoeff;

  /// A _ndim-by-_ncoeff matrix containing the appropriate one-dimensional polynomial order
  std::vector<std::vector<unsigned int>> & _tuple;

  /// These are the coefficients we are after in the PC expansion
  std::vector<Real> & _coeff;

  /// The distributions used for sampling
  std::vector<std::unique_ptr<const PolynomialQuadrature::Polynomial>> & _poly;

  /// Leverages
  std::vector<Real> & _hatval;
};
