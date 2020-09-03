//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "libmesh/utility.h"
#include "SamplerTrainer.h"
#include "MultiDimPolynomialGenerator.h"

class PolynomialRegressionTrainer : public SamplerTrainer
{
public:
  static InputParameters validParams();

  PolynomialRegressionTrainer(const InputParameters & parameters);

protected:
  virtual void preTrain() override;
  virtual void train() override;
  virtual void postTrain() override;

  /// Coefficients of regression model
  std::vector<Real> & _coeff;

  /// Matirx co containing the touples of the powers for each term
  std::vector<std::vector<unsigned int>> & _power_matrix;

  /// Types for the polynomial regression
  const MooseEnum & _regression_type;

  /// Leverages
  std::vector<Real> & _hatval;

private:
  /// Maximum polynomial degree, limiting the sum of constituent polynomial degrees.
  const unsigned int & _max_degree;

  /// The penalty parameter for Ridge regularization
  const Real & _penalty;

  /// Number of terms in the polynomial expression.
  unsigned int _n_poly_terms;

  ///@{
  /// Matrix and rhs for the regression problem
  DenseMatrix<Real> _matrix;
  DenseVector<Real> _rhs;
  ///@}

  /// Local predictor matrix
  std::vector<DenseVector<Real>> _x_full;
};
