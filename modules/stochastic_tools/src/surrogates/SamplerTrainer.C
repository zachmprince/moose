//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "SamplerTrainer.h"

InputParameters
SamplerTrainer::validParams()
{
  InputParameters params = SurrogateTrainer::validParams();
  params += SamplerInterface::validParams();
  params.addRequiredParam<SamplerName>("sampler", "Training set defined by a sampler object.");
  params.addRequiredParam<VectorPostprocessorName>(
      "results_vpp", "Vectorpostprocessor with results of samples created by trainer.");
  params.addRequiredParam<std::string>(
      "results_vector",
      "Name of vector from vectorpostprocessor with results of samples created by trainer");

  params.addParam<std::vector<dof_id_type>>(
      "skip_index", std::vector<dof_id_type>(0), "Starting data index to skip (indexing starts at 1)");
  params.addParam<std::vector<dof_id_type>>(
      "num_skip", std::vector<dof_id_type>(0), "Number of rows to skip for each skip_index.");

  return params;
}

SamplerTrainer::SamplerTrainer(const InputParameters & parameters)
  : SurrogateTrainer(parameters), SamplerInterface(this)
{
  const auto first_vec = getParam<std::vector<dof_id_type>>("skip_index");
  const auto nrow_vec = getParam<std::vector<dof_id_type>>("num_skip");
  if (first_vec.size() != nrow_vec.size())
    paramError(isParamValid("num_skip") ? "num_skip" : "skip_index", "Number of entries in skip_index and num_skip must match.");

  _skip_indices.clear();
  for (unsigned int n = 0; n < first_vec.size(); ++n)
    for (dof_id_type i = first_vec[n]; i < nrow_vec[n] + first_vec[n]; ++i)
      _skip_indices.insert(i - 1);
}

void
SamplerTrainer::initialSetup()
{
  // Results VPP
  _values_distributed = isVectorPostprocessorDistributed("results_vpp");
  _values_ptr = &getVectorPostprocessorValue(
      "results_vpp", getParam<std::string>("results_vector"), !_values_distributed);

  // Sampler
  _sampler = &getSamplerByName(getParam<SamplerName>("sampler"));

  // Number of data points
  const auto first_skip = std::lower_bound(_skip_indices.begin(), _skip_indices.end(), _sampler->getLocalRowBegin());
  const auto last_skip = std::lower_bound(first_skip, _skip_indices.end(), _sampler->getLocalRowEnd());
  _n_points = _sampler->getNumberOfRows() - _skip_indices.size();
  _n_local_points = _sampler->getNumberOfLocalRows() - std::distance(first_skip, last_skip);
  _first_p = _sampler->getLocalRowBegin() - std::distance(_skip_indices.begin(), first_skip);
}

void
SamplerTrainer::initialize()
{
  // Check if results of samples matches number of samples
  __attribute__((unused)) dof_id_type num_rows =
      _values_distributed ? _sampler->getNumberOfLocalRows() : _sampler->getNumberOfRows();
  if (num_rows != _values_ptr->size())
    paramError("results_vpp",
               "The number of elements in '",
               getParam<VectorPostprocessorName>("results_vpp"),
               "/",
               getParam<std::string>("results_vector"),
               "' is not equal to the number of samples in '",
               getParam<SamplerName>("sampler"),
               "'!");
}

void
SamplerTrainer::execute()
{
  _p = _first_p;
  _local_p = 0;

  preTrain();

  // Offset for replicated/distributed result data
  dof_id_type offset = _values_distributed ? _sampler->getLocalRowBegin() : 0;

  // Loop over samples
  for (_row = _sampler->getLocalRowBegin(); _row < _sampler->getLocalRowEnd(); ++_row)
  {
    _data = _sampler->getNextLocalRow();
    _val = (*_values_ptr)[_row - offset];

    if (_skip_indices.find(_row) != _skip_indices.end())
      continue;

    train();

    _p++;
    _local_p++;
  }

  postTrain();
}

void
SamplerTrainer::finalize()
{
}
