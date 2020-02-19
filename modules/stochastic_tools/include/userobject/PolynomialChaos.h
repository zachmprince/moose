//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "SurrogateModel.h"
#include "PolynomialQuadrature.h"
#include "QuadratureSampler.h"

#include "Distribution.h"

// Forward Declarations
class PolynomialChaos;

template <>
InputParameters validParams<PolynomialChaos>();

class PolynomialChaos : public SurrogateModel
{
public:
  static InputParameters validParams();

  PolynomialChaos(const InputParameters & parameters);

  virtual void initialSetup() override;

  virtual void initialize() override {}
  virtual void execute() override;
  virtual void finalize() override;
  virtual void threadJoin(const UserObject & y) override;

  /// Number of terms in expansion
  unsigned int getNumberofCoefficients() const { return _ncoeff; }
  /// Access polynomial orders from tuple
  ////@{
  std::vector<std::vector<unsigned int>> getPolynomialOrders() const { return _tuple; }
  unsigned int getPolynomialOrder(const unsigned int dim, const unsigned int i) const
  {
    return _tuple[i][dim];
  }
  ///@}
  /// Access computed expansion coefficients
  const std::vector<Real> & getCoefficients() const { return _coeff; }

  /// Evaluate expansion
  virtual Real evaluate(const std::vector<Real> & x) const override;

protected:
  /**
   * Function computing for computing _tuple
   * Example for ndim = 3, order = 4:
   * | 0 | 1 0 0 | 2 1 1 0 0 0 | 3 2 2 1 1 1 0 0 0 0 |
   * | 0 | 0 1 0 | 0 1 0 2 1 0 | 0 1 0 2 1 0 3 2 1 0 |
   * | 0 | 0 0 1 | 0 0 1 0 1 2 | 0 0 1 0 1 2 0 1 2 3 |
   */
  static std::vector<std::vector<unsigned int>> generateTuple(const unsigned int ndim,
                                                              const unsigned int order);
  /// Utility function that computes the full tensor tuple
  static std::vector<std::vector<unsigned int>> computeTensorTuple(const unsigned int ndim,
                                                                   const unsigned int order);
  /// Tuple sorter function
  static bool sortTuple(const std::vector<unsigned int> & first,
                        const std::vector<unsigned int> & second);

  /// Maximum polynomial order. The sum of the one-dimensional polynomial orders does not go above this value.
  const unsigned int _order;
  /// QuadratureSampler pointer, necessary for applying quadrature weights
  QuadratureSampler * _quad_sampler;
  /// A _ndim-by-_ncoeff matrix containing the appropriate one-dimensional polynomial order
  const std::vector<std::vector<unsigned int>> _tuple;
  /// Total number of coefficient (defined by size of _tuple)
  const unsigned int _ncoeff;
  /// These are the coefficients we are after in the PC expansion
  std::vector<Real> _coeff;
  /// The distributions used for sampling
  std::vector<std::unique_ptr<const PolynomialQuadrature::Polynomial>> _poly;
};
