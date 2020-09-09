//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "AddCrossValidationAction.h"

#include "AddSurrogateAction.h"
#include "AddSamplerAction.h"
#include "Sampler.h"
#include "SurrogateModel.h"

InputParameters
AddCrossValidationAction::validParams()
{
  InputParameters params = Action::validParams();
  params.addRequiredParam<UserObjectName>("trainer", "Trainer to perform cross validation on.");
  params.addRequiredParam<UserObjectName>(
      "surrogate", "Surrogate object to copy for cross validating evaluation.");
  params.addParam<PostprocessorName>(
      "postprocessor_name",
      "Desired name of postprocessor that contains the cross validation value.");
  params.addRequiredParam<dof_id_type>("num_rows", "Number of rows in sampler.");
  return params;
}

AddCrossValidationAction::AddCrossValidationAction(const InputParameters & params)
  : Action(params),
    _trainer_name(getParam<UserObjectName>("trainer")),
    _trainer_type(_awh.getAction<AddSurrogateAction>(_trainer_name).getMooseObjectType()),
    _trainer_params(_awh.getAction<AddSurrogateAction>(_trainer_name).getObjectParams()),
    _surrogate_name(getParam<UserObjectName>("surrogate")),
    _surrogate_type(_awh.getAction<AddSurrogateAction>(_surrogate_name).getMooseObjectType()),
    _surrogate_params(_awh.getAction<AddSurrogateAction>(_surrogate_name).getObjectParams()),
    _pp_name(isParamValid("postprocessor_name")
                 ? getParam<PostprocessorName>("postprocessor_name")
                 : static_cast<PostprocessorName>(name() + "_value")),
    _num_rows(getParam<dof_id_type>("num_rows"))
{
  checkTrainerIntegrity();
}

void
AddCrossValidationAction::act()
{
  if (_current_task == "add_trainer")
  {
    for (dof_id_type i = 0; i < numSets(); ++i)
      addCVTrainer(trainerName(i), testIndices(i), testNumRow(i));
  }
  else if (_current_task == "add_surrogate")
  {
    for (dof_id_type i = 0; i < numSets(); ++i)
      addCVSurrogate(surrogateName(i), trainerName(i));
  }
  else if (_current_task == "add_postprocessor")
  {
    std::vector<UserObjectName> surrogate_names(numSets());
    std::vector<std::vector<dof_id_type>> test_indices(numSets());
    std::vector<std::vector<dof_id_type>> test_nrows(numSets());
    for (dof_id_type i = 0; i < numSets(); ++i)
    {
      surrogate_names[i] = surrogateName(i);
      test_indices[i] = testIndices(i);
      test_nrows[i] = testNumRow(i);
    }

    addCVPostprocessor(valueName(), surrogate_names, test_indices, test_nrows);
  }
  else if (_current_task == "check_integrity")
  {
    checkCVIntegrity();
  }
}

void
AddCrossValidationAction::checkTrainerIntegrity()
{
  if (!_trainer_params.isParamValid("skip_index") || !_trainer_params.isParamValid("num_skip"))
    paramError("trainer",
               "Trainer ",
               _trainer_name,
               " (",
               _trainer_type,
               ") cannot be used for cross validation.");
  else if (_trainer_params.isParamSetByUser("skip_index") ||
           _trainer_params.isParamSetByUser("num_skip"))
    paramError(
        "trainer",
        "Trainer ",
        _trainer_name,
        " has skip_index/num_skip set, which is not allowed when performing cross validation.");
}

void
AddCrossValidationAction::checkCVIntegrity()
{
  const SamplerName sampler_name = _trainer_params.get<SamplerName>("sampler");
  const Sampler & sampler = _problem->getSampler(sampler_name);
  if (_num_rows != sampler.getNumberOfRows())
    paramError("num_rows",
               "Number of rows specified (",
               _num_rows,
               ") does not match number of rows in ",
               sampler_name,
               " (",
               sampler.getNumberOfRows(),
               ").");
}

UserObjectName
AddCrossValidationAction::trainerName(dof_id_type i) const
{
  return name() + "_trainer" + std::to_string(i);
}

UserObjectName
AddCrossValidationAction::surrogateName(dof_id_type i) const
{
  return name() + "_surrogate" + std::to_string(i);
}

void
AddCrossValidationAction::addCVTrainer(const UserObjectName & name,
                                       const std::vector<dof_id_type> & index,
                                       const std::vector<dof_id_type> & nrow)
{
  InputParameters params = _factory.getValidParams(_trainer_type);
  params.applyParameters(_trainer_params);
  params.set<std::vector<dof_id_type>>("skip_index") = index;
  params.set<std::vector<dof_id_type>>("num_skip") = nrow;
  _problem->addUserObject(_trainer_type, name, params);
}

void
AddCrossValidationAction::addCVSurrogate(const UserObjectName & name,
                                         const UserObjectName & trainer_name)
{
  InputParameters params = _factory.getValidParams(_surrogate_type);
  params.applyParameters(_surrogate_params, {"trainer", "filename"});
  params.set<UserObjectName>("trainer") = trainer_name;
  std::shared_ptr<SurrogateModel> model =
      _factory.create<SurrogateModel>(_surrogate_type, name, params);
  _problem->theWarehouse().add(model);
}

void
AddCrossValidationAction::addCVPostprocessor(const PostprocessorName & name,
                                             const std::vector<UserObjectName> & surrogate_names,
                                             const std::vector<std::vector<dof_id_type>> & index,
                                             const std::vector<std::vector<dof_id_type>> & nrow)
{
  InputParameters params = _factory.getValidParams("CrossValidatedRSME");
  params.set<VectorPostprocessorName>("data_vpp") =
      _trainer_params.get<VectorPostprocessorName>("results_vpp");
  params.set<std::string>("data_vector") = _trainer_params.get<std::string>("results_vector");
  params.set<SamplerName>("sampler") = _trainer_params.get<SamplerName>("sampler");
  params.set<std::vector<UserObjectName>>("models") = surrogate_names;
  params.set<std::vector<std::vector<dof_id_type>>>("test_index") = index;
  params.set<std::vector<std::vector<dof_id_type>>>("num_points") = nrow;
  _problem->addPostprocessor("CrossValidatedRSME", name, params);
}
