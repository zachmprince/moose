//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "SurrogateModel.h"

class PolynomialRegressionSurrogate : public SurrogateModel
{
public:
  static InputParameters validParams();

  PolynomialRegressionSurrogate(const InputParameters & parameters);

  virtual Real evaluate(const std::vector<Real> & x) const override;

  virtual unsigned int getDOF() const override { return _coeff.size(); }

  /**
   * Obtaining leverages which are equivalent to d\hat{y}_i/dy_i or x_i(X^TX)^{-1}x_i^T.
   * This is mainly used for computing leave-one-out RSME.
   * See https://robjhyndman.com/hyndsight/loocv-linear-models/ for more details
   */
  Real getLeverage(dof_id_type ind) const
  {
    mooseAssert(ind < _hatval.size(), "Leverage index out of bounds.");
    return _hatval[ind];
  }

protected:
  /// Coefficients of regression model
  const std::vector<Real> & _coeff;

  /// The power matrix for the terms in the polynomial expressions
  const std::vector<std::vector<unsigned int>> & _power_matrix;

  /// Leverages
  const std::vector<Real> & _hatval;

private:
  /// Maximum polynomial degree, limiting the sum of constituent polynomial degrees.
  const unsigned int & _max_degree;
};
