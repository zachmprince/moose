//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "CrossValidatedRSME.h"

registerMooseObject("StochasticToolsApp", CrossValidatedRSME);

InputParameters
CrossValidatedRSME::validParams()
{
  InputParameters params = GeneralPostprocessor::validParams();
  params += SamplerInterface::validParams();
  params += SurrogateModelInterface::validParams();
  params.addClassDescription("Compute Cross Validated RSME for a given set of surrogate models.");

  params.addRequiredParam<VectorPostprocessorName>(
      "data_vpp", "VectorPostprocessor containing vector of observed quantities.");
  params.addRequiredParam<std::string>("data_vector",
                                       "VectorPostprocessor vector of observed quantities.");
  params.addRequiredParam<SamplerName>("sampler",
                                       "Sampler for evaluating surrogate model, should be same "
                                       "sampler that was used to train model.");

  params.addRequiredParam<std::vector<UserObjectName>>(
      "models", "Cross valideated models to perform cross validation.");
  params.addRequiredParam<std::vector<std::vector<dof_id_type>>>(
      "test_index", "Starting test data index for each model (indexing starts at 1).");
  params.addRequiredParam<std::vector<std::vector<dof_id_type>>>(
      "num_points", "Number of test points to include for each index for each model.");
  return params;
}

CrossValidatedRSME::CrossValidatedRSME(const InputParameters & parameters)
  : GeneralPostprocessor(parameters),
    SamplerInterface(this),
    SurrogateModelInterface(this),
    _rsme(0.0),
    _sampler(getSampler("sampler")),
    _perf_initial_setup(registerTimedSection("initialSetup", 2)),
    _perf_execute(registerTimedSection("execute", 1))
{
  const auto model_names = getParam<std::vector<UserObjectName>>("models");
  const auto first_vec = getParam<std::vector<std::vector<dof_id_type>>>("test_index");
  const auto nrow_vec = getParam<std::vector<std::vector<dof_id_type>>>("num_points");
  if (model_names.size() != first_vec.size() || model_names.size() != nrow_vec.size())
    paramError(model_names.size() != first_vec.size() ? "test_index" : "num_points",
               "Number of test sets must match number of models.");

  _models.reserve(model_names.size());
  _test_indices.resize(model_names.size());
  _n_points.resize(model_names.size());
  for (unsigned int m = 0; m < model_names.size(); ++m)
  {
    _models.push_back(&getSurrogateModelByName(model_names[m]));

    if (first_vec[m].size() != nrow_vec[m].size())
      paramError("num_points", "Size of test_index does not match size of num_points.");

    for (unsigned int n = 0; n < first_vec[m].size(); ++n)
    {
      dof_id_type start = std::max(first_vec[m][n] - 1, _sampler.getLocalRowBegin());
      dof_id_type end = std::min(first_vec[m][n] + nrow_vec[m][n] - 1, _sampler.getLocalRowEnd());
      for (dof_id_type i = start; i < end; ++i)
        _test_indices[m].insert(i);
    }

    _n_points[m] = std::accumulate(nrow_vec[m].begin(), nrow_vec[m].end(), 0);
  }
}

void
CrossValidatedRSME::initialSetup()
{
  TIME_SECTION(_perf_initial_setup);

  _is_distributed = isVectorPostprocessorDistributed("data_vpp");
  _data = &getVectorPostprocessorValue("data_vpp", getParam<std::string>("data_vector"), true);
}

void
CrossValidatedRSME::initialize()
{
  // Check that vector sizes are the same
  dof_id_type data_size = _data->size();
  if (_is_distributed)
    gatherSum(data_size);

  if (_sampler.getNumberOfRows() != data_size)
    paramError("sampler",
               "Number of sampler rows (",
               _sampler.getNumberOfRows(),
               ") and size of data vector (",
               data_size,
               ") need to be consistent.");
}

void
CrossValidatedRSME::execute()
{
  TIME_SECTION(_perf_execute);

  dof_id_type offset = _is_distributed ? _sampler.getLocalRowBegin() : 0;
  _sme.resize(_models.size());
  for (unsigned int m = 0; m < _models.size(); ++m)
  {
    for (unsigned int i = _sampler.getLocalRowBegin(); i < _sampler.getLocalRowEnd(); ++i)
    {
      std::vector<Real> param = _sampler.getNextLocalRow();
      if (_test_indices[m].find(i) == _test_indices[m].end())
        continue;

      Real res = (*_data)[i - offset] - _models[m]->evaluate(param);
      _sme[m] += res * res;
    }
    _sme[m] /= _n_points[m];
  }
}

void
CrossValidatedRSME::finalize()
{
  gatherSum(_sme);
  _rsme = std::sqrt(std::accumulate(_sme.begin(), _sme.end(), 0.0) / _models.size());
}
