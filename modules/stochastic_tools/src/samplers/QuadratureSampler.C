//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "QuadratureSampler.h"
#include "Distribution.h"
#include "PolynomialQuadrature.h"

registerMooseObject("StochasticToolsApp", QuadratureSampler);

defineLegacyParams(QuadratureSampler);

InputParameters
QuadratureSampler::validParams()
{
  InputParameters params = Sampler::validParams();
  params.addClassDescription("Quadrature sampler for Polynomial Chaos.");
  params.addRequiredParam<unsigned int>(
      "order", "Specify the maximum order of the polynomials in the expansion.");
  MooseEnum grid("none", "none");
  params.addParam<MooseEnum>(
      "sparse_grid", grid, "Type of sparse grid to use, if none, full tensor product is used.");
  params.addRequiredParam<std::vector<DistributionName>>(
      "distributions",
      "The distribution names to be sampled, the number of distributions provided defines the "
      "number of columns per matrix and their type defines the quadrature.");
  return params;
}

QuadratureSampler::QuadratureSampler(const InputParameters & parameters) : Sampler(parameters)
{
  // For each distribution, get the 1-D quadrature
  std::vector<DistributionName> dname = getParam<std::vector<DistributionName>>("distributions");
  std::vector<std::vector<Real>> qpoints_1D;
  std::vector<std::vector<Real>> qweights_1D;
  for (const DistributionName & name : dname)
  {
    std::unique_ptr<const PolynomialQuadrature::Quadrature> quad =
        PolynomialQuadrature::makeQuadrature(&getDistributionByName(name));
    qpoints_1D.push_back(quad->points(getParam<unsigned int>("order")));
    qweights_1D.push_back(quad->weights(getParam<unsigned int>("order")));
  }

  // Here, we take the 1-D quadratures and perform a tensor product for multi-D integration
  switch (getParam<MooseEnum>("sparse_grid"))
  {
    case 0:
    {
      tensorGrid(qpoints_1D, qweights_1D, _qpoints, _qweights);
      break;
    }
      paramError("sparse_grid", "Unknown or unimplemented sparse grid type.");
  }

  // We need to make sure the
  Real qtot = std::accumulate(_qweights.get_values().begin(), _qweights.get_values().end(), 0.0);
  _qweights.scale(1.0 / qtot);

  setNumberOfRows(_qpoints.m());
  setNumberOfCols(_qpoints.n());
}

Real
QuadratureSampler::computeSample(dof_id_type row_index, dof_id_type col_index)
{
  return _qpoints(row_index, col_index);
}

Real
QuadratureSampler::getQuadratureWeight(dof_id_type row_index) const
{
  return _qweights(row_index);
}

void
QuadratureSampler::tensorGrid(const std::vector<std::vector<Real>> & points_1D,
                              const std::vector<std::vector<Real>> & weights_1D,
                              DenseMatrix<Real> & points,
                              DenseVector<Real> & weights)
{
  unsigned int ndim = points_1D.size();
  if (ndim == 1)
  {
    points.resize(points_1D[0].size(), 1);
    weights.resize(points_1D[0].size());
    for (unsigned int p = 0; p < points_1D[0].size(); ++p)
    {
      points(p, 0) = points_1D[0][p];
      weights(p) = weights_1D[0][p];
    }
  }
  else
  {
    std::vector<std::vector<Real>> tmpp(points_1D.begin(), points_1D.end() - 1);
    std::vector<std::vector<Real>> tmpw(weights_1D.begin(), weights_1D.end() - 1);
    DenseMatrix<Real> points_old;
    DenseVector<Real> weights_old;
    tensorGrid(tmpp, tmpw, points_old, weights_old);

    points.resize(points_old.m() * points_1D[ndim - 1].size(), ndim);
    weights.resize(points.m());
    unsigned int k = 0;
    for (unsigned int p = 0; p < points_1D[ndim - 1].size(); ++p)
      for (unsigned int i = 0; i < points_old.m(); ++i)
      {
        weights(k) = weights_1D[ndim - 1][p] * weights_old(i);
        for (unsigned int d = 0; d < ndim - 1; ++d)
          points(k, d) = points_old(i, d);
        points(k++, ndim - 1) = points_1D[ndim - 1][p];
      }
  }
}
