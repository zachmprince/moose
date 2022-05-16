//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "SnapshotMatrix.h"

namespace StochasticTools
{

SnapshotMatrix::SnapshotMatrix(const libMesh::Parallel::Communicator & comm_in)
  : libMesh::ParallelObject(comm_in),
    _initialized(false),
    _finalized(false),
    _performed_svd(false),
    _n_local_rows(0),
    _local_row_begin(0),
    _local_row_end(0),
    _n_rows(0),
    _n_cols(0),
    _sigma(0),
    _idx(0),
    _cutoff_ind(0),
    _uvec(_communicator),
    _vvec(_communicator)
{
  SVDCreate(_communicator.get(), &_svd);
  SVDSetType(_svd, SVDSCALAPACK);
  SVDSetFromOptions(_svd);
}

SnapshotMatrix::~SnapshotMatrix()
{
  if (_initialized)
    MatDestroy(&_mat);
  if (_performed_svd)
    SVDDestroy(&_svd);
}

void
SnapshotMatrix::init(dof_id_type num_snapshots_local, dof_id_type num_snapshots, dof_id_type snapshot_size)
{
  _finalized = false;
  _performed_svd = false;

  if (_initialized)
  {
    if (_n_local_rows == num_snapshots_local && _n_rows == num_snapshots && _n_cols == snapshot_size)
    {
      MatZeroEntries(_mat);
      _uvec.zero();
      _vvec.zero();
      return;
    }
    MatDestroy(&_mat);
  }

  MatCreateDense(_communicator.get(), num_snapshots_local, snapshot_size, num_snapshots, snapshot_size, NULL, &_mat);
  _uvec.init(snapshot_size, snapshot_size);
  _vvec.init(num_snapshots, num_snapshots_local);

  _n_local_rows = num_snapshots_local;
  _n_rows = num_snapshots;
  _n_cols = snapshot_size;
  MatGetOwnershipRange(_mat, numeric_petsc_cast(&_local_row_begin), numeric_petsc_cast(&_local_row_end));

  _initialized = true;
}

void
SnapshotMatrix::addSnapshot(const std::vector<Real> & snapshot, dof_id_type snapshot_index, bool local_index)
{
  checkInitialize();
  if (snapshot.size() != _n_cols)
    ::mooseError("Inputted snapshot size (", snapshot.size(), ") does not match size specified at initialization (", _n_cols, ").");
  const dof_id_type row = local_index ? snapshot_index + _local_row_begin : snapshot_index;
  if (row >= _local_row_end || row < _local_row_begin)
    ::mooseError("Inputted row index for snapshot is not contained in local range of rows specified in initialization.");

  for (const auto & col : make_range(_n_cols))
    MatSetValue(_mat, row, col, snapshot[col], INSERT_VALUES);

  _finalized = false;
  _performed_svd = false;
}

void
SnapshotMatrix::finalize()
{
  checkInitialize();
  MatAssemblyBegin(_mat, MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(_mat, MAT_FINAL_ASSEMBLY);
  _finalized = true;
}

dof_id_type
SnapshotMatrix::solveSVD(Real error)
{
  checkFinalize();
  if (!_performed_svd)
  {
    SVDSetOperators(_svd, _mat, NULL);
    SVDSolve(_svd);
    PetscInt nconv;
    SVDGetConverged(_svd, &nconv);

    PetscReal sig;
    _sigma.resize(nconv);
    for (unsigned int i = 0; i < nconv; ++i)
    {
      SVDGetSingularTriplet(_svd, i, &sig, NULL, NULL);
      _sigma[i] = sig * sig;
    }
    Moose::indirectSort(_sigma.begin(), _sigma.end(), _idx, std::greater<Real>());
    Moose::applyIndices(_sigma, _idx);
  }

  _cutoff_ind = 0;
  const Real sum = std::accumulate(_sigma.begin(), _sigma.end(), 0.0);
  Real partial_sum = 0.0;
  for (const auto & sig2 : _sigma)
  {
    ++_cutoff_ind;
    partial_sum += sig2;
    if (partial_sum / sum > 1 - error)
      break;
  }

  _performed_svd = true;
  return _cutoff_ind;
}

Real
SnapshotMatrix::getBasesLocal(dof_id_type index, PetscVector<Real> * u, PetscVector<Real> * v)
{
  checkSVD(index);
  PetscReal sigma;
  if (u && v)
    SVDGetSingularTriplet(_svd, _idx[index], &sigma, v->vec(), u->vec());
  else if (u)
    SVDGetSingularTriplet(_svd, _idx[index], &sigma, NULL, u->vec());
  else if (v)
    SVDGetSingularTriplet(_svd, _idx[index], &sigma, v->vec(), NULL);
  else
    sigma = _sigma[index];

  return sigma;
}

Real
SnapshotMatrix::getBasesSerial(dof_id_type index, DenseVector<Real> * u, DenseVector<Real> * v)
{
  Real sigma;
  if (u && v)
    sigma = getBasesLocal(index, &_uvec, &_vvec);
  else if (u)
    sigma = getBasesLocal(index, &_uvec, nullptr);
  else if (v)
    sigma = getBasesLocal(index, nullptr, &_vvec);
  else
    sigma = _sigma[index];

  if (u)
  {
    u->resize(_uvec.size());
    _uvec.localize(u->get_values());
  }
  if (v)
  {
    v->resize(_vvec.size());
    _vvec.localize(v->get_values());
  }

  return sigma;
}

void
SnapshotMatrix::getBasesFull(DenseMatrix<Real> * sigma, DenseMatrix<Real> * u, DenseMatrix<Real> * v)
{
  if (sigma)
    sigma->resize(_cutoff_ind, _cutoff_ind);
  if (u)
    u->resize(_n_cols, _cutoff_ind);
  if (v)
    v->resize(_n_rows, _cutoff_ind);

  for (dof_id_type i = 0; i < _cutoff_ind; ++i)
  {
    if (u && v)
      getBasesLocal(i, &_uvec, &_vvec);
    else if (u)
      getBasesLocal(i, &_uvec, nullptr);
    else if (v)
      getBasesLocal(i, nullptr, &_vvec);

    if (u)
    {
      auto up = _uvec.get_array_read();
      for (const auto & d : make_range(_n_cols))
        (*u)(d, i) = up[d];
      _uvec.restore_array();
    }
    if (v)
    {
      _vvec.print();
      auto vp = _vvec.get_array_read();
      for (const auto & s : make_range(_n_local_rows))
        (*v)(s + _local_row_begin, i) = vp[s];
      _vvec.restore_array();
    }
    if (sigma)
      (*sigma)(i, i) = _sigma[i];
  }

  if (v)
    _communicator.sum(v->get_values());
}

void
SnapshotMatrix::checkInitialize()
{
  if (!_initialized)
    ::mooseError("This function call requires that the object be initialized.");
}

void
SnapshotMatrix::checkFinalize()
{
  if (!_finalized)
    ::mooseError("This function call requires that the object be finalized.");
}

void
SnapshotMatrix::checkSVD(dof_id_type index)
{
  if (!_performed_svd)
    ::mooseError("SVD has not been performed yet, call solveSVD() first.");
  else if (index >= _cutoff_ind)
    ::mooseError("Requested index (", index, ") is not contained in decomposition with ", _cutoff_ind, " singular values.");
}

}
