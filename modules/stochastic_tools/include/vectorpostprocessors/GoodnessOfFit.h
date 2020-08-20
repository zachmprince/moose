//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "GeneralVectorPostprocessor.h"
#include "SamplerInterface.h"
#include "SurrogateModelInterface.h"

/**
 * Compute several goodness of fit metrics for supplied surrogate results or models.
 */
class GoodnessOfFit : public GeneralVectorPostprocessor,
                      public SamplerInterface,
                      public SurrogateModelInterface
{
public:
  static InputParameters validParams();
  GoodnessOfFit(const InputParameters & parameters);

  virtual void initialSetup() override;
  virtual void execute() override;

  virtual void initialize() override;
  virtual void finalize() override{};

protected:
  /**
   * This function is for computing certain statistics that might be needed by
   * multiple GOF metrics.
   */
  virtual void preComputeStatistics();

  /// Determines whether or not SST needs to be computed given the desired metrics
  virtual bool isSSTNeeded() const;

  /// Determines whether or not SSE needs to be computed given the desired metrics
  virtual bool isSSENeeded() const;

  /// The selected GOF metrics to compute
  const MultiMooseEnum & _gof;

  /// The VPP vector that will hold the GOF identifiers
  VectorPostprocessorValue & _gof_type;

  /// Sampler for evaluating surrogate models
  Sampler * _sampler;

  /// Number of models to compute metrics for
  unsigned int _num_models;

  /// Vector of surrogate models
  std::vector<const SurrogateModel *> _model;

  /// Names of VPPs holding the model data
  std::vector<VectorPostprocessorName> _model_vpp_names;

  /// Names of vectors holding the model data
  std::vector<std::string> _model_vector_names;

  /// Data from models
  std::vector<const VectorPostprocessorValue *> _model_data;

  /// Vectors containing GOF metrics for each model
  std::vector<VectorPostprocessorValue *> _gof_vectors;

  /// Whether the supplied VPPs are distributed
  bool _is_distributed;

  /// Data containing observed values
  const VectorPostprocessorValue * _data = nullptr;

  /// Size of data sets
  dof_id_type _data_size;

  /// Standard deviation of observed values (SST)
  Real _sst;

  /// Sum squared error for each model (SSE)
  std::vector<Real> _sse;

private:
  /// Computes standard deviation of the supplied data
  Real computeSST(const VectorPostprocessorValue & data, bool is_distributed);

  /// Computes sum squared difference between the two supplied data sets
  Real computeSSE(const VectorPostprocessorValue & data,
                  const VectorPostprocessorValue & model_data,
                  bool is_distributed);

  /// Computes sum squared difference between the data and sampler model evaluations
  Real computeSSE(const VectorPostprocessorValue & data,
                  const SurrogateModel & model,
                  Sampler & sampler,
                  bool is_distributed);

  ///@{
  /// PrefGraph timers
  const PerfID _perf_initial_setup;
  const PerfID _perf_execute;
  ///@}
};
