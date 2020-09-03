//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "GaussianProcessTrainer.h"
#include "Sampler.h"
#include "CartesianProduct.h"

registerMooseObject("StochasticToolsApp", GaussianProcessTrainer);

InputParameters
GaussianProcessTrainer::validParams()
{
  InputParameters params = SamplerTrainer::validParams();
  params += CovarianceInterface::validParams();
  params.addClassDescription(
      "Provides data preperation and training for a Gaussian Process surrogate model.");
  params.addRequiredParam<UserObjectName>("covariance_function", "Name of covariance function.");
  params.addParam<bool>(
      "standardize_params", true, "Standardize (center and scale) training parameters (x values)");
  params.addParam<bool>(
      "standardize_data", true, "Standardize (center and scale) training data (y values)");

  return params;
}

GaussianProcessTrainer::GaussianProcessTrainer(const InputParameters & parameters)
  : SamplerTrainer(parameters),
    CovarianceInterface(parameters),
    _training_params(declareModelData<RealEigenMatrix>("_training_params")),
    _param_standardizer(declareModelData<StochasticTools::Standardizer>("_param_standardizer")),
    _training_data(),
    _data_standardizer(declareModelData<StochasticTools::Standardizer>("_data_standardizer")),
    _K(declareModelData<RealEigenMatrix>("_K")),
    _K_results_solve(declareModelData<RealEigenMatrix>("_K_results_solve")),
    _standardize_params(getParam<bool>("standardize_params")),
    _standardize_data(getParam<bool>("standardize_data")),
    _hyperparam_map(declareModelData<std::unordered_map<std::string, Real>>("_hyperparam_map")),
    _hyperparam_vec_map(declareModelData<std::unordered_map<std::string, std::vector<Real>>>(
        "_hyperparam_vec_map")),
    _covariance_function(
        getCovarianceFunctionByName(getParam<UserObjectName>("covariance_function"))),
    _covar_type(declareModelData<std::string>("_covar_type", _covariance_function->type()))
{
}

void
GaussianProcessTrainer::preTrain()
{
  // Consider the possibility of a very large matrix load.
  _training_params.setZero(getNumberOfPoints(), getNumberOfParameters());
  _training_data.setZero(getNumberOfPoints(), 1);
}

void
GaussianProcessTrainer::train()
{
  for (unsigned int d = 0; d < _data.size(); ++d)
    _training_params(_p, d) = _data[d];

  // Loading result data from VPP
  _training_data(_p, 0) = _val;
}

void
GaussianProcessTrainer::postTrain()
{
  for (unsigned int ii = 0; ii < _training_params.rows(); ++ii)
  {
    for (unsigned int jj = 0; jj < _training_params.cols(); ++jj)
      gatherSum(_training_params(ii, jj));
    gatherSum(_training_data(ii, 0));
  }

  // Standardize (center and scale) training params
  if (_standardize_params)
  {
    _param_standardizer.computeSet(_training_params);
    _param_standardizer.getStandardized(_training_params);
  }
  // if not standardizing data set mean=0, std=1 for use in surrogate
  else
    _param_standardizer.set(0, 1, getNumberOfParameters());

  // Standardize (center and scale) training data
  if (_standardize_data)
  {
    _data_standardizer.computeSet(_training_data);
    _data_standardizer.getStandardized(_training_data);
  }
  // if not standardizing data set mean=0, std=1 for use in surrogate
  else
    _param_standardizer.set(0, 1, getNumberOfParameters());

  _covariance_function->buildHyperParamMap(_hyperparam_map, _hyperparam_vec_map);
  _K.resize(_training_params.rows(), _training_params.rows());
  _covariance_function->computeCovarianceMatrix(_K, _training_params, _training_params, true);
  _K_cho_decomp = _K.llt();
  _K_results_solve = _K_cho_decomp.solve(_training_data);
}
