//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "SamplerTrainer.h"

class NearestPointTrainer : public SamplerTrainer
{
public:
  static InputParameters validParams();
  NearestPointTrainer(const InputParameters & parameters);

protected:
  virtual void preTrain() override;
  virtual void train() override;
  virtual void postTrain() override;

  /// Map containing sample points and the results
  std::vector<std::vector<Real>> & _sample_points;
};
