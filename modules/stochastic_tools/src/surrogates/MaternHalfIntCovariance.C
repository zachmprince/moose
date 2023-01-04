//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "MaternHalfIntCovariance.h"
#include <cmath>

registerMooseObject("StochasticToolsApp", MaternHalfIntCovariance);

InputParameters
MaternHalfIntCovariance::validParams()
{
  InputParameters params = CovarianceFunctionBase::validParams();
  params.addClassDescription("Matern half-integer covariance function.");
  params.makeParamRequired<std::vector<Real>>("length_factor");
  params.makeParamRequired<Real>("signal_variance");
  params.makeParamRequired<Real>("noise_variance");
  params.addRequiredParam<unsigned int>(
      "p", "Integer p to use for Matern Half Integer Covariance Kernel");
  return params;
}

MaternHalfIntCovariance::MaternHalfIntCovariance(const InputParameters & parameters)
  : CovarianceFunctionBase(parameters), _p(getParam<unsigned int>("p"))
{
  _tunable_hp.insert("noise_variance");
  _tunable_hp.insert("signal_variance");
  _tunable_hp.insert("length_factor");
}

void
MaternHalfIntCovariance::buildAdditionalHyperParamMap(
    std::unordered_map<std::string, Real> & map,
    std::unordered_map<std::string, std::vector<Real>> & /*vec_map*/) const
{
  map["p"] = _p;
}

void
MaternHalfIntCovariance::loadAdditionalHyperParamMap(
    std::unordered_map<std::string, Real> & map,
    std::unordered_map<std::string, std::vector<Real>> & /*vec_map*/)
{
  _p = map["p"];
}

void
MaternHalfIntCovariance::computeCovarianceMatrix(RealEigenMatrix & K,
                                                 const RealEigenMatrix & x,
                                                 const RealEigenMatrix & xp,
                                                 const bool is_self_covariance) const
{
  if ((unsigned)x.cols() != _length_factor.size())
    mooseError("length_factor size does not match dimension of trainer input.");

  maternHalfIntFunction(
      K, x, xp, _length_factor, _sigma_f_squared, _sigma_n_squared, _p, is_self_covariance);
}

void
MaternHalfIntCovariance::computedKdx(RealEigenMatrix & dKdx,
                                     const RealEigenMatrix & x,
                                     const RealEigenMatrix & xp,
                                     unsigned int ind) const
{
  // This factor is used over and over, don't calculate each time
  const Real factor = sqrt(2 * _p + 1);

  for (unsigned int ii = 0; ii < x.rows(); ++ii)
    for (unsigned int jj = 0; jj < xp.rows(); ++jj)
    {
      // Compute distance per parameter, scaled by length factor
      Real r_scaled = 0;
      for (unsigned int kk = 0; kk < x.cols(); ++kk)
      {
        const Real diff = (x(ii, kk) - xp(jj, kk)) / _length_factor[kk];
        r_scaled += diff * diff;
      }
      r_scaled = sqrt(r_scaled);

      // Compute the polynomial term and its derivative
      // tgamma(x+1) == x! when x is a natural number, which should always be the case for
      // MaternHalfInt
      Real sum = 0;
      Real dsum = 0;
      for (unsigned int pp = 0; pp <= _p; ++pp)
      {
        const Real pfact = (tgamma(_p + pp + 1) / (tgamma(pp + 1) * tgamma(_p - pp + 1))) *
                           pow(2.0 * factor * r_scaled, _p - pp);
        sum += pfact;
        dsum += pfact / r_scaled * (_p - pp);
      }

      // Compute the exponential term and its derivative
      const Real pexp =
          _sigma_f_squared * std::exp(-factor * r_scaled) * (tgamma(_p + 1) / (tgamma(2 * _p + 1)));
      dKdx(ii, jj) = -factor * pexp * sum + pexp * dsum;

      // Apply derivative of distance
      dKdx(ii, jj) *=
          (xp(jj, ind) - x(ii, ind)) / (_length_factor[ind] * _length_factor[ind]) / r_scaled;
    }
}

