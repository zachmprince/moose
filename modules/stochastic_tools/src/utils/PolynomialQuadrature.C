//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "PolynomialQuadrature.h"

#include "MooseError.h"
#include "libmesh/auto_ptr.h"

// For computing legendre polynomials
#include "libmesh/quadrature_gauss.h"
#ifdef LIBMESH_HAVE_EXTERNAL_BOOST
#include <boost/math/special_functions/legendre.hpp>
#endif

namespace PolynomialQuadrature
{

std::unique_ptr<const Polynomial>
makePolynomial(const Distribution * dist)
{
  const UniformDistribution * u_dist = dynamic_cast<const UniformDistribution *>(dist);
  if (u_dist)
    return libmesh_make_unique<const Legendre>(u_dist);

  ::mooseError("Polynomials for '", dist->type(), "' distributions have not been implemented.");
  return nullptr;
}

std::unique_ptr<const Quadrature>
makeQuadrature(const Distribution * dist)
{
  const UniformDistribution * u_dist = dynamic_cast<const UniformDistribution *>(dist);
  if (u_dist)
    return libmesh_make_unique<const GaussLegendre>(u_dist);

  ::mooseError("Quadrature for '", dist->type(), "' distributions have not been implemented.");
  return nullptr;
}

Real
Polynomial::compute(const unsigned int /*order*/, const Real /*x*/, const bool /*normalize*/) const
{
  ::mooseError("Polynomial type has not been implemented.");
  return 0;
}

Legendre::Legendre(const UniformDistribution * dist)
  : Polynomial(), _lower_bound(dist->getLowerBound()), _upper_bound(dist->getUpperBound())
{
}

#ifdef LIBMESH_HAVE_EXTERNAL_BOOST

Real
Legendre::compute(const unsigned int order, const Real x, const bool normalize) const
{
  if ((x < _lower_bound) || (x > _upper_bound))
    ::mooseError("The requested polynomial point is outside of distribution bounds.");

  Real val = legendre(order, x, _lower_bound, _upper_bound);
  if (normalize)
    val /= 1.0 / (2.0 * (Real)order + 1);

  return val;
}

Real
legendre(const unsigned int order, const Real x, const Real lower_bound, const Real upper_bound)
{
  Real xref = 2 / (upper_bound - lower_bound) * (x - (upper_bound + lower_bound) / 2);
  return boost::math::legendre_p(order, xref);
}

#endif

GaussLegendre::GaussLegendre(const UniformDistribution * dist)
  : Quadrature(), _lower_bound(dist->getLowerBound()), _upper_bound(dist->getUpperBound())
{
}

std::vector<Real>
GaussLegendre::points(const unsigned int order) const
{
  std::vector<Real> points;
  std::vector<Real> weights;

  gauss_legendre(order, points, weights, _lower_bound, _upper_bound);

  return points;
}

std::vector<Real>
GaussLegendre::weights(const unsigned int order) const
{
  std::vector<Real> points;
  std::vector<Real> weights;

  gauss_legendre(order, points, weights, _lower_bound, _upper_bound);

  return weights;
}

void
gauss_legendre(const unsigned int order,
               std::vector<Real> & points,
               std::vector<Real> & weights,
               const Real lower_bound,
               const Real upper_bound)
{
  // Gauss quadrature is only defined for odd order
  Order order_enum = ((order % 2) == 0 ? static_cast<Order>(order + 1) : static_cast<Order>(order));

  std::unique_ptr<QBase> quad =
      QBase::build(Moose::stringToEnum<QuadratureType>("GAUSS"), 1, order_enum);

  points.resize(quad->n_points());
  Real dx = (upper_bound - lower_bound) / 2.0;
  Real xav = (upper_bound + lower_bound) / 2.0;
  for (unsigned int n = 0; n < quad->n_points(); ++n)
    points[n] = quad->get_points()[n](0) * dx + xav;
  weights = quad->get_weights();
}

} // namespace PolynomialQuadrature
