//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "LayeredAverage.h"
#include "VectorPostprocessor.h"

class LayerAverage;

template <>
InputParameters validParams<LayerAverage>();

/**
 * Compute a layer average of a variable as a function of distance throughout the
 * simulation domain.
 */
class LayerAverage : public LayeredAverage, public VectorPostprocessor
{
public:
  static InputParameters validParams();

  LayerAverage(const InputParameters & parameters);

protected:
  /// compute the distance of the current quadrature point for binning
  virtual void finalize() override;

private:
  const Real & _scale_factor;
  VectorPostprocessorValue & _points_vector;
  VectorPostprocessorValue & _values_vector;
};
