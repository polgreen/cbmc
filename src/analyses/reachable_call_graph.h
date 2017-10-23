/*******************************************************************\

Module: Reachable Call Graphs

Author:

\*******************************************************************/

/// \file
/// Reachable Call Graphs

#ifndef CPROVER_ANALYSES_REACHABLE_CALL_GRAPH_H
#define CPROVER_ANALYSES_REACHABLE_CALL_GRAPH_H

#include "call_graph.h"

class reachable_call_grapht: public call_grapht
{
public:
  explicit reachable_call_grapht(const goto_modelt &);
  reachable_call_grapht() = default;

  /// \brief Generates list of functions reachable from initial state and
  /// that may reach a given destination function
  ///
  /// This is done by inverting the reachable call graph and performing bfs on
  /// the inverted call graph.
  /// \param destination function
  /// \return unorderded set of function names as irep_idts
  std::unordered_set<irep_idt, irep_id_hash>
  backward_slice(irep_idt destination_function);
  /// \brief used to initialise reachable_call_graph if we have used the
  /// empty constructor. Necessary for aggressive slicer, to avoid constructing
  /// reachable call graph unless we need it.
  void initialize(const goto_modelt &);
protected:
  void build(const goto_functionst &);
  void build(const goto_functionst &, irep_idt start_function);
};

#endif /* SRC_ANALYSES_REACHABLE_CALL_GRAPH_H_ */
