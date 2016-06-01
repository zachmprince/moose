/****************************************************************/
/*               DO NOT MODIFY THIS HEADER                      */
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*           (c) 2010 Battelle Energy Alliance, LLC             */
/*                   ALL RIGHTS RESERVED                        */
/*                                                              */
/*          Prepared by Battelle Energy Alliance, LLC           */
/*            Under Contract No. DE-AC07-05ID14517              */
/*            With the U. S. Department of Energy               */
/*                                                              */
/*            See COPYRIGHT for full restrictions               */
/****************************************************************/

// MOOSE includes
#include "DT2mod.h"
#include "FEProblem.h"
#include "TimeIntegrator.h"
#include "NonlinearSystem.h"

template<>
InputParameters validParams<DT2mod>()
{
  InputParameters params = validParams<DT2>();

  return params;
}

DT2mod::DT2mod(const InputParameters & parameters) :
    DT2(parameters),
    _DT2_step(1),
    _at_sync_time(false)

{}

void
DT2mod::step()
{
  _error = 0.0;
  TimeStepper::step();
}

void
DT2mod::postStep()
{
  _error = _e_tol;

  if (_converged)
  {
  TransientNonlinearImplicitSystem & nl_sys = _fe_problem.getNonlinearSystem().sys();
  TransientExplicitSystem & aux_sys = _fe_problem.getAuxiliarySystem().sys();

    if (_DT2_step == 1)
    {
      _time_old_orig = _time_old;

      // save the solution (for time step with dt)
      *_u1 = *nl_sys.current_local_solution;
      _u1->close();
      *_aux1 = *aux_sys.current_local_solution;
      _aux1->close();

      _console << "  - 1. step" << std::endl;

      _e_max_orig = _e_max;
      _e_max = _e_tol;
    }
    else if (_DT2_step == 2)
    {
      _e_max += 1.0;
      _console << "  - 2. step" << std::endl;
    }
    else if (_DT2_step == 3)
    {
      *_u2 = *nl_sys.current_local_solution;
      _u2->close();

      *_u_diff = *_u2;
      *_u_diff -= *_u1;
      _u_diff->close();

      _error = (_u_diff->l2_norm() / std::max(_u1->l2_norm(), _u2->l2_norm()));

      _time_old = _time_old_orig;
      _e_max = _e_max_orig;
      _DT2_step = 0;
    }
  }
  else
  {
    if (_DT2_step > 1)
    {
      _time_old = _time_old_orig;
      _e_max = _e_max_orig;
    }

    _DT2_step = 0;
  }

  _DT2_step++;
}

Real
DT2mod::computeFailedDT()
{
  if (!_converged)
    return TimeStepper::computeFailedDT();
  else
    return computeDT();
}

Real
DT2mod::computeDT()
{
  Real new_dt;

  if (_DT2_step == 3)
    new_dt = getCurrentDT();
  else if (_DT2_step == 2)
    new_dt = getCurrentDT() / 2;
  else if (_at_sync_time && converged())
  {
    new_dt = computeInitialDT();
    _at_sync_time = false;
  }
  else
  {
      Real curr_dt = getCurrentDT() * 2.0;
      new_dt = curr_dt * std::pow(_e_tol / _error, 1.0 / (_fe_problem.getNonlinearSystem().getTimeIntegrator()->order() + 1.0));
      if (new_dt / curr_dt > _max_increase)
        new_dt = curr_dt * _max_increase;
      _at_sync_time = TimeStepper::constrainStep(new_dt);
  }

  return new_dt;
}
