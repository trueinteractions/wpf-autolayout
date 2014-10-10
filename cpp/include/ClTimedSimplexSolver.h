// $Id: ClTimedSimplexSolver.h,v 1.2 2005/10/20 18:20:53 gjb Exp $
//
// Cassowary Incremental Constraint Solver
// Original Smalltalk Implementation by Alan Borning
// This C++ Implementation by Greg J. Badros, <gjb@cs.washington.edu>
// http://www.cs.washington.edu/homes/gjb
// (C) 1998, 1999, 2000 Greg J. Badros and Alan Borning
// See ../LICENSE for legal details regarding this software
//
// ClTimedSimplexSolver.h

#ifndef ClTimedSimplexSolver_H
#define ClTimedSimplexSolver_H

#if defined(HAVE_CONFIG_H) && !defined(CONFIG_H_INCLUDED) && !defined(CONFIG_INLINE_H_INCLUDED)
#include "config-inline.h"
#define CONFIG_INLINE_H_INCLUDED
#endif

#include "ClSimplexSolver.h"
#include "timer.h"
#include <iostream>

class ClTimedSimplexSolver : 
  public ClSimplexSolver  {
 protected: typedef ClSimplexSolver super;
  
 public:

  // Constructor
  ClTimedSimplexSolver() :
      ClSimplexSolver(),
      _cAdd(0),
      _cRemove(0),
      _cBeginEdit(0),
      _cEndEdit(0),
      _cResolve(0)
  { }

  virtual ~ClTimedSimplexSolver()
  { }
  
  // Add the constraint cn to the tableau
  ClSimplexSolver &AddConstraint(ClConstraint *const pcn)
    throw(ExCLTooDifficultSpecial,
          ExCLStrictInequalityNotAllowed,
          ExCLReadOnlyNotAllowed,
          ExCLEditMisuse,
          ExCLRequiredFailure,
          ExCLRequiredFailureWithExplanation,
          ExCLInternalError)
  { 
    _addTimer.Start();
    ++_cAdd;
    super::AddConstraint(pcn);
    _addTimer.Stop();
    return *this;
  }

  ClSimplexSolver &AddConstraint(ClConstraint &cn)
    throw(ExCLTooDifficultSpecial,
          ExCLStrictInequalityNotAllowed,
          ExCLReadOnlyNotAllowed,
          ExCLEditMisuse,
          ExCLRequiredFailure,
          ExCLRequiredFailureWithExplanation,
          ExCLInternalError)
  { return AddConstraint(&cn); }
  

  // BeginEdit() should be called before sending
  // Resolve() messages, after adding the appropriate edit variables
  ClSimplexSolver &BeginEdit()
  {
    _beginEditTimer.Start();
    ++_cBeginEdit;
    super::BeginEdit();
    _beginEditTimer.Stop();
    return *this;
  }

  // EndEdit should be called after editing has finished
  // for now, it just removes edit variables added from before the last BeginEdit
  ClSimplexSolver &EndEdit()
  {
    _endEditTimer.Start();
    ++_cEndEdit;
    super::EndEdit();
    _endEditTimer.Stop();
    return *this;
  }
  
  // Remove the constraint cn from the tableau
  // Also remove any error variable associated with cn
  ClSimplexSolver &RemoveConstraint(ClConstraint *const pcn)
      throw (ExCLConstraintNotFound)
  { 
    _removeTimer.Start();
    ++_cRemove;
    super::RemoveConstraint(pcn);
    _removeTimer.Stop();
    return *this;
  }
  
  void Resolve()
  { 
    _resolveTimer.Start();
    ++_cResolve;
    super::Resolve();
    _resolveTimer.Stop();
  }
  
  Timer &GetAddTimer()
  { return _addTimer; }

  Timer &GetRemoveTimer()
  { return _removeTimer; }

  Timer &GetBeginEditTimer()
  { return _beginEditTimer; }

  Timer &GetEndEditTimer()
  { return _endEditTimer; }

  Timer &GetResolveTimer()
  { return _resolveTimer; }

public:
  unsigned long _cAdd;
  unsigned long _cRemove;
  unsigned long _cBeginEdit;
  unsigned long _cEndEdit;
  unsigned long _cResolve;

  private:
  
  Timer _addTimer;
  Timer _removeTimer;
  Timer _beginEditTimer;
  Timer _endEditTimer;
  Timer _resolveTimer;

  
};
#endif
