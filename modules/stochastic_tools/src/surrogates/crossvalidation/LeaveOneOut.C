//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "LeaveOneOut.h"

#include "Factory.h"
#include "FEProblemBase.h"

registerCrossValidationAction("StochasticToolsApp", LeaveOneOut);

InputParameters
LeaveOneOut::validParams()
{
  InputParameters params = AddCrossValidationAction::validParams();
  params.addClassDescription(
      "Action for adding appropiate objects for leave-one-out cross validation.");
  params.addParam<bool>(
      "force_retrain",
      false,
      "True to force retraining. Otherwise, linear models will use LinearLOOCrossValidation.");
  return params;
}

LeaveOneOut::LeaveOneOut(const InputParameters & params)
  : AddCrossValidationAction(params),
    _retrain(getParam<bool>("force_retrain") || needRetrain(_surrogate_type))
{
}

bool
LeaveOneOut::needRetrain(const std::string & type)
{
  if (type == "PolynomialChaos")
    return false;
  else if (type == "PolynomialRegressionSurrogate")
    return false;

  return true;
}

void
LeaveOneOut::addCVPostprocessor(const PostprocessorName & name,
                                const std::vector<UserObjectName> & surrogate_names,
                                const std::vector<std::vector<dof_id_type>> & index,
                                const std::vector<std::vector<dof_id_type>> & nrow)
{
  if (_retrain)
    AddCrossValidationAction::addCVPostprocessor(name, surrogate_names, index, nrow);
  else
  {
    InputParameters params = _factory.getValidParams("LinearLOOCrossValidation");
    params.set<VectorPostprocessorName>("data_vpp") =
        _trainer_params.get<VectorPostprocessorName>("results_vpp");
    params.set<std::string>("data_vector") = _trainer_params.get<std::string>("results_vector");
    params.set<SamplerName>("sampler") = _trainer_params.get<SamplerName>("sampler");
    params.set<UserObjectName>("model") = _surrogate_name;
    _problem->addPostprocessor("LinearLOOCrossValidation", name, params);
  }
}

dof_id_type
LeaveOneOut::numSets() const
{
  return (_retrain ? _num_rows : 0);
}

std::vector<dof_id_type>
LeaveOneOut::testIndices(dof_id_type i) const
{
  if (_retrain)
    return {i + 1};
  else
    return {};
}

std::vector<dof_id_type>
LeaveOneOut::testNumRow(dof_id_type /*i*/) const
{
  if (_retrain)
    return {1};
  else
    return {};
}
