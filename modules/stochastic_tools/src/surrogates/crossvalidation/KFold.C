//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "KFold.h"

#include "AddSamplerAction.h"
#include "ActionWarehouse.h"
#include "MooseRandom.h"

registerCrossValidationAction("StochasticToolsApp", KFold);

InputParameters
KFold::validParams()
{
  InputParameters params = AddCrossValidationAction::validParams();
  params.addClassDescription("Action for adding appropiate objects for k-Fold cross validation.");
  params.addRequiredRangeCheckedParam<unsigned int>(
      "num_folds", "num_folds > 1", "Number of folds (k), i.e. number of train-test sets.");
  params.addParam<bool>("shuffle",
                        "True to randomly shuffle samples before folding. If not provided, "
                        "shuffling will be inferred from training sampler type.");
  params.addParam<unsigned int>("seed", 0, "Seed for random shuffling.");
  return params;
}

KFold::KFold(const InputParameters & params)
  : AddCrossValidationAction(params),
    _num_folds(getParam<unsigned int>("num_folds")),
    _shuffle(isParamValid("shuffle")
                 ? getParam<bool>("shuffle")
                 : isSamplerSorted(
                       _awh.getAction<AddSamplerAction>(_trainer_params.get<SamplerName>("sampler"))
                           .getMooseObjectType()))
{
  if (_num_folds > _num_rows)
    paramError("num_folds",
               "Number of folds (",
               _num_folds,
               ") cannot be greater than number of rows (",
               _num_rows,
               ")");

  _test_nrow.resize(_num_folds, _num_rows / _num_folds);
  for (unsigned int i = _num_folds - (_num_rows % _num_folds); i < _num_folds; ++i)
    _test_nrow[i]++;
  _test_index.resize(_num_folds, 0);
  for (unsigned int i = 1; i < _num_folds; ++i)
    _test_index[i] = _test_index[i - 1] + _test_nrow[i - 1];

  if (_shuffle)
  {
    _shuffled_indices.resize(_num_rows);
    std::iota(_shuffled_indices.begin(), _shuffled_indices.end(), 1);
    MooseRandom seed_generator;
    seed_generator.seed(0, getParam<unsigned int>("seed"));
    for (dof_id_type i = 0; i < _num_rows - 1; ++i)
    {
      unsigned int j = seed_generator.randl(0, i, _num_rows - 1);
      std::swap(_shuffled_indices[i], _shuffled_indices[j]);
    }
  }
}

bool
KFold::isSamplerSorted(const std::string & type)
{
  if (type == "CartesianProduct" || type == "CartesianProductSampler")
    return true;
  else if (type == "Quadrature" || type == "QuadratureProductSampler")
    return true;

  return false;
}

dof_id_type
KFold::numSets() const
{
  return _num_folds;
}

std::vector<dof_id_type>
KFold::testIndices(dof_id_type i) const
{
  if (_shuffle)
    return std::vector<dof_id_type>(_shuffled_indices.begin() + _test_index[i],
                                    _shuffled_indices.begin() + _test_index[i] + _test_nrow[i]);
  else
    return {_test_index[i] + 1};
}

std::vector<dof_id_type>
KFold::testNumRow(dof_id_type i) const
{
  if (_shuffle)
    return std::vector<dof_id_type>(_test_nrow[i], 1);
  else
    return {_test_nrow[i]};
}
