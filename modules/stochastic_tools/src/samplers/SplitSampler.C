//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "SplitSampler.h"

InputParameters
SplitSampler::validParams()
{
  InputParameters params = Sampler::validParams();
  params.addRequiredParam<SamplerName>("sampler", "Sampler to split.");
  return params;
}

SplitSampler::SplitSampler(const InputParameters & parameters)
  : Sampler(parameters),
    _sampler(getSampler("sampler")),
    _matrix(0, 0),
    _split_indices(0),
    _perf_sample_setup(registerTimedSection("sampleSetup", 3)),
    _perf_compute_sample(registerTimedSection("computeSample", 4))
{
  setNumberOfCols(_sampler.getNumberOfCols());
}

void
SplitSampler::sampleSetUp()
{
  TIME_SECTION(_perf_sample_setup);

  recomputeSplitIndices();

  if (getNumberOfRows() != _split_indices.size())
    mooseError("Inconsitent number of rows and split indices.");
  if (*std::max_element(_split_indices.begin(), _split_indices.end()) >= _sampler.getNumberOfRows())
    mooseError("Split indicies contain an index that is greater than the sampler number of rows.");

  _matrix.resize(getNumberOfLocalRows(), getNumberOfCols());

  // Must gather full matrix because the partitioning between the supplied object
  // and this object differ
  DenseMatrix<Real> glob_matrix(getNumberOfRows(), getNumberOfCols());
  for (dof_id_type p = _sampler.getLocalRowBegin(); p < _sampler.getLocalRowEnd(); ++p)
  {
    std::vector<Real> data = _sampler.getNextLocalRow();
    auto it = _split_indices.begin();
    // Need this loop in case there are repeat entries
    while ((it = std::find(it, _split_indices.end(), p)) != _split_indices.end())
    {
      auto ind = std::distance(_split_indices.begin(), it++);
      for (unsigned int n = 0; n < data.size(); ++n)
        glob_matrix(ind, n) = data[n];
    }
  }
  _communicator.sum(glob_matrix.get_values());

  // Save the local part of the matrix
  for (dof_id_type p = getLocalRowBegin(); p < getLocalRowEnd(); ++p)
    for (unsigned int n = 0; n < _matrix.n(); ++n)
      _matrix(p - getLocalRowBegin(), n) = glob_matrix(p, n);
}

Real
SplitSampler::computeSample(dof_id_type row_index, dof_id_type col_index)
{
  TIME_SECTION(_perf_compute_sample);

  return _matrix(row_index - getLocalRowBegin(), col_index);
}
