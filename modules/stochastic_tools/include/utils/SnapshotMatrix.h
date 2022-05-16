//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include <slepcsvd.h>
#include "libmesh/parallel_object.h"
#include "libmesh/petsc_vector.h"
#include "MooseTypes.h"

namespace StochasticTools
{

class SnapshotMatrix : public libMesh::ParallelObject
{
public:
  SnapshotMatrix(const libMesh::Parallel::Communicator & comm_in);
  virtual ~SnapshotMatrix();

  void init(dof_id_type nrows_local, dof_id_type nrows_global, dof_id_type ncols);
  void addSnapshot(const std::vector<Real> & snapshot, dof_id_type row_index, bool local_index = false);
  void finalize();
  dof_id_type solveSVD(Real error = 0.0);
  Real getBasesLocal(dof_id_type index, PetscVector<Real> * u, PetscVector<Real> * v);
  Real getBasesSerial(dof_id_type index, DenseVector<Real> * u, DenseVector<Real> * v);
  void getBasesFull(DenseMatrix<Real> * sigma, DenseMatrix<Real> * u, DenseMatrix<Real> * v);

protected:
  void checkInitialize();
  void checkFinalize();
  void checkSVD(dof_id_type index = 0);

  bool _initialized;
  bool _finalized;
  bool _performed_svd;

  dof_id_type _n_local_rows;
  dof_id_type _local_row_begin;
  dof_id_type _local_row_end;
  dof_id_type _n_rows;
  dof_id_type _n_cols;

  std::vector<Real> _sigma;
  std::vector<size_t> _idx;
  dof_id_type _cutoff_ind;

private:
  PetscVector<Real> _uvec;
  PetscVector<Real> _vvec;
  SVD _svd;
  Mat _mat;
};

}
