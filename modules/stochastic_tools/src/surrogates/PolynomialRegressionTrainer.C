//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "PolynomialRegressionTrainer.h"

registerMooseObject("StochasticToolsApp", PolynomialRegressionTrainer);

InputParameters
PolynomialRegressionTrainer::validParams()
{
  InputParameters params = SamplerTrainer::validParams();
  params.addClassDescription("Computes coefficients for polynomial regession model.");

  MooseEnum rtype("ols=0 ridge=1");
  params.addRequiredParam<MooseEnum>(
      "regression_type", rtype, "The type of regression to perform.");
  params.addRequiredParam<unsigned int>("max_degree",
                                        "Maximum polynomial degree to use for the regression.");
  params.addParam<Real>("penalty", 0.0, "Penalty for Ridge regularization.");

  return params;
}

PolynomialRegressionTrainer::PolynomialRegressionTrainer(const InputParameters & parameters)
  : SamplerTrainer(parameters),
    _coeff(declareModelData<std::vector<Real>>("_coeff")),
    _power_matrix(declareModelData<std::vector<std::vector<unsigned int>>>("_power_matrix")),
    _regression_type(getParam<MooseEnum>("regression_type")),
    _hatval(declareModelData<std::vector<Real>>("_hatval")),
    _max_degree(
        declareModelData<unsigned int>("_max_degree", getParam<unsigned int>("max_degree"))),
    _penalty(getParam<Real>("penalty"))
{
  // Throwing a warning if the penalty parameter is set with OLS regression
  if (_regression_type == "ols" && _penalty != 0)
    paramWarning(
        "penalty", "Penalty parameter is not used for OLS regression, found penalty=", _penalty);
}

void
PolynomialRegressionTrainer::preTrain()
{
  // Initializing power matrix, using _max_degree+1 to tackle the indexing offset
  // within generateTuple
  _power_matrix = StochasticTools::MultiDimPolynomialGenerator::generateTuple(
      getNumberOfParameters(), _max_degree + 1);

  _n_poly_terms = _power_matrix.size();

  // Check if we have enough data points to solve the problem
  if (getNumberOfPoints() <= _n_poly_terms)
    mooseError("Number of data points must be greater than the number of terms in the polynomial.");

  // Resize _coeff, _matrix, _rhs to number of sampler columns
  _coeff.resize(_n_poly_terms);
  _matrix.resize(_n_poly_terms, _n_poly_terms);
  _rhs.resize(_n_poly_terms);

  // Leverages
  _hatval.resize(getNumberOfLocalPoints());
  // Caching data for computing leverages
  const DenseVector<Real> zero_vec(_n_poly_terms);
  _x_full.resize(getNumberOfLocalPoints(), zero_vec);
}

void
PolynomialRegressionTrainer::train()
{
  // Caching the different powers of data to accelerate the assembly of the
  // system
  DenseMatrix<Real> data_pow(_data.size(), _max_degree + 1);
  for (unsigned int d = 0; d < _data.size(); ++d)
    for (unsigned int i = 0; i <= _max_degree; ++i)
      data_pow(d, i) = pow(_data[d], i);

  for (unsigned int i = 0; i < _n_poly_terms; ++i)
  {
    std::vector<unsigned int> i_powers(_power_matrix[i]);

    Real i_value(1.0);
    for (unsigned int ii = 0; ii < _data.size(); ++ii)
      i_value *= data_pow(ii, i_powers[ii]);
    _x_full[_local_p](i) = i_value;

    for (unsigned int j = 0; j < _n_poly_terms; ++j)
    {
      std::vector<unsigned int> j_powers(_power_matrix[j]);

      Real j_value(1.0);
      for (unsigned int jj = 0; jj < _data.size(); ++jj)
        j_value *= data_pow(jj, j_powers[jj]);

      _matrix(i, j) += i_value * j_value;
    }

    _rhs(i) += i_value * _val;
  }
}

void
PolynomialRegressionTrainer::postTrain()
{

  gatherSum(_matrix.get_values());
  gatherSum(_rhs.get_values());

  // Adding penalty term for Ridge regularization
  if (_regression_type == "ridge")
    for (unsigned int i = 0; i < _n_poly_terms; ++i)
    {
      _matrix(i, i) += _penalty;
    }

  DenseVector<Real> sol;
  _matrix.lu_solve(_rhs, sol);
  _coeff = sol.get_values();

  // Compute leverages
  DenseVector<Real> tmp(_n_poly_terms);
  for (dof_id_type i = 0; i < getNumberOfLocalPoints(); ++i)
  {
    _matrix.lu_solve(_x_full[i], tmp);
    _hatval[i] = tmp.dot(_x_full[i]);
  }
  _communicator.allgather(_hatval);
}
