//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "PolynomialChaosTrainer.h"
#include "Sampler.h"
#include "CartesianProduct.h"

registerMooseObject("StochasticToolsApp", PolynomialChaosTrainer);

InputParameters
PolynomialChaosTrainer::validParams()
{
  InputParameters params = SamplerTrainer::validParams();
  params.addClassDescription("Computes and evaluates polynomial chaos surrogate model.");
  params.addRequiredParam<unsigned int>("order", "Maximum polynomial order.");
  params.addRequiredParam<std::vector<DistributionName>>(
      "distributions", "Names of the distributions samples were taken from.");

  return params;
}

PolynomialChaosTrainer::PolynomialChaosTrainer(const InputParameters & parameters)
  : SamplerTrainer(parameters),
    _order(declareModelData<unsigned int>("_order", getParam<unsigned int>("order"))),
    _ndim(declareModelData<unsigned int>("_ndim")),
    _ncoeff(declareModelData<std::size_t>("_ncoeff")),
    _tuple(declareModelData<std::vector<std::vector<unsigned int>>>("_tuple")),
    _coeff(declareModelData<std::vector<Real>>("_coeff")),
    _poly(declareModelData<std::vector<std::unique_ptr<const PolynomialQuadrature::Polynomial>>>(
        "_poly")),
    _hatval(declareModelData<std::vector<Real>>("_hatval"))
{
}

void
PolynomialChaosTrainer::initialSetup()
{
  SamplerTrainer::initialSetup();

  // Setup data needed for training
  _tuple = StochasticTools::MultiDimPolynomialGenerator::generateTuple(
      getParam<std::vector<DistributionName>>("distributions").size(), _order);
  _ncoeff = _tuple.size();
  _ndim = getNumberOfParameters();

  // Special circumstances if sampler is quadrature
  _quad_sampler = dynamic_cast<const QuadratureSampler *>(&sampler());

  // Check if sampler dimension matches number of distributions
  std::vector<DistributionName> dname = getParam<std::vector<DistributionName>>("distributions");
  if (dname.size() != _ndim)
    mooseError("Sampler number of columns does not match number of inputted distributions.");
  for (auto nm : dname)
    _poly.push_back(PolynomialQuadrature::makePolynomial(&getDistributionByName(nm)));
}

void
PolynomialChaosTrainer::preTrain()
{
  _coeff.resize(_ncoeff, 0);

  // Leverages
  _hatval.resize(getNumberOfLocalPoints(), 0.0);
}

void
PolynomialChaosTrainer::train()
{
  // Evaluate polynomials to avoid duplication
  DenseMatrix<Real> poly_val(_ndim, _order);
  for (unsigned int d = 0; d < _ndim; ++d)
    for (unsigned int i = 0; i < _order; ++i)
      poly_val(d, i) = _poly[d]->compute(i, _data[d]);

  // Loop over coefficients
  for (std::size_t i = 0; i < _ncoeff; ++i)
  {
    Real val = 1.0;
    // Loop over parameters
    for (std::size_t d = 0; d < _ndim; ++d)
      val *= poly_val(d, _tuple[i][d]);

    // Compute levarage
    Real valn = 1.0;
    for (std::size_t d = 0; d < _ndim; ++d)
      valn *= poly_val(d, _tuple[i][d]) * _poly[d]->innerProduct(_tuple[i][d]);
    _hatval[_local_p] += valn * val;

    if (_quad_sampler)
      val *= _quad_sampler->getQuadratureWeight(_row);
    _coeff[i] += val * _val;
  }

  if (_quad_sampler)
    _hatval[_local_p] *= _quad_sampler->getQuadratureWeight(_row);
  else
    _hatval[_local_p] /= getNumberOfPoints();
}

void
PolynomialChaosTrainer::postTrain()
{
  gatherSum(_coeff);

  if (!_quad_sampler)
  {
    for (std::size_t i = 0; i < _ncoeff; ++i)
      _coeff[i] /= getNumberOfPoints();
    _communicator.allgather(_hatval);
  }
}
