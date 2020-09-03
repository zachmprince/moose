//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "SurrogateTrainer.h"
#include "SamplerInterface.h"
#include "Sampler.h"

class SamplerTrainer : public SurrogateTrainer, public SamplerInterface
{
public:
  static InputParameters validParams();
  SamplerTrainer(const InputParameters & parameters);
  virtual void initialSetup() override;
  virtual void initialize() override final;
  virtual void execute() override final;
  virtual void finalize() override final;

protected:
  /// Called before sampler loop. Usually to resize variables
  virtual void preTrain(){};
  /// Called within sampler loop
  virtual void train(){};
  /// Called after element loop. Usually to perform mpi communication
  virtual void postTrain(){};

  /// Length of training data
  dof_id_type getNumberOfPoints() const { return _n_points; }
  /// Local length of data
  dof_id_type getNumberOfLocalPoints() const { return _n_local_points; }
  /// Predictor dimensionality
  unsigned int getNumberOfParameters() const { return _sampler->getNumberOfCols(); }

  /// Reference to sampler object
  const Sampler & sampler() const { return *_sampler; }

  /// Global sampler row index
  dof_id_type _row;
  /// Global data index
  dof_id_type _p;
  /// Local data index
  dof_id_type _local_p;
  /// Sampler row
  std::vector<Real> _data;
  /// VPP data
  Real _val;

private:
  /// A set of data indices to skip (typically to save for validation)
  std::set<dof_id_type> _skip_indices;
  /// Sampler from which the parameters were perturbed
  Sampler * _sampler = nullptr;
  /// Vector postprocessor of the results from perturbing the model with _sampler
  const VectorPostprocessorValue * _values_ptr = nullptr;
  /// True when _sampler data is distributed
  bool _values_distributed = false; // default to false; set in initialSetup
  /// Number of data points
  dof_id_type _n_points;
  /// Number of local data points
  dof_id_type _n_local_points;
  /// Global index of first local point
  dof_id_type _first_p;
};
