//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "UniformSplitSampler.h"

registerMooseObjectAliased("StochasticToolsApp", UniformSplitSampler, "UniformSplit");

InputParameters
UniformSplitSampler::validParams()
{
  InputParameters params = SplitSampler::validParams();
  params.addClassDescription(
      "Extracts portion from provided sampler, given an intial row and number of samples.");
  params.addRequiredRangeCheckedParam<std::vector<dof_id_type>>(
      "sampler_row", "sampler_row > 0", "First row of sampler to extract (indexing starts at 1)");
  params.addRequiredRangeCheckedParam<std::vector<dof_id_type>>(
      "num_rows", "num_rows > 0", "Number of rows to extract.");
  return params;
}

UniformSplitSampler::UniformSplitSampler(const InputParameters & parameters)
  : SplitSampler(parameters)
{
  const auto first_vec = getParam<std::vector<dof_id_type>>("sampler_row");
  const auto nrow_vec = getParam<std::vector<dof_id_type>>("num_rows");
  if (first_vec.size() != nrow_vec.size())
    paramError("num_rows", "Number of entries in sampler_row and num_rows must match.");

  const dof_id_type nrows = std::accumulate(nrow_vec.begin(), nrow_vec.end(), 0);

  _split_indices.reserve(nrows);
  for (unsigned int n = 0; n < first_vec.size(); ++n)
  {
    if ((first_vec[n] + nrow_vec[n] - 1) > _sampler.getNumberOfRows())
      paramError("num_rows",
                 "sampler_row + num_rows (",
                 first_vec[n] + nrow_vec[n] - 1,
                 ") is greater than the number of rows in sampler (",
                 _sampler.getNumberOfRows(),
                 ").");
    for (dof_id_type i = first_vec[n]; i < nrow_vec[n] + first_vec[n]; ++i)
      _split_indices.push_back(i - 1);
  }

  setNumberOfRows(nrows);
}