void
MaternHalfIntCovariance::maternHalfIntFunction(RealEigenMatrix & K,
                                               const RealEigenMatrix & x,
                                               const RealEigenMatrix & xp,
                                               const std::vector<Real> & length_factor,
                                               const Real sigma_f_squared,
                                               const Real sigma_n_squared,
                                               const unsigned int p,
                                               const bool is_self_covariance)
{
  unsigned int num_samples_x = x.rows();
  unsigned int num_samples_xp = xp.rows();
  unsigned int num_params_x = x.cols();

  mooseAssert(num_params_x == xp.cols(),
              "Number of parameters do not match in covariance kernel calculation");

  // This factor is used over and over, don't calculate each time
  Real factor = sqrt(2 * p + 1);

  for (unsigned int ii = 0; ii < num_samples_x; ++ii)
  {
    for (unsigned int jj = 0; jj < num_samples_xp; ++jj)
    {
      // Compute distance per parameter, scaled by length factor
      Real r_scaled = 0;
      for (unsigned int kk = 0; kk < num_params_x; ++kk)
        r_scaled += pow((x(ii, kk) - xp(jj, kk)) / length_factor[kk], 2);
      r_scaled = sqrt(r_scaled);
      // tgamma(x+1) == x! when x is a natural number, which should always be the case for
      // MaternHalfInt
      Real summation = 0;
      for (unsigned int tt = 0; tt < p + 1; ++tt)
        summation += (tgamma(p + tt + 1) / (tgamma(tt + 1) * tgamma(p - tt + 1))) *
                     pow(2 * factor * r_scaled, p - tt);
      K(ii, jj) = sigma_f_squared * std::exp(-factor * r_scaled) *
                  (tgamma(p + 1) / (tgamma(2 * p + 1))) * summation;
    }
    if (is_self_covariance)
      K(ii, ii) += sigma_n_squared;
  }
}

void
MaternHalfIntCovariance::computedKdhyper(RealEigenMatrix & dKdhp,
                                         const RealEigenMatrix & x,
                                         std::string hyper_param_name,
                                         unsigned int ind) const
{
  if (hyper_param_name == "noise_variance")
    maternHalfIntFunction(dKdhp, x, x, _length_factor, 0, 1, _p, true);

  if (hyper_param_name == "signal_variance")
    maternHalfIntFunction(dKdhp, x, x, _length_factor, 1, 0, _p, false);

  if (hyper_param_name == "length_factor")
    computedKdlf(dKdhp, x, _length_factor, _sigma_f_squared, _p, ind);
}

void
MaternHalfIntCovariance::computedKdlf(RealEigenMatrix & K,
                                      const RealEigenMatrix & x,
                                      const std::vector<Real> & length_factor,
                                      const Real sigma_f_squared,
                                      const unsigned int p,
                                      const int ind)
{
  unsigned int num_samples_x = x.rows();
  unsigned int num_params_x = x.cols();

  mooseAssert(ind < x.cols(), "Incorrect length factor index");

  // This factor is used over and over, don't calculate each time
  Real factor = sqrt(2 * p + 1);

  for (unsigned int ii = 0; ii < num_samples_x; ++ii)
  {
    for (unsigned int jj = 0; jj < num_samples_x; ++jj)
    {
      // Compute distance per parameter, scaled by length factor
      Real r_scaled = 0;
      for (unsigned int kk = 0; kk < num_params_x; ++kk)
        r_scaled += pow((x(ii, kk) - x(jj, kk)) / length_factor[kk], 2);
      r_scaled = sqrt(r_scaled);
      if (r_scaled != 0)
      {
        // product rule to compute dK/dr_scaled
        // u'v
        Real summation = 0;
        for (unsigned int tt = 0; tt < p + 1; ++tt)
          summation += (tgamma(p + tt + 1) / (tgamma(tt + 1) * tgamma(p - tt + 1))) *
                       pow(2 * factor * r_scaled, p - tt);
        K(ii, jj) = -factor * std::exp(-factor * r_scaled) * summation;
        // uv'
        // dont need tt=p, (p-tt) factor ->0. Also avoids unsigned integer subtraction wraparound
        summation = 0;
        for (unsigned int tt = 0; tt < p; ++tt)
          summation += (tgamma(p + tt + 1) / (tgamma(tt + 1) * tgamma(p - tt + 1))) * 2 * factor *
                       (p - tt) * pow(2 * factor * r_scaled, p - tt - 1);
        K(ii, jj) += std::exp(-factor * r_scaled) * summation;
        // Apply chain rule for dr_scaled/dl_i
        K(ii, jj) *= -std::pow(x(ii, ind) - x(jj, ind), 2) /
                     (std::pow(length_factor[ind], 3) * r_scaled) * sigma_f_squared *
                     (tgamma(p + 1) / (tgamma(2 * p + 1)));
      }
      else // avoid div by 0. 0/0=0 scenario.
        K(ii, jj) = 0;
    }
  }
}
