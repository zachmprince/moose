//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "GoodnessOfFit.h"

#include "Calculators.h"
#include "Sampler.h"
#include "SurrogateModel.h"
#include "FDistribution.h"

registerMooseObject("StochasticToolsApp", GoodnessOfFit);

InputParameters
GoodnessOfFit::validParams()
{
  InputParameters params = GeneralVectorPostprocessor::validParams();
  params += SamplerInterface::validParams();
  params += SurrogateModelInterface::validParams();
  params.addClassDescription("Compute goodness of fit quantities for surrogate models.");

  params.addRequiredParam<VectorPostprocessorName>(
      "data_vpp", "VectorPostprocessor containing vector of observed quantities.");
  params.addRequiredParam<std::string>("data_vector",
                                       "VectorPostprocessor vector of observed quantities.");

  params.addParam<std::vector<VectorPostprocessorName>>(
      "model_results_vpp", "VectorPostprocessor(s) containing vector of model evaluations.");
  params.addParam<std::vector<std::string>>(
      "model_results_vector",
      "List of vectors containing the model evaluations, paired with the vectorpostprocessros "
      "listed in 'model_results_vpp'.");

  params.addParam<SamplerName>("sampler", "Sampler for evaluating surrogate models.");
  params.addParam<std::vector<UserObjectName>>("model",
                                               "Models to evaluate goodness of fit metrics.");

  MultiMooseEnum gof = MultiMooseEnum("rmse=0 rsquared=1 adj-rsquared=2 fstatistic=3 pvalue=4");
  params.addRequiredParam<MultiMooseEnum>(
      "compute", gof, "The goodness of fit quantities to be computed.");

  // Compute values are computed on rconst VectorPostprocessorValue & model_dataank 0 and broadcast
  params.set<MooseEnum>("parallel_type") = "REPLICATED";
  params.suppressParameter<MooseEnum>("parallel_type");
  params.set<bool>("_auto_boradcast") = true;

  return params;
}

GoodnessOfFit::GoodnessOfFit(const InputParameters & parameters)
  : GeneralVectorPostprocessor(parameters),
    SamplerInterface(this),
    SurrogateModelInterface(this),
    _gof(getParam<MultiMooseEnum>("compute")),
    _gof_type(declareVector("GOF_type")),
    _sampler(isParamValid("sampler") ? &getSampler("sampler") : nullptr),
    _perf_initial_setup(registerTimedSection("initialSetup", 2)),
    _perf_execute(registerTimedSection("execute", 1))
{
  if (isParamValid("model"))
  {
    const auto & model_names = getParam<std::vector<UserObjectName>>("model");
    _model.reserve(model_names.size());
    for (const auto & nm : model_names)
      _model.push_back(&getSurrogateModelByName(nm));
  }
  else if (_gof.contains("adj-rsquared") || _gof.contains("fstatistic") || _gof.contains("pvalue"))
    paramError("compute", "Models must be specified in 'model' param for selected GOF metrics.");

  if (_sampler)
  {
    if (isParamValid("model_results_vpp") || isParamValid("model_results_vector"))
      paramError(isParamValid("model_results_vpp") ? "model_results_vpp" : "model_results_vector",
                 "Using sampler to evaluate surrogate models, so no results vector is needed.");
    if (!isParamValid("model"))
      paramError("sampler", "Using sampler to evaluate models, but no models are provided.");

    _num_models = _model.size();
    _gof_vectors.reserve(_num_models);
    for (const auto & mod : _model)
      _gof_vectors.push_back(&declareVector(mod->name()));
  }
  else if (isParamValid("model_results_vpp") && isParamValid("model_results_vector"))
  {
    _model_vpp_names = getParam<std::vector<VectorPostprocessorName>>("model_results_vpp");
    _model_vector_names = getParam<std::vector<std::string>>("model_results_vector");
    _num_models = _model_vpp_names.size();
    if (_num_models != _model_vector_names.size())
      paramError("model_results_vector",
                 "There must be a vector name for each VectorPostprocessor listed in "
                 "'model_results_vpp'.");
    if (isParamValid("model") && _model.size() != _num_models)
      paramError(
          "model",
          "Number of models specified is inconsistent with number of model results vectors.");

    _model_data.reserve(_num_models);
    _gof_vectors.reserve(_num_models);
    for (unsigned int i = 0; i < _num_models; ++i)
    {
      std::string name = _model_vpp_names[i] + ":" + _model_vector_names[i];
      _gof_vectors.push_back(&declareVector(name));
    }
  }
  else
    paramError("compute",
               "GoodnessOfFit requires either a sampler and a set of models to be specified or a "
               "set of vpps and vectors.");

  for (const auto & item : _gof)
    _gof_type.push_back(item.id());

  for (auto & vec : _gof_vectors)
    vec->reserve(_gof_type.size());
}

void
GoodnessOfFit::initialSetup()
{
  TIME_SECTION(_perf_initial_setup);

  _is_distributed = isVectorPostprocessorDistributed("data_vpp");
  _data = &getVectorPostprocessorValue("data_vpp", getParam<std::string>("data_vector"), true);

  if (!_sampler)
    for (unsigned int i = 0; i < _num_models; ++i)
    {
      if (_is_distributed != isVectorPostprocessorDistributedByName(_model_vpp_names[i]))
        paramError("model_results_vpp",
                   "The data vectorpostprocessor parallel type must match the model "
                   "vectorpostprocessor(s) type.");
      _model_data.push_back(
          &getVectorPostprocessorValueByName(_model_vpp_names[i], _model_vector_names[i], true));
    }
}

