//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "LinearLOOCrossValidation.h"

registerMooseObject("StochasticToolsApp", LinearLOOCrossValidation);

InputParameters
LinearLOOCrossValidation::validParams()
{
  InputParameters params = GeneralPostprocessor::validParams();
  params += SamplerInterface::validParams();
  params += SurrogateModelInterface::validParams();
  params.addClassDescription("Compute leave-one-out RSME for linear surrogate models.");

  params.addRequiredParam<VectorPostprocessorName>(
      "data_vpp", "VectorPostprocessor containing vector of observed quantities.");
  params.addRequiredParam<std::string>("data_vector",
                                       "VectorPostprocessor vector of observed quantities.");

  params.addRequiredParam<SamplerName>("sampler",
                                       "Sampler for evaluating surrogate model, should be same "
                                       "sampler that was used to train model.");
  params.addRequiredParam<UserObjectName>("model", "Model to perform cross validation.");
  return params;
}

LinearLOOCrossValidation::LinearLOOCrossValidation(const InputParameters & parameters)
  : GeneralPostprocessor(parameters),
    SamplerInterface(this),
    SurrogateModelInterface(this),
    _rsme(0.0),
    _sampler(getSampler("sampler")),
    _perf_initial_setup(registerTimedSection("initialSetup", 2)),
    _perf_execute(registerTimedSection("execute", 1))
{
  const SurrogateModel & sm = getSurrogateModel("model");
  _pr_model = dynamic_cast<const PolynomialRegressionSurrogate *>(&sm);
  _pc_model = dynamic_cast<const PolynomialChaos *>(&sm);

  if (!_pr_model && !_pc_model)
    paramError(
        "model", "Surrogate model of type ", sm.type(), " is not supported for this object.");
}

void
LinearLOOCrossValidation::initialSetup()
{
  TIME_SECTION(_perf_initial_setup);

  _is_distributed = isVectorPostprocessorDistributed("data_vpp");
  _data = &getVectorPostprocessorValue("data_vpp", getParam<std::string>("data_vector"), true);
}

void
LinearLOOCrossValidation::initialize()
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
LinearLOOCrossValidation::execute()
{
  TIME_SECTION(_perf_execute);

  dof_id_type offset = _is_distributed ? _sampler.getLocalRowBegin() : 0;
  _rsme = 0.0;
  if (_pr_model)
  {
    for (unsigned int i = _sampler.getLocalRowBegin(); i < _sampler.getLocalRowEnd(); ++i)
    {
      Real val = _pr_model->evaluate(_sampler.getNextLocalRow());
      Real hatval = _pr_model->getLeverage(i);
      Real res = ((*_data)[i - offset] - val) / (1.0 - hatval);
      _rsme += res * res;
    }
  }
  else if (_pc_model)
  {
    for (unsigned int i = _sampler.getLocalRowBegin(); i < _sampler.getLocalRowEnd(); ++i)
    {
      Real val = _pc_model->evaluate(_sampler.getNextLocalRow());
      Real hatval = _pc_model->getLeverage(i);
      Real res = (1.0 - hatval) * (*_data)[i - offset] - val;
      _rsme += res * res;
    }
  }
}

void
LinearLOOCrossValidation::finalize()
{
  gatherSum(_rsme);
  _rsme /= static_cast<Real>(_sampler.getNumberOfRows());
  _rsme = std::sqrt(_rsme);
}
