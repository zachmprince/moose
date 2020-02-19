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

class QuadratureSampler;

template <>
InputParameters validParams<QuadratureSampler>();
/**
 * A class used to produce samples based on quadrature for Polynomial Chaos
 */
class QuadratureSampler : public Sampler
{
public:
  static InputParameters validParams();

  QuadratureSampler(const InputParameters & parameters);

  Real getQuadratureWeight(dof_id_type row_index) const;

protected:
  /**
   * Function that performs tensor produces of all inputted points and weights
   */
  static void tensorGrid(const std::vector<std::vector<Real>> & points_1D,
                         const std::vector<std::vector<Real>> & weights_1D,
                         DenseMatrix<Real> & points,
                         DenseVector<Real> & weights);

  /// Return the sample for the given row and column
  virtual Real computeSample(dof_id_type row_index, dof_id_type col_index) override;

  ///@{
  /// Quadrature weights and points
  DenseMatrix<Real> _qpoints;
  DenseVector<Real> _qweights;
  ///@}
};
