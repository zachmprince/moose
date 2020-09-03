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
#include "Standardizer.h"
#include <Eigen/Dense>

#include "CovarianceFunctionBase.h"
#include "CovarianceInterface.h"

class GaussianProcessTrainer : public SamplerTrainer, public CovarianceInterface
{
public:
  static InputParameters validParams();
  GaussianProcessTrainer(const InputParameters & parameters);

  CovarianceFunctionBase * getCovarPtr() const { return _covariance_function; }

protected:
  virtual void preTrain() override;
  virtual void train() override;
  virtual void postTrain() override;

private:
  /// Paramaters (x) used for training, along with statistics
  RealEigenMatrix & _training_params;

  /// Standardizer for use with params (x)
  StochasticTools::Standardizer & _param_standardizer;

  /// Data (y) used for training
  RealEigenMatrix _training_data;

  /// Standardizer for use with data (y)
  StochasticTools::Standardizer & _data_standardizer;

  /// An _n_sample by _n_sample covariance matrix constructed from the selected kernel function
  RealEigenMatrix & _K;

  /// A solve of Ax=b via Cholesky.
  RealEigenMatrix & _K_results_solve;

  /// Cholesky decomposition Eigen object
  Eigen::LLT<RealEigenMatrix> _K_cho_decomp;

  /// Switch for training param (x) standardization
  bool _standardize_params;

  /// Switch for training data(y) standardization
  bool _standardize_data;

  /// Scalar hyperparameters. Stored for use in surrogate
  std::unordered_map<std::string, Real> & _hyperparam_map;

  /// Vector hyperparameters. Stored for use in surrogate
  std::unordered_map<std::string, std::vector<Real>> & _hyperparam_vec_map;

  /// Covariance function object
  CovarianceFunctionBase * _covariance_function = nullptr;

  /// Type of covariance function used for this surrogate
  std::string & _covar_type;
};
