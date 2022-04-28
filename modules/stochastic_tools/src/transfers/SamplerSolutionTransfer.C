//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

// StochasticTools includes
#include "SamplerSolutionTransfer.h"
#include "NonlinearSystemBase.h"
#include "Sampler.h"

registerMooseObject("StochasticToolsApp", SamplerSolutionTransfer);

InputParameters
SamplerSolutionTransfer::validParams()
{
  InputParameters params = StochasticToolsTransfer::validParams();
  params.addClassDescription("Transfers solution vectors from the sub-applications.");
  params.addRequiredParam<std::vector<VariableName>>("variables", "Variables to gather from sub-applications.");
  return params;
}

SamplerSolutionTransfer::SamplerSolutionTransfer(const InputParameters & parameters)
  : StochasticToolsTransfer(parameters),
    _var_names(getParam<std::vector<VariableName>>("variables"))
{
}

void
SamplerSolutionTransfer::initialSetup()
{
  const auto multi_app = hasFromMultiApp() ? getFromMultiApp() : getToMultiApp();

  // Checking if the subapplication has the requested variables
  const dof_id_type n = multi_app->numGlobalApps();
  for (MooseIndex(n) i = 0; i < n; i++)
  {
    if (multi_app->hasLocalApp(i))
      for (auto var_name : _var_names)
        if (!multi_app->appProblemBase(i).hasVariable(var_name))
          mooseError("Variable '" + var_name + "' not found on sub-application ", i, "!");
  }
}

void
SamplerSolutionTransfer::execute()
{
}

void
SamplerSolutionTransfer::initializeFromMultiapp()
{
}

void
SamplerSolutionTransfer::executeFromMultiapp()
{
}

void
SamplerSolutionTransfer::finalizeFromMultiapp()
{
}

void
SamplerSolutionTransfer::initializeToMultiapp()
{
}

void
SamplerSolutionTransfer::executeToMultiapp()
{
}

void
SamplerSolutionTransfer::finalizeToMultiapp()
{
}
