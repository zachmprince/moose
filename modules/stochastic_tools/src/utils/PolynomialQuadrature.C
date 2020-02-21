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
#include "libmesh/utility.h"

// For computing legendre quadrature
#include "libmesh/quadrature_gauss.h"

// For quickly computing polynomials
#ifdef LIBMESH_HAVE_EXTERNAL_BOOST
#include <boost/math/special_functions/legendre.hpp>
#include <boost/math/special_functions/hermite.hpp>
#endif

#include <cmath>

const Real SQRT_2PI = M_2_SQRTPI * M_PI * M_SQRT1_2;

namespace PolynomialQuadrature
{

std::unique_ptr<const Polynomial>
makePolynomial(const Distribution * dist)
{
  const UniformDistribution * u_dist = dynamic_cast<const UniformDistribution *>(dist);
  if (u_dist)
    return libmesh_make_unique<const Legendre>(u_dist);

  const NormalDistribution * n_dist = dynamic_cast<const NormalDistribution *>(dist);
  if (n_dist)
    return libmesh_make_unique<const Hermite>(n_dist);

  const BoostNormalDistribution * bn_dist = dynamic_cast<const BoostNormalDistribution *>(dist);
  if (bn_dist)
    return libmesh_make_unique<const Hermite>(bn_dist);

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
#ifdef LIBMESH_HAVE_EXTERNAL_BOOST
  return boost::math::legendre_p(order, xref);
#else
  if (order == 0)
    return 1.0;
  else if (order == 1)
    return xref;
  else
  {
    Real m = (Real) order;
    return ((2.0 * m - 1.0) * xref * legendre(order - 1, xref) -
            (m - 1) * legendre(order - 2, xref)) / m;
  }
#endif
}

Hermite::Hermite(const NormalDistribution * dist)
  : Polynomial(), _mu(dist->getMean()), _sig(dist->getStdDev())
{
}


Hermite::Hermite(const BoostNormalDistribution * dist)
  : Polynomial(), _mu(dist->getMean()), _sig(dist->getStdDev())
{
}

Real
Hermite::compute(const unsigned int order, const Real x, const bool normalize) const
{
  Real val = hermite(order, x, _mu, _sig);
  // val /= sqrt(2pi)order!
  if (normalize)
    val /= (Real)Utility::factorial(order);
  //   val /= SQRT_2PI * (Real)Utility::factorial(order);

  return val;
}

Real
hermite(const unsigned int order, const Real x, const Real mu, const Real sig)
{
  Real xref = (x - mu) / sig;
#ifdef LIBMESH_HAVE_EXTERNAL_BOOST
  // Need to do some modification since boost does physicists hermite polynomials:
  // H_n^prob(x) = 2^(-n/2)H_n^phys(x / sqrt(2))
  xref /= M_SQRT2; // 1 / sqrt(2)
  Real val = boost::math::hermite(order, xref);
  val /= pow(M_SQRT2, order); // 2^(-order / 2)
  return val;
#else
  if (order == 0)
    return 1.0;
  else if (order == 1)
    return xref;
  else
    return xref * hermite(order - 1, xref) - ((Real)order - 1.0) * hermite(order - 2, xref);
#endif
}

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

void
gauss_hermite(const unsigned int order,
              std::vector<Real> & points,
              std::vector<Real> & weights,
              const Real mu,
              const Real sig)
{
  // Number of points needed
  Real npts = ((Real)order + 1.0) / 2.0;

  // These are physicists hermite points and weights taken from:
  // https://keisan.casio.com/exec/system/1281195844
  if (npts <= 2)
  {
    points.resize(2);
    weights.resize(2);

    points[0] = -0.707106781186547524400844362105;
    points[1] = -points[0];

    weights[0] = 0.886226925452758013649083741671;
    weights[1] = weights[0];
  }
  else if (npts <= 3)
  {
    points.resize(3);
    weights.resize(3);

    points[0] = -1.22474487139158904909864203735;
    points[1] = 0.0;
    points[2] = -points[0];

    weights[0] = 0.29540897515091933788302791389;
    weights[1] = 1.18163590060367735153211165556;
    weights[2] = weights[0];
  }
  else if (npts <= 4)
  {
    points.resize(4);
    weights.resize(4);

    points[0] = -1.65068012388578455588334111112;
    points[1] = -0.524647623275290317884060253835;
    points[2] = -points[1];
    points[3] = -points[0];

    weights[0] = 0.08131283544724517714303455719;
    weights[1] = 0.804914090005512836506049184481;
    weights[2] = weights[1];
    weights[3] = weights[0];
  }
  else if (npts <= 5)
  {
    points.resize(5);
    weights.resize(5);

    points[0] = -2.02018287045608563292872408814;
    points[1] = -0.958572464613818507112770593893;
    points[2] = 0.0;
    points[3] = -points[1];
    points[4] = -points[0];

    weights[0] = 0.0199532420590459132077434585942;
    weights[1] = 0.393619323152241159828495620852;
    weights[2] = 0.945308720482941881225689324449;
    weights[3] = weights[1];
    weights[4] = weights[0];
  }
  else if (npts <= 6)
  {
    points.resize(6);
    weights.resize(6);

    points[0] = -2.35060497367449222283392198706;
    points[1] = -1.33584907401369694971489528297;
    points[2] = -0.436077411927616508679215948251;
    points[3] = -points[2];
    points[4] = -points[1];
    points[5] = -points[0];

    weights[0] = 0.00453000990550884564085747256463;
    weights[1] = 0.157067320322856643916311563508;
    weights[2] = 0.724629595224392524091914705598;
    weights[3] = weights[2];
    weights[4] = weights[1];
    weights[5] = weights[0];
  }
} // namespace PolynomialQuadrature
