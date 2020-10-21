//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "GeneralVectorPostprocessor.h"

// Forward Declarations
class SumVectors;

template <>
InputParameters validParams<SumVectors>();

class SumVectors : public GeneralVectorPostprocessor
{
public:
  static InputParameters validParams();

  SumVectors(const InputParameters & parameters);

  virtual void initialize() override;
  virtual void execute() override;

protected:
  /// Resulting vector from summing input vectors
  VectorPostprocessorValue & _values;

private:
  /// Names of inputted vector postprocessors
  const std::vector<VectorPostprocessorName> & _vpp_names;
  /// Names of inputter vectors (for each vpp)
  const std::vector<std::string> & _vector_names;
  /// Number of vectors to sum
  const unsigned int _num_vector;
  /// Scaling factor postprocessor names
  const std::vector<Real> & _scale_factors;

  /// Inputted vector values
  std::vector<const VectorPostprocessorValue *> _input_vectors;
};
