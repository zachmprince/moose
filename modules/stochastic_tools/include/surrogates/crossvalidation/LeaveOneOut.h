//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "AddCrossValidationAction.h"

class LeaveOneOut : public AddCrossValidationAction
{
public:
  static InputParameters validParams();

  LeaveOneOut(const InputParameters & params);

protected:
  /// See if surrogate type can use LinearLOOCrossValidation
  static bool needRetrain(const std::string & type);
  /// Overwritting in case LinearLOOCrossValidation can be used
  virtual void addCVPostprocessor(const PostprocessorName & name,
                                  const std::vector<UserObjectName> & surrogate_names,
                                  const std::vector<std::vector<dof_id_type>> & index,
                                  const std::vector<std::vector<dof_id_type>> & nrow) override;

  /// Number of train-test splits
  virtual dof_id_type numSets() const override;
  /// Starting indices for each testing set
  virtual std::vector<dof_id_type> testIndices(dof_id_type i) const override;
  /// Number of rows for each testing set
  virtual std::vector<dof_id_type> testNumRow(dof_id_type i) const override;

private:
  /// Whether or not separate trainers and surrogates need to be contructed
  const bool _retrain;
};
