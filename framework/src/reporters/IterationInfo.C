//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "IterationInfo.h"
#include "SubProblem.h"
#include "Transient.h"

registerMooseObject("MooseApp", IterationInfo);

InputParameters
IterationInfo::validParams()
{
  InputParameters params = GeneralReporter::validParams();
  params.addClassDescription("Report iteration information for the simulation.");

  MultiMooseEnum items("num_linear_iterations num_nonlinear_iterations num_picard_iterations");
  params.addParam<MultiMooseEnum>(
      "items",
      items,
      "The iteration information to output, if nothing is provided everything will be output.");

  params.set<ExecFlagEnum>("execute_on") = {EXEC_TIMESTEP_END};
  params.suppressParameter<ExecFlagEnum>("execute_on");
  return params;
}

IterationInfo::IterationInfo(const InputParameters & parameters)
  : GeneralReporter(parameters),
    _picard_solve(_app.getExecutioner()->picardSolve()),
    _items(getParam<MultiMooseEnum>("items")),
    _num_linear(declareHelper<unsigned int>("num_linear_iterations", _dummy_unsigned_int)),
    _num_nonlinear(declareHelper<unsigned int>("num_nonlinear_iterations", _dummy_unsigned_int)),
    _num_picard(declareHelper<unsigned int>("num_picard_iterations", _dummy_unsigned_int))
{
}

void
IterationInfo::execute()
{
  _num_picard = _picard_solve.numPicardIts();
  if (_num_picard == 1)
  {
    _num_nonlinear = _subproblem.nNonlinearIterations();
    _num_linear = _subproblem.nLinearIterations();
  }
  else
  {
    _num_nonlinear += _subproblem.nNonlinearIterations();
    _num_linear += _subproblem.nLinearIterations();
  }
}