void
GoodnessOfFit::initialize()
{
  // Check that vector sizes are the same
  _data_size = _data->size();
  if (_is_distributed)
    gatherSum(_data_size);

  if (_sampler)
  {
    if (_sampler->getNumberOfRows() != _data_size)
      paramError("sampler",
                 "Number of sampler rows (",
                 _sampler->getNumberOfRows(),
                 ") and size of data vector (",
                 _data_size,
                 ") need to be consistent.");
  }
  else
  {
    for (unsigned int i = 0; i < _num_models; ++i)
    {
      dof_id_type model_size = _model_data[i]->size();
      if (_is_distributed)
        gatherSum(model_size);
      if (model_size != _data_size)
        paramError("model_results_vector",
                   "The size of model vector ",
                   _model_vector_names[i],
                   " (",
                   model_size,
                   ")",
                   " does not match data vector (",
                   _data_size,
                   ").");
    }
  }
}

void
GoodnessOfFit::execute()
{
  TIME_SECTION(_perf_execute);

  preComputeStatistics();

  if (processor_id() == 0)
    for (const auto & type : _gof)
      for (unsigned int i = 0; i < _num_models; ++i)
      {
        if (type == "rmse")
          _gof_vectors[i]->push_back(std::sqrt(_sse[i] / static_cast<Real>(_data_size)));

        else if (type == "rsquared")
          _gof_vectors[i]->push_back(1.0 - (_sse[i] / _sst));

        else if (type == "adj-rsquared" || type == "fstatistic" || type == "pvalue")
        {
          Real r2 = 1.0 - (_sse[i] / _sst);
          Real n = _data_size;
          Real p = _model[i]->getDOF();
          if (n <= p)
            paramWarning("compute",
                         "Data size (",
                         n,
                         ") is equal to or smaller than model degrees of freedom (",
                         p,
                         "), ",
                         type,
                         " value might be incorrect.");
          if (type == "adj-rsquared")
            _gof_vectors[i]->push_back(1.0 - (1.0 - r2) * (n - 1.0) / (n - p));
          else if (type == "fstatistic")
            _gof_vectors[i]->push_back(r2 / (1.0 - r2) * (n - p) / (p - 1.0));
          else if (type == "pvalue")
          {
            if (r2 < 0.0 || r2 > 1.0)
            {
              mooseWarning(
                  "Model ", _model[i]->name(), " is poorly fitted, outputting NaN for pvalue.");
              _gof_vectors[i]->push_back(std::numeric_limits<double>::quiet_NaN());
            }
            else
            {
              Real fstat = r2 / (1.0 - r2) * (n - p) / (p - 1.0);
              unsigned int df1 = p - 1;
              unsigned int df2 = n - p;
              _gof_vectors[i]->push_back(1.0 - FDistribution::cdf(fstat, df1, df2));
            }
          }
        }
      }
}

void
GoodnessOfFit::preComputeStatistics()
{
  // SSTot
  if (isSSTNeeded())
    _sst = computeSST(*_data, _is_distributed);

  // SSError
  if (isSSENeeded())
  {
    _sse.resize(_num_models);
    for (unsigned int i = 0; i < _num_models; ++i)
      if (_sampler)
        _sse[i] = computeSSE(*_data, *(_model[i]), *_sampler, _is_distributed);
      else if (_is_distributed || processor_id() == 0)
        _sse[i] = computeSSE(*_data, *(_model_data[i]), _is_distributed);
  }
}

bool
GoodnessOfFit::isSSTNeeded() const
{
  if (_gof.contains("rsquared"))
    return true;
  else if (_gof.contains("adj-rsquared"))
    return true;
  else if (_gof.contains("fstatistic"))
    return true;
  else if (_gof.contains("pvalue"))
    return true;

  return false;
}

bool
GoodnessOfFit::isSSENeeded() const
{
  if (_gof.contains("rmse"))
    return true;
  else if (_gof.contains("rsquared"))
    return true;
  else if (_gof.contains("adj-rsquared"))
    return true;
  else if (_gof.contains("fstatistic"))
    return true;
  else if (_gof.contains("pvalue"))
    return true;

  return false;
}

Real
GoodnessOfFit::computeSST(const VectorPostprocessorValue & data, bool is_distributed)
{
  StochasticTools::Mean calc(*this);
  Real mean = calc.compute(data, is_distributed);

  Real sst = 0.0;
  for (unsigned int i = 0; i < data.size(); ++i)
  {
    Real ss = data[i] - mean;
    sst += ss * ss;
  }

  if (is_distributed)
    gatherSum(sst);

  return sst;
}

Real
GoodnessOfFit::computeSSE(const VectorPostprocessorValue & data,
                          const SurrogateModel & model,
                          Sampler & sampler,
                          bool is_distributed)
{
  dof_id_type offset = is_distributed ? sampler.getLocalRowBegin() : 0;

  Real sse = 0.0;
  for (unsigned int i = sampler.getLocalRowBegin(); i < sampler.getLocalRowEnd(); ++i)
  {
    Real res = data[i - offset] - model.evaluate(sampler.getNextLocalRow());
    sse += res * res;
  }

  gatherSum(sse);

  return sse;
}

Real
GoodnessOfFit::computeSSE(const VectorPostprocessorValue & data,
                          const VectorPostprocessorValue & model_data,
                          bool is_distributed)
{
  Real sse = 0.0;
  for (unsigned int i = 0; i < data.size(); ++i)
  {
    Real res = data[i] - model_data[i];
    sse += res * res;
  }

  if (is_distributed)
    gatherSum(sse);

  return sse;
}
