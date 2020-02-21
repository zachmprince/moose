//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "UniformDistribution.h"
#include "NormalDistribution.h"
#include "BoostNormalDistribution.h"

/**
 * Polynomials and quadratures based on defined distributions for Polynomial Chaos
 */
namespace PolynomialQuadrature
{
class Polynomial;
class Quadrature;

std::unique_ptr<const Polynomial> makePolynomial(const Distribution * dist);
std::unique_ptr<const Quadrature> makeQuadrature(const Distribution * dist);

/**
 * General polynomial class with function for evaluating a polynomial of a given
 * order at a given point
 */
class Polynomial
{
public:
  Polynomial() {}
  virtual ~Polynomial() = default;
  virtual Real compute(const unsigned int order, const Real x, const bool normalize = true) const;
};

/**
 * Uniform distributions use Legendre polynomials
 */
class Legendre : public Polynomial
{
public:
  Legendre(const UniformDistribution * dist);
  virtual Real
  compute(const unsigned int order, const Real x, const bool normalize = true) const override;

private:
  Real _lower_bound;
  Real _upper_bound;
};

Real legendre(const unsigned int order,
              const Real x,
              const Real lower_bound = -1.0,
              const Real upper_bound = 1.0);

/**
 * Normal distributions use Hermite polynomials
 */
class Hermite : public Polynomial
{
public:
  Hermite(const NormalDistribution * dist);
  Hermite(const BoostNormalDistribution * dist);
  virtual Real
  compute(const unsigned int order, const Real x, const bool normalize = true) const override;

private:
  Real _mu;
  Real _sig;
};

Real hermite(const unsigned int order,
             const Real x,
             const Real mu = 0.0,
             const Real sig = 1.0);

/**
 * General quadrature class with functions for getting weights and points
 */
class Quadrature
{
public:
  Quadrature() {}
  virtual ~Quadrature() = default;
  virtual std::vector<Real> points(const unsigned int order) const = 0;
  virtual std::vector<Real> weights(const unsigned int order) const = 0;
};

/**
 * Uniform distributions use Gauss-Legendre quadrature
 */
class GaussLegendre : public Quadrature
{
public:
  GaussLegendre(const UniformDistribution * dist);
  virtual std::vector<Real> points(const unsigned int order) const;
  virtual std::vector<Real> weights(const unsigned int order) const;

private:
  Real _lower_bound;
  Real _upper_bound;
};

void gauss_legendre(const unsigned int order,
                    std::vector<Real> & points,
                    std::vector<Real> & weights,
                    const Real lower_bound,
                    const Real upper_bound);
}
