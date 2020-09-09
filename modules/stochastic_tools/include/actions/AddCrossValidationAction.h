//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "Action.h"
#include "MooseEnum.h"

#define registerCrossValidationAction(app, classname)                                              \
  registerMooseActionCombined(app, classname, "add_trainer", 0);                                   \
  registerMooseActionCombined(app, classname, "add_surrogate", 1);                                 \
  registerMooseActionCombined(app, classname, "add_postprocessor", 2);                             \
  registerMooseActionCombined(app, classname, "check_integrity", 3);

class AddCrossValidationAction : public Action
{
public:
  static InputParameters validParams();

  AddCrossValidationAction(const InputParameters & params);

  virtual void act() override final;

protected:
  /// Check if the inputted trainer is appropriate for cross validation
  virtual void checkTrainerIntegrity();
  /// Check integrity of built objects
  virtual void checkCVIntegrity();
  /// Trainer name given a model index
  virtual UserObjectName trainerName(dof_id_type i) const;
  /// Surrogate name given a model index
  virtual UserObjectName surrogateName(dof_id_type i) const;
  /// Name of postprocessor with cross validation value
  virtual PostprocessorName valueName() const { return _pp_name; }
  /// Adding trainer given train set indices
  virtual void addCVTrainer(const UserObjectName & name,
                            const std::vector<dof_id_type> & index,
                            const std::vector<dof_id_type> & nrow);
  /// Adding surrogate given a trainer name
  virtual void addCVSurrogate(const UserObjectName & name, const UserObjectName & trainer_name);
  /// Adding postprocessor to compute validation value
  virtual void addCVPostprocessor(const PostprocessorName & name,
                                  const std::vector<UserObjectName> & surrogate_names,
                                  const std::vector<std::vector<dof_id_type>> & index,
                                  const std::vector<std::vector<dof_id_type>> & nrow);

  /// Number of train-test splits
  virtual dof_id_type numSets() const = 0;
  /// Starting indices for each testing set
  virtual std::vector<dof_id_type> testIndices(dof_id_type i) const = 0;
  /// Number of rows for each testing set
  virtual std::vector<dof_id_type> testNumRow(dof_id_type i) const = 0;

  /// Trainer name
  const UserObjectName _trainer_name;
  /// Trainer type
  const std::string _trainer_type;
  /// Trainer input parameters
  const InputParameters & _trainer_params;
  /// Trainer name
  const UserObjectName _surrogate_name;
  /// Trainer type
  const std::string _surrogate_type;
  /// Trainer input parameters
  const InputParameters & _surrogate_params;
  /// Name of postprocessor with cross validation value
  const PostprocessorName _pp_name;
  /// Total number of training points
  const dof_id_type _num_rows;
};
