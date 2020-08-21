//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "RandomSplitSampler.h"

registerMooseObjectAliased("StochasticToolsApp", RandomSplitSampler, "RandomSplit");

InputParameters
RandomSplitSampler::validParams()
{
  InputParameters params = SplitSampler::validParams();
  params.addClassDescription("Extracts a random portion of the provided sampler.");
  params.addRequiredRangeCheckedParam<dof_id_type>(
      "num_rows", "num_rows > 0", "Number of rows to extract.");
  params.addParam<bool>(
      "allow_repeated_rows", false, "Whether or not the a row can be used more than once.");
  return params;
}

RandomSplitSampler::RandomSplitSampler(const InputParameters & parameters)
  : SplitSampler(parameters),
    _nrows(getParam<dof_id_type>("num_rows")),
    _allow_repeat(getParam<bool>("allow_repeated_rows"))
{
  if (!_allow_repeat && _nrows > _sampler.getNumberOfRows())
    paramError("num_rows",
               "Number of rows requested is greater than the number of rows in sampler.");
  setNumberOfRows(_nrows);
}

void
RandomSplitSampler::recomputeSplitIndices()
{
  _split_indices.resize(_nrows);
  if (_allow_repeat)
    for (unsigned int i = 0; i < _nrows; ++i)
      _split_indices[i] = getRandl(0, 0, _sampler.getNumberOfRows());
  else
  {
    std::vector<dof_id_type> all_ind(_sampler.getNumberOfRows());
    std::iota(all_ind.begin(), all_ind.end(), 0);
    for (dof_id_type i = 0; i < _nrows; ++i)
    {
      auto ind = all_ind.begin() + getRandl(0, 0, all_ind.size());
      _split_indices[i] = *ind;
      all_ind.erase(ind);
    }
  }
}
