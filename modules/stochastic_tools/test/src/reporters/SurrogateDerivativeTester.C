//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

// Stochastic Tools Includes
#include "SurrogateDerivativeTester.h"

#include "SurrogateModel.h"
#include "Sampler.h"

registerMooseObject("StochasticToolsTestApp", SurrogateDerivativeTester);
registerMooseObject("StochasticToolsTestApp", VectorSurrogateDerivativeTester);

template <typename ResponseType>
InputParameters
SurrogateDerivativeTesterTempl<ResponseType>::validParams()
{
  InputParameters params = StochasticReporter::validParams();
  params += SurrogateModelInterface::validParams();
  params.addClassDescription("Tool for sampling surrogate model derivatives.");
  params.addRequiredParam<UserObjectName>("model", "Name of surrogate model.");
  params.addRequiredParam<SamplerName>("sampler", "Sampler to use for evaluating model.");
  return params;
}

template <typename ResponseType>
SurrogateDerivativeTesterTempl<ResponseType>::SurrogateDerivativeTesterTempl(
    const InputParameters & parameters)
  : StochasticReporter(parameters),
    SurrogateModelInterface(this),
    _model(getSurrogateModel("model")),
    _sampler(getSampler("sampler")),
    _values(declareStochasticReporter<std::vector<ResponseType>>(_model.name(), _sampler)),
    _fd_values(
        declareStochasticReporter<std::vector<ResponseType>>(_model.name() + "_fd", _sampler))
{
}

template <typename ResponseType>
void
SurrogateDerivativeTesterTempl<ResponseType>::execute()
{
  for (dof_id_type p = 0; p < _sampler.getNumberOfLocalRows(); ++p)
  {
    std::vector<Real> data = _sampler.getNextLocalRow();
    _model.evaluateDerivative(data, _values[p]);
    evaluateFiniteDifference(_model, data, _fd_values[p]);
  }
}

template <>
void
SurrogateDerivativeTesterTempl<Real>::evaluateFiniteDifference(const SurrogateModel & model,
                                                               const std::vector<Real> & x,
                                                               std::vector<Real> & fd_dydx) const
{
  const Real y = model.evaluate(x);
  fd_dydx.resize(x.size());
  const Real eps = 1.0e-6;
  for (unsigned int j = 0; j < x.size(); ++j)
  {
    std::vector<Real> x_eps = x;
    x_eps[j] += eps;
    const Real y_eps = model.evaluate(x_eps);
    fd_dydx[j] = (y_eps - y) / (x_eps[j] - x[j]);
  }
}

template <>
void
SurrogateDerivativeTesterTempl<std::vector<Real>>::evaluateFiniteDifference(
    const SurrogateModel & model,
    const std::vector<Real> & x,
    std::vector<std::vector<Real>> & fd_dydx) const
{
  std::vector<Real> y;
  model.evaluate(x, y);
  fd_dydx.assign(x.size(), y);
  std::vector<Real> y_eps(y.size());
  const Real eps = 1.0e-6;
  for (unsigned int j = 0; j < x.size(); ++j)
  {
    std::vector<Real> x_eps = x;
    x_eps[j] += eps;
    model.evaluate(x_eps, y_eps);

    for (const auto & r : index_range(y))
      fd_dydx[j][r] = (y_eps[r] - y[r]) / (x_eps[j] - x[j]);
  }
}

template class SurrogateDerivativeTesterTempl<Real>;
template class SurrogateDerivativeTesterTempl<std::vector<Real>>;
