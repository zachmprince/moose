//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "SnapshotMatrix.h"

namspace StochasticTools
{

SnapshotMatrix::SnapshotMatrix(const libMesh::Parallel::Communicator & comm_in)
    : libMesh::Parallel(comm_in), _initialized(false), _finalized(false)
{
}

SnapshotMatrix::intialize(dof_id_type nrows_local, dof_id_type nrows_global, dof_id_type ncols)
{
  if (_initialized)
  {
    MatDestroy(&mat);
  }

  MatCreateDense(_communicator.get(), nrows_local, ncols, nrows_global, ncols, NULL, &_mat);

  _col_indices.resize(ncols);
  std::iota(_col_indices.begin(), _col_indices.end(), 0);
}

SnapshotMatrix::addSnapshot(dof_id_type row_index, const std::vector<Real> & snapshot)
{
  checkInitialize();
  _finalized = false;

  MatSetValues(_mat, 1, &row_index, _col_indices.size(), _col_indices.data(), snapshot.data(), INSERT_VALUES);
}
