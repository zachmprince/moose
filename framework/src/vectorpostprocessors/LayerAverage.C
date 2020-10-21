//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "LayerAverage.h"

registerMooseObject("MooseApp", LayerAverage);

defineLegacyParams(LayerAverage);

InputParameters
LayerAverage::validParams()
{
  InputParameters params = LayeredAverage::validParams();
  params += VectorPostprocessor::validParams();
  params.addClassDescription("Computes the average value of variables in layers along a specified direction.");
  params.addParam<Real>("scale_factor", 1.0, "Factor to scale average of all layers by.");
  return params;
}

LayerAverage::LayerAverage(const InputParameters & parameters)
  : LayeredAverage(parameters),
    VectorPostprocessor(parameters),
    _scale_factor(getParam<Real>("scale_factor")),
    _points_vector(declareVector("point_" + std::string(_direction_enum))),
    _values_vector(declareVector("value"))
{
  _points_vector.resize(_num_layers);
  _values_vector.resize(_num_layers);
}

void
LayerAverage::finalize()
{
  LayeredAverage::finalize();

  Real dx = (_direction_max - _direction_min) / static_cast<Real>(_num_layers);
  for (unsigned int i = 0; i < _num_layers; ++i)
  {
    _values_vector[i] = getLayerValue(i) * _scale_factor;
    if (_interval_based)
      _points_vector[i] = dx * (static_cast<Real>(i) + 0.5);
    else
      _points_vector[i] = (_layer_bounds[i + 1] + _layer_bounds[i]) / 2.0;
  }
}
