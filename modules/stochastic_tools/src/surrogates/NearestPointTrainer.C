//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "NearestPointTrainer.h"

registerMooseObject("StochasticToolsApp", NearestPointTrainer);

InputParameters
NearestPointTrainer::validParams()
{
  InputParameters params = SamplerTrainer::validParams();
  params.addClassDescription("Loops over and saves sample values for [NearestPointSurrogate.md].");
  return params;
}

NearestPointTrainer::NearestPointTrainer(const InputParameters & parameters)
  : SamplerTrainer(parameters),
    _sample_points(declareModelData<std::vector<std::vector<Real>>>("_sample_points"))
{
}

void
NearestPointTrainer::preTrain()
{
  // Resize to number of sample points
  _sample_points.resize(getNumberOfParameters() + 1);
  for (auto & it : _sample_points)
    it.resize(getNumberOfLocalPoints());
}

void
NearestPointTrainer::train()
{
  for (unsigned int i = 0; i < _data.size(); ++i)
    _sample_points[i][_local_p] = _data[i];
  _sample_points[_data.size()][_local_p] = _val;
}

void
NearestPointTrainer::postTrain()
{
  for (auto & it : _sample_points)
    _communicator.allgather(it);
}
