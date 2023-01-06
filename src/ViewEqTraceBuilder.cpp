#include "ViewEqTraceBuilder.h"

typedef int IPid;

ViewEqTraceBuilder::ViewEqTraceBuilder(const Configuration &conf) : TSOPSOTraceBuilder(conf) {
  threads.push_back(Thread(CPid(), -1));
  current_thread = -1;
  current_state = -1;
  replay_point = -1;
  prefix_idx = 0;
  execution_sequence.set_container_reference(this);
  empty_sequence.set_container_reference(this);
  empty_sequence.clear();
  check_optimality = conf.check_optimality;
}

ViewEqTraceBuilder::~ViewEqTraceBuilder() {}

bool ViewEqTraceBuilder::schedule(int *proc, int *aux, int *alt, bool *DryRun) {
  // snj: For compatibility with existing design
  *aux = -1; *alt = 0; *DryRun = false;

  assert(execution_sequence.size() == prefix_state.size());
  if (is_replaying()) {
    // out << "REPLAYING: ";
    replay_schedule(proc);
    return true;
  }

  if (at_replay_point()) {
    // out << "AT REPLAY PT: \n";
    
    current_state = prefix_state[prefix_idx];
    assert(states[current_state].has_unexplored_leads());

    execution_sequence.pop_back();
    prefix_state.pop_back();

    states[current_state].lead_head_execution_prefix = prefix_idx;
    alpha_head = prefix_idx;
    
    execute_next_lead();
    *proc = current_thread;
    return true;
  }

  // peak next read/write event / execute next non-read/write event
  if (exists_non_memory_access(proc)) {
    return true;
  }

  if (Enabled.empty()) {
    assert(forbidden.evaluate != RESULT::TRUE);
    return false; // maximal trace explored
  }

  // Explore Algo function
  // out << "next RW\n";
  make_new_state(); 
  // out << "made new state\n";
  compute_new_leads(); 
  // out << "made new leads\n";
  
  if (states[current_state].has_unexplored_leads()) { // TODO should be assert not check
    execute_next_lead();
    // out << "executed lead\n";
    // execute current event from Interpreter::run() in Execution.cpp
    *proc = current_thread;
    return true;
  }
  
  assert(false); //must not reach here
  return false;
}

void ViewEqTraceBuilder::replay_non_memory_access(int next_replay_thread, IID<IPid> next_replay_event) {
  assert(!threads[next_replay_thread].awaiting_load_store);
  assert(next_replay_event.get_index() == threads[next_replay_thread].size());
  threads[next_replay_thread].push_back(next_replay_event);
  current_thread = next_replay_thread;
}

void ViewEqTraceBuilder::replay_memory_access(int next_replay_thread, IID<IPid> next_replay_event) {
  assert(prefix_state[prefix_idx] >= 0);
  assert(next_replay_event.get_index() == (threads[next_replay_thread].size()-1));

  auto it = std::find(Enabled.begin(), Enabled.end(), next_replay_event);
  Enabled.erase(it);

  current_state = prefix_state[prefix_idx];
  current_thread = next_replay_thread;
  current_event = get_event(next_replay_event);
  threads[current_thread].awaiting_load_store = false; // next event after current may not be load or store
  assert(current_event.is_write() || current_event.is_read());
  assert(0 <= current_thread && current_thread < long(threads.size()));
  
  //update last_write
  if(current_event.is_write()){
    last_write[current_event.object] = current_event.iid;
    mem[current_event.object] = current_event.value;
  }
  //update vpo when read is done
  if(current_event.is_read()) { 
    current_event.value = mem[current_event.object];
    threads[current_thread][current_event.get_index()].value = current_event.value;
  }

  if (prefix_idx == states[current_state].lead_head_execution_prefix)
    alpha_head = prefix_idx;
}

void ViewEqTraceBuilder::replay_schedule(int *proc) {
  IID<IPid> next_replay_event = execution_sequence[prefix_idx];
  int next_replay_thread = next_replay_event.get_pid();
  assert(threads[next_replay_thread].available);
  
  if (prefix_state[prefix_idx] < 0) replay_non_memory_access(next_replay_thread, next_replay_event);
  else replay_memory_access(next_replay_thread, next_replay_event);

  *proc = next_replay_thread;
  prefix_idx++;
}

bool ViewEqTraceBuilder::exists_non_memory_access(int *proc) {
  // reverse loop to prioritize newly created threads
  for (int i = threads.size()-1; i >=0 ; i--) {
    if (threads[i].available && !threads[i].awaiting_load_store) {
      IID<IPid> iid = IID<IPid>(IPid(i), threads[i].size());
      Event non_global(iid);
      threads[i].push_back(non_global);
      execution_sequence.push_back(iid);
      prefix_state.push_back(-1);
      prefix_idx++;

      current_thread = i;
      *proc = i;
      return true;
    }
  }

  return false;
}

void ViewEqTraceBuilder::make_new_state() {
  State next_state; // new state
  next_state.sequence_prefix = prefix_idx;
  next_state.forbidden = forbidden;
  next_state.mem = mem;
  
  current_state = states.size();
  states.push_back(next_state);
}

void ViewEqTraceBuilder::compute_new_leads() {
  if (to_explore.empty()) {
    IID<IPid> next_event;
    std::pair<bool, IID<IPid>> enabled_read_of_RW = enabaled_RWpair_read();
    if (enabled_read_of_RW.first == true) {
      next_event = enabled_read_of_RW.second; // read event of RW pair (event removed from enabled)
      analyse_unexplored_influenecers(next_event);

      states[current_state].performed_fwd_analysis = true;
      states[current_state].fwd_read = next_event;
    }
    else { //no read of RW-pair available in enabled
      next_event = Enabled.front(); // pick any event from enabled
    }

    update_leads(next_event, forbidden);
    
    if (!states[current_state].has_unexplored_leads()) {
      Sequence seq(next_event, this);
      Lead l(seq, forbidden, mem); 
      consistent_union(current_state, l);
    }
    states[current_state].lead_head_execution_prefix = prefix_idx;
    alpha_head = prefix_idx;
  }
  else { // has a lead to explore
    states[current_state].executing_alpha_lead = true;
    states[current_state].lead_head_execution_prefix = alpha_head;

    // lead of remaining start
    Lead l(to_explore, forbidden);
    consistent_union(current_state, l);
    to_explore.pop_front();
  }
}

void ViewEqTraceBuilder::execute_next_lead() {
  // ////
  // out << "has unexplored leads\n";
  // // [snj]: TODO remove - only for debug
  // out << "unexplored leads:\n";
  // std::vector<Lead> unexl = states[current_state].unexplored_leads();
  // for (auto it = unexl.begin(); it != unexl.end(); it++) {
  //   out << "lead: ";
  //   out << it->to_string();
  //   out << " = " << it->merged_sequence.to_string() << "\n";
  // }
  // ////
  Lead next_lead = states[current_state].next_unexplored_lead();
  Event next_Event = states[current_state].alpha_sequence().head();
  IID<IPid> next_event = next_Event.iid;
  to_explore = states[current_state].alpha_sequence().tail();
  auto it = std::find(Enabled.begin(), Enabled.end(), next_event);
  // if (it == Enabled.end()) {
  //   out << "not found " << next_event << " in Enabled\n";
  //   out << "current explored seq=" << execution_sequence.to_string() << "\n";
  //   out << "Enabled:\n";
  //   for (auto enit = Enabled.begin(); enit != Enabled.end(); enit++) {
  //     out << (*enit) << ", ";
  //   }
  //   out << "\n";
  // }
  assert(it != Enabled.end());
  Enabled.erase(it);
  
  // update forbidden with lead forbidden and read event executed
  forbidden = next_lead.forbidden;
  if(next_Event.is_read()){ // update formula for value read
      forbidden.reduce(std::make_pair(next_Event.iid, mem[next_Event.object]));

      if (!to_explore.empty() && next_Event.corresponding_rmw_store(to_explore.head()))
        to_explore = to_explore.tail(); // next event in to_explore is rmw write, executed with read
  }
  
  current_thread = next_event.get_pid();
  current_event = threads[current_thread][next_event.get_index()];
  assert(current_event.is_write() || current_event.is_read());
  assert(0 <= current_thread && current_thread < long(threads.size()));

  //update memory
  if(current_event.is_write()){
    last_write[current_event.object] = current_event.iid;
    mem[current_event.object] = current_event.value;
  }
  
  //update read value
  if(current_event.is_read()) { 
    current_event.value = mem[current_event.object];
    threads[current_thread][current_event.get_index()].value = current_event.value;    
  }

  // record current event as next in execution sequence
  execution_sequence.push_back(current_event.iid);
  prefix_state.push_back(current_state);
  prefix_idx++;
  threads[current_thread].awaiting_load_store = false; // next event after current may not be load or store
}

void ViewEqTraceBuilder::analyse_unexplored_influenecers(IID<IPid> read_event) {
  std::unordered_set<IID<IPid>> ui = unexploredInfluencers(get_event(read_event), forbidden);
 
  for (auto it = ui.begin(); it != ui.end(); it++) {
    Event ui_event = threads[it->get_pid()][it->get_index()];
    update_leads(ui_event, forbidden);
  }
}

void ViewEqTraceBuilder::forward_analysis(Event event, SOPFormula& forbidden) {
  std::unordered_set<IID<IPid>> ui = unexploredInfluencers(event, forbidden);
 
  SOPFormula fui_all = forbidden;
  for (auto it = ui.begin(); it != ui.end(); it++) {
    if (threads[(*it).get_pid()][(*it).get_index()].value == mem[event.object]) 
      continue; // ui value = current value and current value is not forbidden 
    fui_all || std::make_pair(event.iid, threads[(*it).get_pid()][(*it).get_index()].value);
  }
 
  std::vector<Lead> L;

  if (!(forbidden.check_evaluation(std::make_pair(event.iid, mem[event.object])) == RESULT::TRUE))
    L.push_back(Lead(Sequence(event.iid, this), fui_all, mem));
   
  for (auto it = ui.begin(); it != ui.end(); it++) {
    if (get_event(*it).value == mem[event.object]) 
      continue; // ui value = curretn value, this value already seen in first forward lead

    Sequence start(event.iid, this);
    start.push_front(*it);

    SOPFormula fui = forbidden; 
    fui || (std::make_pair(event.iid, mem[event.object]));
    for (auto it2 = ui.begin(); it2 != ui.end(); it2++) {
      if (it2 != it)
        fui || std::make_pair(event.iid, get_event((*it2)).value);
    }
  
    Lead l(start, fui, mem);
    L.push_back(l);
  }

  consistent_union(current_state, L); // add leads at current state
}

int ViewEqTraceBuilder::union_state_start (int prefix_idx, IID<IPid> event, Sequence& start) {
  int event_index = prefix_state[prefix_idx];

  if (states[event_index].executing_alpha_lead) { // part of of alpha, alpha cannot be modified, move to head of alpha      
    Sequence alpha_prefix(execution_sequence[states[event_index].lead_head_execution_prefix], this);
    int ex_idx = states[event_index].lead_head_execution_prefix + 1;
    while (execution_sequence[ex_idx] != event) {
      if (prefix_state[ex_idx] >= 0) // this event has a state => this event is a global R/W
        alpha_prefix.push_back(execution_sequence[ex_idx]);
      ex_idx++;
    }
    
    event_index = prefix_state[states[event_index].lead_head_execution_prefix];
    alpha_prefix.concatenate(start);
    start = alpha_prefix;
  }

  return event_index;
}

ViewEqTraceBuilder::EventSequence ViewEqTraceBuilder::get_constraint(int state_id, Sequence& start, Event event) {
  EventSequence constraint = states[state_id].alpha_sequence();
  if (!states[state_id].performed_fwd_analysis || !event.is_rmw)
    return constraint;

  Event fwd_read = *constraint.find_iid(states[state_id].fwd_read);
  if (!fwd_read.is_rmw)
    return constraint;

  constraint.clear();
  return constraint;
}

bool ViewEqTraceBuilder::indepenent_event_in_leads(int state, IID<IPid> event) {
  bool found_event_in_another_lead = false;
  int  previously_found_value = -42; // dummy value

  for (auto l = states[state].leads.begin(); l != states[state].leads.end(); l++) {
    auto e = l->merged_sequence.find_iid(event);
    if (e != l->merged_sequence.end()) { // event found in current lead
      if (found_event_in_another_lead) { // event found in another lead of this state
        if (e->value != previously_found_value) { // values of te event don't match in two leads
          return false;
        }
      }
      else { // event not found in another lead of this state
        previously_found_value = e->value;
        found_event_in_another_lead = true;
      }
    }
  }

  return true;
}

bool ViewEqTraceBuilder::is_independent_EW_lead(Sequence& start, IID<IPid> write_event, IID<IPid> read_event) {
  assert(get_event(start.last()).is_read() && start.last() == read_event);

  int e = execution_sequence.size()-1; // last executed event
  int s = start.size()-2;              // start = s.(read_event)
  if (start[s] == write_event) s--;

  for (; e >= 0 && s >= 0; e--) {
    if (execution_sequence[e] == write_event) continue;
    if (execution_sequence[e] != start[s]) continue;

    int state = prefix_state[e];
    if (states[state].executing_alpha_lead) {
      state = prefix_state[states[state].lead_head_execution_prefix];
    }
    assert (state >= 0);

    if (states[state].leads.size() > 1) {
      if (!indepenent_event_in_leads(state, execution_sequence[e])) {
        return false;
      }
    }

    s--;
    if (start[s] == write_event) s--;
  }

  return true;
}

void ViewEqTraceBuilder::add_EW_leads(int state) {
  if (EW_leads.find(state) == EW_leads.end()) return; // no pending EW leads

  // EW_leads[state] = read -> value -> list of leads 
  for (auto it = EW_leads[state].begin(); it != EW_leads[state].end(); it++) {
    // it = (read, value -> list of leads)
    for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
      // it2 = (value, list of leads)
      consistent_union(state, it2->second);
      forbid_value_for_ei_leads(state, it->first, it2->first);
    }
  }

  EW_leads.erase(state);
}

void ViewEqTraceBuilder::forbid_value_for_ei_leads(int state, IID<IPid> read, int value) {
  if (!states[state].performed_fwd_analysis) return;
  if (states[state].fwd_read != read) return;
  // forward-analysis was perfromed for 'read' at 'state'
  for (auto it = states[state].ei_leads.begin(); it != states[state].ei_leads.end(); it++) {
    std::vector<Lead> updated_leads;
    
    for (auto l = it->second.begin(); l != it->second.end(); l++) {
      SOPFormula new_forbidden = l->forbidden;
      new_forbidden || std::make_pair(read, value);
      updated_leads.push_back(Lead((*l), new_forbidden));
    }
    
    states[state].ei_leads[it->first] = updated_leads;
  }
}

void ViewEqTraceBuilder::backward_analysis_read(Event event, SOPFormula& forbidden, std::unordered_map<int, std::vector<Lead>>& L) {
  std::unordered_set<IID<IPid>> ui = unexploredInfluencers(event, forbidden);
  std::unordered_set<IID<IPid>> ei = exploredInfluencers(event, forbidden);
  
  std::unordered_set<int> ui_values;
  SOPFormula fui(std::make_pair(event.iid, mem[event.object]));
  for (auto it = ui.begin(); it != ui.end(); it++) {
    fui || std::make_pair(event.iid, get_event(*it).value);
    ui_values.insert(get_event(*it).value);
  }
  
  for (auto it = ei.begin(); it != ei.end(); it++) {
    Event eit = get_event(*it);
    if (eit.value == mem[event.object] || ui_values.find(eit.value) != ui_values.end())
      continue; // skip current value, it is covered in fwd analysis
    if (std::find(covered_read_values[event.get_iid()].begin(), covered_read_values[event.get_iid()].end(), eit.value)
      != covered_read_values[event.get_iid()].end())
      continue; // skip current value, it is covered by another ei
    
    Sequence start = execution_sequence.backseq((*it), event.iid);
    if (start.empty()) // cannot form view-start for (*it --rf--> event)
      continue;

    int es_idx = execution_sequence.index_of((*it)); // index in execution_sequnce of ei
    IID<IPid> event_at_union_state = (*it); // event explored from state of lead union
    if (eit.is_rmw) {
      // if the evnt is rmw (write), then shift 1 up to its corresponding read
      es_idx--;
      event_at_union_state = threads[it->get_pid()][it->get_index()-1].iid;
    }

    int event_index = union_state_start(es_idx, event_at_union_state, start);
    
    EventSequence constraint = get_constraint(event_index, start, event);
    
    SOPFormula fei;
    for (auto i = ei.begin(); i != ei.end(); i++) {
      if (i == it) continue;
      fei || std::make_pair(event.iid, get_event((*i)).value);
    }
    
    SOPFormula inF = states[event_index].leads[states[event_index].alpha].forbidden;
    
    inF || fui; inF || fei;
    Lead back_lead(constraint, start, inF, states[event_index].mem);
    L[event_index].push_back(back_lead);

    covered_read_values[event.get_iid()].push_back(eit.value);
  }
}

void ViewEqTraceBuilder::backward_analysis_write(Event event, SOPFormula& forbidden, std::unordered_map<int, std::vector<Lead>>& L) {
  std::unordered_set<IID<IPid>> ew = exploredWitnesses(event, forbidden);
  
  for (auto it = ew.begin(); it != ew.end(); it++) {
    int it_val = get_event((*it)).value;
    if (it_val == event.value)
      continue;

    if (std::find(covered_read_values[(*it)].begin(), covered_read_values[(*it)].end(), event.value) != covered_read_values[(*it)].end())
      continue; // value covered for EW read by another write of same value in current execution

    Sequence start = execution_sequence.backseq((*it), event.iid);
    if (start.empty()) // cannot form view start for (event --rf--> *it)
      continue;

    int event_index = union_state_start(execution_sequence.index_of((*it)), (*it), start);

    EventSequence constraint = get_constraint(event_index, start, event);
    SOPFormula inF = states[event_index].leads[states[event_index].alpha].forbidden;
    (inF || std::make_pair((*it),it_val));

    if (is_independent_EW_lead(start, event.iid, (*it))) {
      // the events in start are equivalent in all extensions from event_index state
      L[event_index].push_back(Lead(constraint, start, inF, states[event_index].mem));
      EW_leads[event_index][(*it)].erase(event.value);

      forbid_value_for_ei_leads(event_index, (*it), event.value);

      // add to the list of EW reads values covered
      covered_read_values[(*it)].push_back(event.value);
    }
    else {
      // the events in start are Not equivalent in all extensions from event_index state
      // the ew read may need different leads to get the valuei under different contexts
      EW_leads[event_index][(*it)][event.value].push_back(Lead(constraint, start, inF, states[event_index].mem));
    }
  }
}

void ViewEqTraceBuilder::backward_analysis(Event event, SOPFormula& forbidden) {
  std::unordered_map<int, std::vector<Lead>> L; // map of execution index -> leads to be added at state[execution index]

  if (event.is_read()) {
    backward_analysis_read(event, forbidden, L);
    states[current_state].ei_leads = L;
  }
  else if (event.is_write()) {
    backward_analysis_write(event, forbidden, L);
    for (auto it = L.begin(); it != L.end(); it++) {
     consistent_union(it->first, it->second); // consistent join at respective execution prefix
    }
  }
}

void ViewEqTraceBuilder::prefix_analysis(Lead& l1, Lead& l2, std::vector<Lead>& added_leads, int state) {
  Sequence seq = l1.merged_sequence.to_iid_sequence();
  SOPFormula f = l1.forbidden;
  f && l2.forbidden;

  EventSequence l2_suffix(this);
  for (auto it = l2.merged_sequence.begin(); it != l2.merged_sequence.end(); it++) {
    if (l1.merged_sequence.find_iid(it->iid) == l1.merged_sequence.end()) 
      l2_suffix.push_back(*it);
  }

  for (auto it = l1.merged_sequence.begin(); it != l1.merged_sequence.end(); it++) {
    if (l2.merged_sequence.find_iid(it->iid) != l2.merged_sequence.end()) 
      continue; // events in common prefix are not analyzed

    for (auto it2 = l1.merged_sequence.begin(); it2 != it; it2++) {
      Event eread, ewrite;
      if (!it->RWpair(*it2))
        continue;

      if (it->is_read()) {
        eread = (*it);
        ewrite = (*it2);
      }
      else {
        eread = (*it2);
        ewrite = (*it);
      }

      if (eread.value == ewrite.value)
        continue; // current value
      
      if (f.check_evaluation(std::make_pair(eread.iid, ewrite.value)) == RESULT::TRUE)
        continue; // forbidden value
      
      Sequence start = seq.backseq(it2->iid, it->iid, l1.merged_sequence);
      if (start.empty())
        continue;
      
      SOPFormula f1 = f;
      f || std::make_pair(eread.iid, eread.value);
      EventSequence constraint = l1.merged_sequence; 
      constraint.erase(eread.iid); constraint.concatenate(l2_suffix);
      
      added_leads.push_back(Lead(constraint, start, f1, states[state].mem));
    }
  }
}

bool ViewEqTraceBuilder::ordered_by_join(IID<IPid> e1, IID<IPid> e2) {
  if (join_summary[e2.get_pid()].find(e1.get_pid()) == join_summary[e2.get_pid()].end())
    return false; // e1's thread does not join in e2's thread

  if (e2.get_index() < join_summary[e2.get_pid()][e1.get_pid()])
    return false; // e2 occurs before the join of e1's thread

  return true; // e2 occurs after the join of e1's thread
}

bool ViewEqTraceBuilder::ordered_by_spawn(IID<IPid> e1, IID<IPid> e2) {
  if (spawn_summary[e1.get_pid()].find(e2.get_pid()) == spawn_summary[e1.get_pid()].end())
    return false; // e1's thread doesn not spawn e2's thread

  if (e1.get_index() > spawn_summary[e1.get_pid()][e2.get_pid()])
    return false; // e1 occurs after the spawn event

  return true; // e1 occurs before the spwan the e2's thread
}

void ViewEqTraceBuilder::update_leads(Event event, SOPFormula& forbidden) {
  assert(event.is_read() || event.is_write());
  
  if (event.is_read()) {
    forward_analysis(event, forbidden); // add leads at current state
  }
  
  backward_analysis(event, forbidden); // add leads at previous states
}

void ViewEqTraceBuilder::mark_available(int proc, int aux)
{
  threads[proc].available = true;
}

void ViewEqTraceBuilder::mark_unavailable(int proc, int aux)
{
  threads[proc].available = false;
}

bool ViewEqTraceBuilder::is_enabled(int thread_id) {
  for (auto i = Enabled.begin(); i != Enabled.end(); i++) {
    if ((*i).get_pid() == thread_id)
      return true;
  }
  return false;
}

std::pair<bool, IID<IPid>> ViewEqTraceBuilder::enabaled_RWpair_read() {
  for (auto i = Enabled.begin(); i != Enabled.end(); i++) {
    for (auto j = i+1; j != Enabled.end(); j++) {
      int tid1 = (*i).get_pid();
      int tid2 = (*j).get_pid();

      int eid1 = (*i).get_index();
      int eid2 = (*j).get_index();

      if (threads[tid1][eid1].same_object(threads[tid2][eid2])) {
        if (threads[tid1][eid1].is_read() && threads[tid2][eid2].is_write()) {
          return std::make_pair(true, (*i));
        }
        if (threads[tid1][eid1].is_write() && threads[tid2][eid2].is_read()) {
          return std::make_pair(true, (*j));
        }
      }
    }
  }

  return std::make_pair(false, dummy_id);
}

int ViewEqTraceBuilder::current_value(std::pair<unsigned, unsigned> obj) {
  return mem[obj];
}

void ViewEqTraceBuilder::Sequence::add_rmw_write(IID<IPid> rmw_read, IID<IPid> rmw_write) {
  auto it = find(rmw_read);
  if (it != end())
    push_at(it + 1, rmw_write);
}

bool ViewEqTraceBuilder::Sequence::hasRWpairs(Sequence& seq) {
  for (sequence_iterator it1 = begin(); it1 != end(); it1++) {
    Event e1 = container->get_event(*it1);
    for (sequence_iterator it2 = seq.begin(); it2 != seq.end(); it2++) {
      Event e2 = container->get_event(*it2);
      if (e1.same_object(e2)) {
        if (e1.is_read() && e2.is_write()) return true;
        if (e1.is_write() && e2.is_read()) return true;
      }
    }
  }

  return false;
}

void ViewEqTraceBuilder::metadata(const llvm::MDNode *md)
{
  IID<IPid> current_event = execution_sequence[prefix_idx-1];
  threads[current_event.get_pid()][current_event.get_index()].md = md;
  
  last_md = md;
}

bool ViewEqTraceBuilder::record_symbolic(SymEv event)
{
  out << "in record symbolic\n";
  assert(false);
  return false;
}

void ViewEqTraceBuilder::finish_up_lead(int replay_state_prefix) {
  add_EW_leads(replay_state_prefix);
  int replay_execution_prefix = states[replay_state_prefix].sequence_prefix;
  IID<IPid> event = execution_sequence[replay_execution_prefix];
  covered_read_values.erase(event);
}

void ViewEqTraceBuilder::finish_up_state(int replay_state_prefix) {
  for (auto it = states[replay_state_prefix].ei_leads.begin(); 
      it != states[replay_state_prefix].ei_leads.end(); it++) {
    consistent_union(it->first, it->second);
  }
}

int ViewEqTraceBuilder::find_replay_state_prefix() {
  int replay_state_prefix = states.size() - 1;
  for (auto it = states.end(); it != states.begin();) {
    it--;
    finish_up_lead(replay_state_prefix);
    
    if (it->has_unexplored_leads()) { // found replay state      
      break;
    }

    finish_up_state(replay_state_prefix);
    replay_state_prefix --;
  }

  return replay_state_prefix;
}

bool ViewEqTraceBuilder::reset() {
  if (check_optimality)
    record_redundant();

  int replay_state_prefix = find_replay_state_prefix();
  if (replay_state_prefix < 0) {// no more leads to explore, model checking complete
    if (check_optimality)
      report_redundant();
    replay_point = -1;
    return false; 
  }

  int replay_execution_prefix = states[replay_state_prefix].sequence_prefix;
  execution_sequence.erase(execution_sequence.begin() + replay_execution_prefix + 1, execution_sequence.end());
  prefix_state.erase(prefix_state.begin() + replay_execution_prefix + 1, prefix_state.end());
  states.erase(states.begin() + replay_state_prefix + 1, states.end());
  
  to_explore.clear();
  // CPS = CPidSystem();
  
  threads.clear();
  threads.push_back(Thread(CPid(), -1));
  // mutexes.clear();
  // cond_vars.clear();
  
  mem.clear();
  last_write.clear();

  forbidden = states[replay_state_prefix].forbidden;

  prefix_idx = 0;
  replay_point = replay_execution_prefix;
  
  // last_full_memory_conflict = -1;
  // dryrun = false;
  // dry_sleepers = 0;
  last_md = 0; // [snj]: TODO ??
  reset_cond_branch_log();

  out.flush();

  return true;
}

IID<CPid> ViewEqTraceBuilder::get_iid() const{
  int i = execution_sequence.size() - 1;
  IPid pid = execution_sequence[i].get_pid();
  int  idx = execution_sequence[i].get_index() + 1;
  return IID<CPid>(threads[pid].cpid, idx);
}

void ViewEqTraceBuilder::refuse_schedule() {
  execution_sequence.pop_back();
  prefix_state.pop_back();
  prefix_idx--;

  threads[current_thread].pop_back();
  mark_unavailable(current_thread);
}

bool ViewEqTraceBuilder::is_replaying() const {
  return prefix_idx < replay_point;
}

bool ViewEqTraceBuilder::at_replay_point() {
  return prefix_idx == replay_point;
}

void ViewEqTraceBuilder::cancel_replay() { replay_point = -1; }

bool ViewEqTraceBuilder::full_memory_conflict() {return false;} //[snj]: TODO

void ViewEqTraceBuilder::update_join_summary(Event event) {
  int joined_thread = event.object.first;
  if (join_summary[event.get_pid()].find(joined_thread) != join_summary[event.get_pid()].end())
    return; // already joined directly or indirectly

  // direct join : join event of joined_thread is in event's thread
  join_summary[event.get_pid()][joined_thread] = event.get_index();

  // indirect join : join event of thread t is in joined_thread
  for (auto it = join_summary[joined_thread].begin(); it != join_summary[joined_thread].end(); it++) {
    if (join_summary[event.get_pid()].find(it->first) == join_summary[event.get_pid()].end()) {
      join_summary[event.get_pid()][it->first] = event.get_index();
    }
  }
}

void ViewEqTraceBuilder::update_spawn_summary(Event event) { // t0 -> (t1 -> t2)
  int spawned_thread = event.object.first;
  spawn_summary[event.get_pid()][spawned_thread] = event.get_index();

  for (auto it = spawn_summary.begin(); it != spawn_summary.end(); it++) {
    auto t = it->second.find(event.get_pid());
    if (t != it->second.end()) { // event's thread was spawned by it
      spawn_summary[it->first][spawned_thread] = t->second;
    }
  }
}

bool ViewEqTraceBuilder::spawn()
{
  Event event(SymEv::Spawn(threads.size()));
  event.make_spawn();
  event.object = std::make_pair(threads.size(), 0);

  // remove the no_load_store event added by exists_non_memory access 
  // add replace with the event crated in this fucntion.
  threads[current_thread].pop_back();
  execution_sequence.pop_back();
  
  // create-event in thread that is spawning a new event
  current_event = event;
  current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].size());
  update_spawn_summary(current_event);

  threads[current_thread].push_back(current_event);
  execution_sequence.push_back(current_event.iid);

  // setup new program thread
  IPid parent_ipid = current_event.iid.get_pid();
  CPid child_cpid = CPS.spawn(threads[parent_ipid].cpid);
  /* Spawn event of thread is dummy */
  threads.push_back(Thread(child_cpid, -42));

  return true;
}

bool ViewEqTraceBuilder::join(int tgt_proc) {
  Event event(SymEv::Join(tgt_proc));
  event.make_join();
  event.object = std::make_pair(tgt_proc, 0); // join of tgt_proc, thread_id of tgt_proc saved in event object

  // remove the no_load_store event added by exists_non_memory access 
  // add replace with the event crated in this fucntion.
  threads[current_thread].pop_back();
  execution_sequence.pop_back();

  current_event = event;
  current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].size());
  update_join_summary(current_event);
  
  threads[current_thread].push_back(current_event);
  execution_sequence.push_back(current_event.iid);

  return true;
}

bool ViewEqTraceBuilder::store(const SymData &ml) {
  // visitStoreInst in Execution.cpp lands in atomic_store not here
  Event event(SymEv::Store(ml));
  event.make_write();
  current_event = event;

  if (current_event.is_global()) {
    /* a global event is peeked from Execution.cpp run(), this is function
       call is a result of the peek. Hence, a no_load_store event was not adeed 
       by exists_non_memory_access function and thus there is no pop in case
       of a global event.
    */
    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].size());
    threads[current_thread].push_back(current_event);
    threads[current_thread].awaiting_load_store = true;

    assert(!is_enabled(current_thread)); // only 1 event of each thread is enabled
    Enabled.push_back(current_event.iid);
  }
  else {
    /* exists_non_memory_access adds a no_load_store event in thread before executing
       the next event of thread, if the next event is a non-global load or strore
       then the no_load_store event is popped and load/store event in added to the
       thread of the event.
    */
    threads[current_thread].pop_back();
    execution_sequence.pop_back();

    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].size());
    threads[current_thread].push_back(current_event);
    execution_sequence.push_back(current_event.iid);
  }

  return true;
}

bool ViewEqTraceBuilder::atomic_store(const SymData &ml) {
  // visitStoreInst in Execution.cpp lands here not in store
  Event event(SymEv::Store(ml));
  event.make_write();
  current_event = event;

  if (current_event.is_global()) {
    /* a global event is peeked from Execution.cpp run(), this is function
       call is a result of the peek. Hence, a no_load_store event was not adeed 
       by exists_non_memory_access function and thus there is no pop in case
       of a global event.
    */
    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].size());
    threads[current_thread].push_back(current_event);
    threads[current_thread].awaiting_load_store = true;

    assert(!is_enabled(current_thread)); // only 1 event of each thread is enabled
    Enabled.push_back(current_event.iid);
  }
  else {
    /* exists_non_memory_access adds a no_load_store event in thread before executing
       the next event of thread, if the next event is a non-global load or strore
       then the no_load_store event is popped and load/store event in added to the
       thread of the event.
    */
    threads[current_thread].pop_back();
    execution_sequence.pop_back();

    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].size());
    threads[current_thread].push_back(current_event);
    execution_sequence.push_back(current_event.iid);
  }

  return true;
}

bool ViewEqTraceBuilder::load(const SymAddrSize &ml) {
  Event event(SymEv::Load(ml));
  event.make_read();
  event.value = mem[event.object];
  current_event = event;

  if (current_event.is_global()) {
    /* a global event is peeked from Execution.cpp run(), this is function
       call is a result of the peek. Hence, a no_load_store event was not adeed 
       by exists_non_memory_access function and thus there is no pop in case
       of a global event.
    */
    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].size());
    threads[current_thread].push_back(current_event);
    threads[current_thread].awaiting_load_store = true;

    assert(!is_enabled(current_thread)); // only 1 event of each thread is enabled
    Enabled.push_back(current_event.iid);
  } 
  else {
    /* exists_non_memory_access adds a no_load_store event in thread before executing
       the next event of thread, if the next event is a non-global load or strore
       then the no_load_store event is popped and load/store event in added to the
       thread of the event.
    */
    threads[current_thread].pop_back();
    execution_sequence.pop_back();

    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].size());
    threads[current_thread].push_back(current_event);
    execution_sequence.push_back(current_event.iid);
  }

  return true;
}

bool ViewEqTraceBuilder::fence() {
  // invoked at pthread create
  return true;
}

Trace* ViewEqTraceBuilder::get_trace() const {
  std::vector<IID<CPid>> cmp;
  SrcLocVectorBuilder cmp_md;
  std::vector<Error *> errs;
  
  for (unsigned i = 0; i < execution_sequence.size(); i++) {
    cmp.push_back(IID<CPid>(threads[execution_sequence[i].get_pid()].cpid, execution_sequence[i].get_index()+1));
    cmp_md.push_from(get_event(execution_sequence[i]).md);
  };

  for (unsigned i = 0; i < errors.size(); ++i)
  {
    errs.push_back(errors[i]->clone());
  }

  Trace *t = new IIDSeqTrace(cmp, cmp_md.build(), errs);
  
  return t;
} 

// HOW TO READ
//  - stack operations have been abstracted ( as they are mostly parameters 'void *arg')
//  - heap operations also abstracted for now
//  - Load(Global(object_id)(block_size))(Event::value)
//  - Store(Global(object_id)(block_size),value)(Event::value)
void ViewEqTraceBuilder::debug_print() const {
  // out.SetUnbuffered(); // flush every write immediately, no bufferring

  // out << "Debug_print: execution_sequence.size()=" << execution_sequence.size() << "\n";
  for (int i = 0; i < execution_sequence.size(); i++) {
    int thread = execution_sequence[i].get_pid();
    Event event = threads[thread][execution_sequence[i].get_index()];
   
    if (i < 10) out << " [" << i << "]: "; 
    else out << "[" << i << "]: ";

    if (event.symEvent.size() < 1) {
      out << "--\n";
      continue;
    }
    if (event.sym_event().addr().addr.block.is_stack()) {
      out << "Stack Operation \n";
      continue;
    }
    if (event.sym_event().addr().addr.block.is_heap()) {
      out << "Heap Operation \n";
      continue;
    }
    out << event.to_string() << "\n";

    if (i==100) out.flush();
  }
} 

void ViewEqTraceBuilder::record_redundant() {
  std::unordered_map<IID<IPid>, int> rv_map;

  for (int i = 0; i < execution_sequence.size(); i++) {
    if (prefix_state[i] >= 0) { // is a global event
      Event event = get_event(execution_sequence[i]);
      if (event.is_read()) {
        rv_map[event.iid] = event.value;
      }
    }
  }

  explored_sequences_summary.push_back(rv_map);

  for (int i = 0; i < explored_sequences_summary.size()-1; i++) {
    if (explored_sequences_summary[i] == rv_map) {
      redundant[(i+1)].push_back(explored_sequences_summary.size());
      break;
    }
  }
}

void ViewEqTraceBuilder::report_redundant() {
  if (redundant.size() > 0) {
    out << "\nRedundant explorations:\n";
    for (auto it = redundant.begin(); it != redundant.end(); it++) {
      out << "Trace#" << it->first;
      for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
        out << ", Trace#" << (*it2);
      }
      out << "\n";
    }
  }
  else {
    out << "Optimal Exploration\n";
  }
}

void ViewEqTraceBuilder::enable_rmw(const SymAddrSize &ml) {
  bool ret = load(ml); 
  // update load's rmw status
  threads[current_thread].events[threads[current_thread].size()-1].is_rmw = true;
  threads[current_thread].events[threads[current_thread].size()-1].rmw_operation = current_rmw_operation.first;
  threads[current_thread].events[threads[current_thread].size()-1].rmw_value = current_rmw_operation.second;  
  
  current_event.is_rmw = true; // same as last event of current thread ie the load event
  current_event.rmw_operation = current_rmw_operation.first;
  current_event.rmw_value = current_rmw_operation.second;

  if (current_rmw_operation.first == RMWOperation::CMPXCHG) {
    threads[current_thread].events[threads[current_thread].size()-1].expected_value = current_cmpxchg_expected_value;
    current_event.expected_value = current_cmpxchg_expected_value;
  }
}

void ViewEqTraceBuilder::complete_rmw(const SymData &ml) {
  Event event(SymEv::Store(ml));
  event.make_write();
  current_event = event;

  current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].size());
  current_event.is_rmw = true;
  current_event.rmw_operation = threads[current_thread][threads[current_thread].size()-1].rmw_operation;
  current_event.rmw_value = threads[current_thread][threads[current_thread].size()-1].rmw_value;
  if (current_event.rmw_operation == RMWOperation::CMPXCHG)
    current_event.expected_value = threads[current_thread][threads[current_thread].size()-1].expected_value;
  threads[current_thread].push_back(current_event);

  // add to alpha lead (only added for forward leads)
  IID<IPid> corresponding_read_iid = threads[current_thread][threads[current_thread].size()-2].iid;
  int alpha = states[prefix_state[states[current_state].lead_head_execution_prefix]].alpha;
  states[prefix_state[states[current_state].lead_head_execution_prefix]].leads[alpha].add_rmw_write(corresponding_read_iid, current_event.iid);
  
  // add to backward leads of corresponding read
  for (auto s = states[current_state].ei_leads.begin(); s != states[current_state].ei_leads.end(); s++) {
    for (auto l = s->second.begin(); l != s->second.end(); l++) {
      l->update_pending_rmw_write(current_event.iid);
    }
  }

  if (!is_replaying()) {
    update_leads(current_event, forbidden);

    // record current event as next in execution sequence
    execution_sequence.push_back(current_event.iid);
    prefix_state.push_back(current_state);
  }

  last_write[current_event.object] = current_event.iid;
  mem[current_event.object] = current_event.value;

  prefix_idx++;
}

void ViewEqTraceBuilder::set_rmw_operation(RMWOperation op, int val) {
  current_rmw_operation = std::make_pair(op, val);
}

void ViewEqTraceBuilder::set_rmw_operation(RMWOperation op, int expected_val, int xchg_val) {
  assert(op == RMWOperation::CMPXCHG);
  current_rmw_operation = std::make_pair(op, xchg_val);
  current_cmpxchg_expected_value = expected_val;
}

int ViewEqTraceBuilder::compute_modified_value(RMWOperation op, int read_value, int new_value, int exp_value) {
  switch(op) {
    case RMWOperation::CMPXCHG:
      if (read_value == exp_value)
        return new_value;
      return read_value;
    case RMWOperation::XCHG:
      return new_value;
    case RMWOperation::ADD:
      return read_value + new_value;
    case RMWOperation::SUB:
      return read_value - new_value;
    case RMWOperation::AND:
      return read_value & new_value;
    case RMWOperation::NAND:
      return ~(read_value & new_value);
    case RMWOperation::OR:
      return read_value | new_value;
    case RMWOperation::XOR:
      return read_value ^ new_value;
    case RMWOperation::MAX:
    case RMWOperation::UMAX:
      return (read_value > new_value) ? read_value : new_value;
    case RMWOperation::MIN:
    case RMWOperation::UMIN:
      return (read_value < new_value) ? read_value : new_value;
  }

  out << "WHY HERE?\n";
  assert(false); // code must not reach here
  return 0; 
}

// rmw read and write are stored as separate events in the execution sequence
// and the corresponding thread events set
// however, there is only one state created for the read of rmw
bool ViewEqTraceBuilder::atomic_rmw(const SymData &ml) {
  Event read_event(SymEv::Load(ml.get_ref()));
  read_event.make_read();
  read_event.value = mem[read_event.object];
  
  if (read_event.is_global()) {
    if (threads[current_thread].size() > 0) {
      Event last_thread_event = threads[current_thread].events[threads[current_thread].size()-1];
      if (read_event.sym_event() == last_thread_event.sym_event()) {
        complete_rmw(ml); // load of rmw peeked earlier, now complete store
        return true;
      }
    }
    enable_rmw(ml.get_ref()); // peeking new rmw event (peeking load)
  } 
  else {
    /* exists_non_memory_access adds a no_load_store event in thread before executing
       the next event of thread, if the next event is a non-global load or strore
       then the no_load_store event is popped and load+store events are added to the
       thread of the current_event.
    */
    threads[current_thread].pop_back();
    execution_sequence.pop_back();

    Event write_event(SymEv::Store(ml));
    write_event.make_write();
    
    read_event.is_rmw = true;
    write_event.is_rmw = true;

    read_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].size());
    threads[current_thread].push_back(read_event);
    execution_sequence.push_back(read_event.iid);

    write_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].size());
    threads[current_thread].push_back(write_event);
    execution_sequence.push_back(write_event.iid);
  }

  return true;
}

bool ViewEqTraceBuilder::compare_exchange(const SymData &sd, const SymData::block_type expected, bool success) {
  Event read_event(SymEv::Load(sd.get_ref()));
  read_event.make_read();
  read_event.value = mem[read_event.object];
  
  if (read_event.is_global()) {
    if (threads[current_thread].size() > 0) {
      Event last_thread_event = threads[current_thread].events[threads[current_thread].size()-1];
      if (read_event.sym_event() == last_thread_event.sym_event()) {
        IID<IPid> prev_last_write = last_write[current_event.object];
        complete_rmw(sd); // load of rmw peeked earlier, now complete store
        if (!success) {
          // restrore original value read
          threads[current_thread].events[threads[current_thread].size()-1].value = last_thread_event.value;
          last_write[current_event.object] = prev_last_write;
          mem[current_event.object] = last_thread_event.value;
        }
        return true;
      }
    }
    enable_rmw(sd.get_ref()); // peeking new rmw event (peeking load)
  } 
  else {
    /* exists_non_memory_access adds a no_load_store event in thread before executing
       the next event of thread, if the next event is a non-global load or strore
       then the no_load_store event is popped and load+store events are added to the
       thread of the current_event.
    */
    threads[current_thread].pop_back();
    execution_sequence.pop_back();

    Event write_event(SymEv::Store(sd));
    write_event.make_write();
    
    read_event.is_rmw = true;
    write_event.is_rmw = true;

    read_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].size());
    threads[current_thread].push_back(read_event);
    execution_sequence.push_back(read_event.iid);

    write_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].size());
    threads[current_thread].push_back(write_event);
    execution_sequence.push_back(write_event.iid);

    if (!success) {
      // restrore original value read
      threads[current_thread].events[threads[current_thread].size()-1].value = read_event.value;
    }
  }

  return true;
}

bool ViewEqTraceBuilder::sleepset_is_empty() const{out << "[snj]: sleepset_is_empty being invoked!!\n"; assert(false); return true;}
bool ViewEqTraceBuilder::check_for_cycles(){out << "[snj]: check_for_cycles being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::mutex_lock(const SymAddrSize &ml){out << "[snj]: mutex_lock being invoked!!\n"; assert(false); return true;}
bool ViewEqTraceBuilder::mutex_lock_fail(const SymAddrSize &ml){out << "[snj]: mutex_lock_fail being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::mutex_trylock(const SymAddrSize &ml){out << "[snj]: mutex_trylock being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::mutex_unlock(const SymAddrSize &ml){out << "[snj]: mutex_unlock being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::mutex_init(const SymAddrSize &ml){out << "[snj]: mutex_init being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::mutex_destroy(const SymAddrSize &ml){out << "[snj]: mutex_ destroy being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::cond_init(const SymAddrSize &ml){out << "[snj]: cond_init being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::cond_signal(const SymAddrSize &ml){out << "[snj]: cond_signal being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::cond_broadcast(const SymAddrSize &ml){out << "[snj]: cond_broadcast being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::cond_wait(const SymAddrSize &cond_ml,
                        const SymAddrSize &mutex_ml){out << "[snj]: cond_wait being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::cond_awake(const SymAddrSize &cond_ml,
                        const SymAddrSize &mutex_ml){out << "[snj]: cond_awake being invoked!!\n"; assert(false); return false;}
int ViewEqTraceBuilder::cond_destroy(const SymAddrSize &ml){out << "[snj]: cond_destroy being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::register_alternatives(int alt_count){out << "[snj]: register_alternatives being invoked!!\n"; assert(false); return false;}

void ViewEqTraceBuilder::Event::make_spawn() {
  type = SPAWN;
}

void ViewEqTraceBuilder::Event::make_join() {
  type = JOIN;
}

void ViewEqTraceBuilder::Event::make_read() {
  type = READ;
  object = std::make_pair(sym_event().addr().addr.block.get_no(), 0);
  if (sym_event().addr().addr.offset != 0) { // object is array element (not base element) 
    object.second = sym_event().addr().addr.offset / sym_event().addr().size;
  }
}

void ViewEqTraceBuilder::Event::make_write() {
  type = WRITE;
  uint8_t *valptr = sym_event()._written.get();
  value = (*valptr);
  object = std::make_pair(sym_event().addr().addr.block.get_no(), 0);
  if (sym_event().addr().addr.offset != 0) { // object is array element (not base element) 
    object.second = sym_event().addr().addr.offset / sym_event().addr().size;
  }
}

bool ViewEqTraceBuilder::Event::RWpair(Event e) {
  if (!same_object(e)) return false;
  if (is_write() && e.is_read()) return true;
  if (is_read() && e.is_write()) return true;
  return false;
}

bool ViewEqTraceBuilder::Event::same_object(Event event) {
  return ((object.first == event.object.first) && (object.second == event.object.second));
}

bool ViewEqTraceBuilder::Event::corresponding_rmw_store(Event e) {
  if (!is_rmw || !e.is_rmw) return false;
  if (get_pid() != e.get_pid()) return false;
  if (get_index() != e.get_index()-1) return false;
  return true;
}

bool ViewEqTraceBuilder::Event::operator==(Event event) {
  if (value != event.value)
    return false;

  return (symEvent == event.symEvent);
}

bool ViewEqTraceBuilder::Event::operator!=(Event event) {
  return !(*this == event);
}

std::string ViewEqTraceBuilder::Event::type_to_string() const {
  if (is_rmw) {
    switch(type) {
      case WRITE: return "rmw-Write"; break;
      case READ:  return "rmw-Read";  break;
    }
  }

  switch(type) {
    case WRITE: return "Write"; break;
    case READ:  return "Read";  break;
    case JOIN:  return "Join";  break;
    case SPAWN: return "Spawn"; break;
  }

  return "";
}

std::string ViewEqTraceBuilder::Event::to_string() const {
  if (type == READ || type == WRITE) {
    if (object.second == 0)
      return ("[" + std::to_string(get_pid()) + ":" + std::to_string(get_index()) + "] (" + type_to_string() + "(" + std::to_string(object.first) + "," + std::to_string(value) + "))");
    return ("[" + std::to_string(get_pid()) + ":" + std::to_string(get_index()) + "] (" + type_to_string() + "(" + std::to_string(object.first) + "+" + std::to_string(object.second) + "," + std::to_string(value) + "))");  
  }
  
  return ("[" + std::to_string(get_pid()) + ":" + std::to_string(get_index()) + "] (" + type_to_string() + ":thread" + std::to_string(object.first) + ")");
}

bool ViewEqTraceBuilder::Sequence::view_adjust(IID<IPid> e1, IID<IPid> e2) {
  if (container->ordered_by_join(e1, e2) || container->ordered_by_spawn(e1, e2)) 
    return false;

  std::vector<IID<IPid>> original_events = events;

  Event ev1 = container->get_event(e1);
  Event ev2 = container->get_event(e2);

  int n2 = index_of(e2);
  int n1 = index_of(e1);
  for (int i = n2 - 1, delim = 0; i >= n1; i--) { // from before e2 till e1
    if (events[i].get_pid() != e1.get_pid()) continue; // shift only events from e1's thread

    for (int j = i; j < n2 - delim; j++) {
      Event ecurr = container->get_event(events[j]);
      Event enext = container->get_event(events[j+1]);

      if (ecurr.RWpair(enext)) { // if cannot exchange
        events = original_events; // restrore original sequence
        return false;
      }

      // can exchange
      IID<IPid> tmp = events[j];
      events[j] = events[j+1];
      events[j+1] = tmp;
    }
    
    delim++; // shifted 1 event of e1's thread, now 1 location fixed
  }

  return true;
}

std::pair<bool, std::pair<IID<IPid>, IID<IPid>>> ViewEqTraceBuilder::Sequence::conflicts_with(Sequence& other_seq) {
  for(int i = 0; i < events.size(); i++){
    for(int j = i + 1; j < events.size() ; j++){
      if(other_seq.has(events[i]) && other_seq.has(events[j])){
        sequence_iterator it1 = other_seq.find(events[i]);
        sequence_iterator it2 = other_seq.find(events[j]);
        if ((it1 - it2) > 0) {
          return std::make_pair(true, std::make_pair((*it1), (*it2)));
        }
      }
    }
  }

  IID<IPid> dummy;
  return std::make_pair(false, std::make_pair(dummy, dummy));
}

bool ViewEqTraceBuilder::Sequence::isprefix(ViewEqTraceBuilder::Sequence &seq){
  for(int i = 0; i<events.size(); i++){
    if(events[i] != seq.events[i]) return false;
  }
  return true;
}

void ViewEqTraceBuilder::Sequence::assign_thread_causality(Event& event, IPid& causal_before, IPid& causal_after,
        Sequence* prefix, std::vector<unsigned>* threads_of_prefix, IID<IPid>& e1_delim, IID<IPid>& e1,IID<IPid>& e2) {
  if (event.type == Event::ACCESS_TYPE::SPAWN) {
    causal_before = event.get_pid();    // thread calling spawn  
    causal_after  = event.object.first; // thread spawned
  }
  else { // event.type == Event::ACCESS_TYPE::JOIN
    causal_before = event.object.first; // thread joined
    causal_after  = event.get_pid();    // thread calling join

    if (causal_after == e2.get_pid()) { // joins in e2's thread
      if (causal_before == e1.get_pid()) { // join of e1's thread
        if (!prefix[CAUSAL].empty()) 
          e1_delim = prefix[CAUSAL].head(); // next event in causal prefix is the delim event
        threads_of_prefix[JOINe1].push_back(e1.get_pid());
      }
      else { // join of a thread that is not e1's thread
        threads_of_prefix[JOIN].push_back(causal_before); // tid of thread that joined
      }
    }
  }
}

void ViewEqTraceBuilder::Sequence::update_thread_causality(Event& event, IPid& causal_before, IPid& causal_after,
      Sequence* prefix, std::vector<unsigned>* threads_of_prefix, IID<IPid>& e1_delim, IID<IPid>& e1,IID<IPid>& e2) {
  assign_thread_causality(event, causal_before, causal_after, prefix, threads_of_prefix, e1_delim, e1, e2);
  if (event.type == Event::ACCESS_TYPE::JOIN && causal_after == e2.get_pid())
    return; // already covered

  if (std::find(threads_of_prefix[JOIN].begin(), threads_of_prefix[JOIN].end(), causal_after) != 
    threads_of_prefix[JOIN].end()) { // spawning/joining thread spwans/joins in a thread in the list threads_of_prefix[JOIN]
    if (causal_before == e1.get_pid()) {
      if (!prefix[JOIN].empty())
        e1_delim = prefix[JOIN].head();
      else if (!prefix[CAUSAL].empty())
        e1_delim = prefix[CAUSAL].head();
    }
    // add spawning/joining thread to the list, as its events are causally prefixed due to spawn/join
    threads_of_prefix[JOIN].push_back(causal_before); 
  }
  else if (std::find(threads_of_prefix[JOINe1].begin(), threads_of_prefix[JOINe1].end(), causal_after) !=
    threads_of_prefix[JOINe1].end()) { // spawning/joining thread spawns/joins in a thread in the list
    // add spawning/joining thread to the list, as its events are causally prefixed due to spawn/join
    threads_of_prefix[JOINe1].push_back(causal_before);
  }
  else if (std::find(threads_of_prefix[CAUSAL].begin(), threads_of_prefix[CAUSAL].end(), causal_after) !=
    threads_of_prefix[CAUSAL].end()) {
    if (causal_before == e1.get_pid()) {
      if (!prefix[CAUSAL].empty())
        e1_delim = prefix[CAUSAL].head();
    }
    threads_of_prefix[CAUSAL].push_back(causal_before);
  }
}

void ViewEqTraceBuilder::Sequence::add_causal_event(Sequence* prefix, std::vector<unsigned>* threads_of_prefix,
            std::unordered_map<unsigned, std::unordered_set<unsigned>>* objects_for_source, bool& added_to_prefix, 
            IID<IPid>& e1, IID<IPid>& e1_delim, Event event, int idx) {
  if (std::find(threads_of_prefix[idx].begin(), threads_of_prefix[idx].end(), event.get_pid()) != 
    threads_of_prefix[idx].end()) {
    prefix[idx].push_front(event.get_iid());
    added_to_prefix = true;

    if (event.is_read()) {
      objects_for_source[idx][event.object.first].insert(event.object.second);
    }
    else if (event.is_write()) {
      objects_for_source[idx][event.object.first].erase(event.object.second);    
    }

    if (event.get_pid() == e1.get_pid())
      e1_delim = event.get_iid();
  }
}

void ViewEqTraceBuilder::Sequence::prefix_rf_source(Sequence* prefix, std::vector<unsigned>* threads_of_prefix,
          std::unordered_map<unsigned, std::unordered_set<unsigned>>* objects_for_source, Event& event, 
          bool& added_to_prefix, int idx, EventSequence& source) {
  if (event.is_rmw && event.rmw_operation == RMWOperation::CMPXCHG) {
    Event corresponding_read;
    if (source.empty()) corresponding_read = container->get_event(IID<IPid>(event.get_pid(), event.get_index()-1));
    else corresponding_read = *source.find_iid(IID<IPid>(event.get_pid(), event.get_index()-1));
    if (corresponding_read.value != corresponding_read.expected_value) {
      return; // value was not modified by CMPXCHG
    }
  }

  if (objects_for_source[idx][event.object.first].find(event.object.second) != 
      objects_for_source[idx][event.object.first].end()) {
    objects_for_source[idx][event.object.first].erase(event.object.second);
    threads_of_prefix[idx].push_back(event.get_pid());
    prefix[idx].push_front(event.get_iid());
    added_to_prefix = true;
  }
}

// return: (delim event ie event beyond which e1 cannot be placed, causal prefix sequence (not including e1 or e2))
// delim event could be either event from e1's thread or join of e1's thread
// prefix of e2 upto e1 that includes events of e2 and events that are prefixed due to rf or join
std::pair<IID<IPid>, ViewEqTraceBuilder::Sequence>
ViewEqTraceBuilder::Sequence::causal_prefix(IID<IPid> e1, IID<IPid> e2, sequence_iterator begin, sequence_iterator end, EventSequence& source) {
  // id0: causal prefix of events whose threads join in thread of e2 (does not include thread of e1)
  // id1: causal prefix of events of thread of e1 given e1 joins in thread of e2
  // id2: causal prefix of events of e2 (not including causal dependence due to join events)
  Sequence prefix[] = { Sequence(container), Sequence(container), Sequence(container) };

  // id0: list of threads that join in e2's thread and threads causally prefixed to them
  // id1: list of threads causally related to e1's thread events, given e1 joins in e2's thread
  // id2: list of threads that have events causally prefixed to e2 (no due to join)
  std::vector<unsigned> threads_of_prefix[3];  
  
  // objects whose read has been included in the prefix and the corresponding source write is to be included if found
  std::unordered_map<unsigned, std::unordered_set<unsigned>> objects_for_source[3]; 

  IID<IPid> e1_delim = e2; // default delim event 
  threads_of_prefix[CAUSAL].push_back(e2.get_pid()); // prefix contains events from e2's thread

  for (auto it = end; it != begin;) { it--;
    Event ite;
    if (source.empty()) ite = container->get_event(*it);
    else ite = *source.find_iid(*it);

    if (ite.type == Event::ACCESS_TYPE::SPAWN || ite.type == Event::ACCESS_TYPE::JOIN) {
      IPid causal_before, causal_after;
      update_thread_causality(ite, causal_before, causal_after, prefix, threads_of_prefix, e1_delim, e1, e2);
      continue; // nothing else to be done for a spwan/join event (it is not added to prefix)
    }

    if (!ite.is_global()) continue; // causal prefix contains only reads and writes
    
    bool added_to_prefix = false;

    // add event to prefix if its thread events are to be prefixed
    add_causal_event(prefix, threads_of_prefix, objects_for_source, added_to_prefix, e1, e1_delim, ite, JOIN);
    add_causal_event(prefix, threads_of_prefix, objects_for_source, added_to_prefix, e1, e1_delim, ite, JOINe1);
    add_causal_event(prefix, threads_of_prefix, objects_for_source, added_to_prefix, e1, e1_delim, ite, CAUSAL);

    // write whose object's read was seen and thus rf write is to be included in prefix
    if (!added_to_prefix && ite.is_write()) { 
      prefix_rf_source(prefix, threads_of_prefix, objects_for_source, ite, added_to_prefix, JOIN, source);
      prefix_rf_source(prefix, threads_of_prefix, objects_for_source, ite, added_to_prefix, JOINe1, source);
      prefix_rf_source(prefix, threads_of_prefix, objects_for_source, ite, added_to_prefix, CAUSAL, source);
      
      if (added_to_prefix && it->get_pid() == e1.get_pid()) {
        e1_delim = (*it);
      }
    }
  }

  Event e1_event;
  if (source.empty()) e1_event = container->get_event(e1);
  else e1_event = *source.find_iid(e1);
  if (e1_event.is_write()) {
    // check if a read in any of the sequences is dependent on write e1
    for (int idx = 0; idx < 3; idx++) {
      if (objects_for_source[idx][e1_event.object.first].find(e1_event.object.second) != 
          objects_for_source[idx][e1_event.object.first].end()) { 
        prefix[idx].push_front(e1);
        e1_delim = e1;
      }
    }
  }

  Sequence result = prefix[JOIN].consistent_merge(prefix[JOINe1]);
  result = result.consistent_merge(prefix[CAUSAL]);

  return std::make_pair(e1_delim, result);
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::backseq(IID<IPid> e1, IID<IPid> e2) {
  EventSequence empty_sequence(container);
  return backseq(e1, e2, empty_sequence);
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::backseq(IID<IPid> e1, IID<IPid> e2, EventSequence& source){
  assert(this->has(e1) && this->has(e2));

  std::pair<IID<IPid>, Sequence> causal_prefix_result = causal_prefix(e1, e2, find(e1)+1, find(e2), source);
  
  IID<IPid> e1_delim = causal_prefix_result.first;  
  Sequence causal_prefix  = causal_prefix_result.second; // prefix including causal events by join and rf
  causal_prefix.push_back(e2);
  
  // if !included_e1 then include at appropriate location ie after any write/read 
  // of same object but diff value but before join
  Event event;
  if (source.empty()) event = container->get_event(e1);
  else event = *source.find_iid(e1);
  if(event.is_write()) {
    sequence_iterator loc = causal_prefix.begin(); // assuming the write can be placed at the very begining
    sequence_iterator delim_loc = causal_prefix.end(); // assuming e1 has no causal after event in the sequence
    
    bool found_racing_write = false;
    for (auto it = causal_prefix.begin(); it != causal_prefix.end()-1; it++) {
      if ((*it) == e1_delim) delim_loc = it;

      Event eit;
      if (source.empty()) eit = container->get_event(*it);
      else event = *source.find_iid(*it);
      if (event.same_object(eit)) {
        if (eit.is_write() && eit.value != event.value) { // found write of diff value
          found_racing_write = true;
          loc = it+1; // location to put write e1
        }
        else if (eit.is_read() && found_racing_write) {
          loc = it+1;
        }
      } 
    }

    if (delim_loc < loc) { 
      // cannot form sequence for the expected read value
      // because there is a write of another value but an event of same thread before the write
      // so e1 cannot be placed after the write
      causal_prefix.clear();
      return causal_prefix;
    }

    if (e1_delim != e1) // if not e1 already added
      causal_prefix.push_at(loc, e1);
    
    if (event.is_rmw) { // also add rmw read
      int mem_val;
      bool has_racing_write = false;
      sequence_iterator rmw_write_loc = causal_prefix.find(e1);

      for (auto it = causal_prefix.begin(); it != rmw_write_loc; it++) {
        Event eit;
        if (source.empty()) eit = container->get_event(*it);
        else event = *source.find_iid(*it);
        if (event.same_object(eit) && eit.is_write()) {
          has_racing_write = true;
          mem_val = eit.value;
        }
      }

      Event rmw_read = container->threads[e1.get_pid()][e1.get_index()-1];
      if (has_racing_write) {
        if (mem_val != rmw_read.value) {
          causal_prefix.clear();
          return causal_prefix;
        }
      }

      causal_prefix.push_at(rmw_write_loc, rmw_read.iid);
    }
  }
  else { // event.is_read()
    if (e1_delim != e2) { // events causal after e1 are in 'causal_prefix' then e1 cannot be added later for reading, infeasible (read, value) pair
      causal_prefix.clear();
      return causal_prefix;
    }

    causal_prefix.push_back(e1);
  }

  return causal_prefix;
}

void ViewEqTraceBuilder::Sequence::join_prefix(
                  std::vector<IID<IPid>>::iterator primary_begin, std::vector<IID<IPid>>::iterator primary_end,
                  std::vector<IID<IPid>>::iterator other_begin,   std::vector<IID<IPid>>::iterator other_end) {

  Sequence &merged_sequence = *this;

  if (primary_begin == primary_end) {
    for (auto it = other_begin; it != other_end; it++)
      merged_sequence.push_back(*it);
    return;
  }

  if (other_begin == other_end) {
    for (auto it = primary_begin; it != primary_end; it++)
      merged_sequence.push_back(*it);
    return;
  }

  auto primary_next = primary_begin;
  for (auto ito = other_begin; ito != other_end; ito++) {
    Event eo = container->get_event(*ito);

    for (auto itp = primary_next; itp != primary_end; itp++) {
      Event ep = container->get_event(*itp);

      if ((eo.is_write() && eo.RWpair(ep)) || // ep is a read of same objectas eo
        (container->ordered_by_join(*itp, *ito) || container->ordered_by_spawn(*itp, *ito))) { // ep is ordered before eo by join/spawn
        // ep must be added before eo
        for (auto ito_ = ito; ito_ != other_end; ito_++) { // events after eo in other
          for (auto itp_ = primary_next; itp_ != itp+1; itp_++) { // events before ep in primary
            if (container->ordered_by_join(*ito_, *itp_) || container->ordered_by_spawn(*ito_, *itp_)) {
              // ito_ is ordered before itp_ by join/spawn
              // cyclic dependency - no feasible merge possible -- ABORT
              merged_sequence.clear();
              return;
            }
          }
        }

        // no dependency of ep (or events before ep) on eo (or events after eo)
        // push events in primary till ep
        for (auto itp2 = primary_next; itp2 != itp; itp2++) {
          Event ep2 = container->get_event(*itp2);

          if (ep2.is_write()) { // next event of other is a write
            // look for reads of same object in other after the write eo, 
            // if such reads exist then they must be pushed before ep2 but 
            // that would crearte a cyclic dependency so ABORT
            for (auto ito2 = ito+1; ito2 != other_end; ito2++) {
              Event eo2 = container->get_event(*ito2);

              if (ep2.RWpair(eo2)) { // eo2 is a read of same object as ep2
                // no feasible merge possible ABORT
                merged_sequence.clear();
                return;
              }
            }
          }

          merged_sequence.push_back(*itp2);
        }

        merged_sequence.push_back(*itp);
        // ep is a read (because eo is a write and (ep,eo) is a RWpair)
        if (ep.is_rmw && (itp+1) != primary_end) {
          merged_sequence.push_back(*(itp+1));
          itp++; // include its write as well (is it exists)
        }
        primary_next = itp+1;
      }
    }

    if (eo.is_write() && eo.is_rmw) { // put rmw read along with its write
      auto ito_read_in_merged = merged_sequence.find(*(ito-1));
      // if rmw read already in the end, do nothing
      if (ito_read_in_merged != merged_sequence.end()-1) {
        // check if bringing rmw read to end will change its value
        Event rit = container->get_event(*ito_read_in_merged);

        bool has_racing_write = false;
        int mem_val;
        for (auto it = ito_read_in_merged+1; it != merged_sequence.end(); it++) {
          Event eit = container->get_event(*it);
          if (rit.RWpair(eit)) {
            has_racing_write = true;
            mem_val = eit.value;
          }
        }

        if (has_racing_write && mem_val != rit.value) {
          // bringing rmw read to end will change its value
          merged_sequence.clear();
          return;
        }
        else {
          // okay to bring rmw read to the end
          merged_sequence.erase(ito_read_in_merged);
          merged_sequence.push_back(rit.get_iid());
        }
      }
    }

    merged_sequence.push_back(*ito);
  }

  // push remaining events of primary
  for (auto it = primary_next; it != primary_end; it++) {
    merged_sequence.push_back(*it);
  }
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::join(Sequence& other_seq) {
  Sequence& primary_seq = *this;
  Sequence merged_sequence(container);

  std::vector<IID<IPid>>::iterator next_index_primary = primary_seq.begin();
  std::vector<IID<IPid>>::iterator next_index_other   = other_seq.begin();

  // list of iterators to common events in both sequences [type: vector(seq_iterator, seq_iterator)]
  // std::vector<std::pair<std::vector<IID<IPid>>::iterator, std::vector<IID<IPid>>::iterator>> common_events;
  for (auto it = primary_seq.begin(); it != primary_seq.end(); it++) {
    auto it_in_other = other_seq.find(*it);
    if (it_in_other == other_seq.end()) // other_seq does not have *it
      continue;

    if (it != next_index_primary || it_in_other != next_index_other) {
      // if the common event is the next event in both sequences then just push it
      // otherwise join prefixes before pushing it
      merged_sequence.join_prefix(next_index_primary, it, next_index_other, it_in_other);
      if (merged_sequence.empty()) // prefixes could not be joined
        return merged_sequence;
    }
    merged_sequence.push_back(*it);
    
    next_index_primary = it+1;
    next_index_other   = it_in_other+1;
  }

  merged_sequence.join_prefix(next_index_primary, primary_seq.end(), next_index_other, other_seq.end());
  return merged_sequence;
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::consistent_merge(Sequence &other_seq) {
  Sequence &primary_seq = *this;

  if (primary_seq.empty()) return other_seq;
  if (other_seq.empty()) return primary_seq;

  Sequence original_primary_seq = primary_seq;
  Sequence original_other_seq   = other_seq;

  // look for a conflicting RW pair (conflict: RW pair (e1,e2) st e1 < e2 in primary and e2 < e1 in other)
  // pair(bool has_conglicting_pair , pair(conflicting event 1, conflicting event2))
  std::pair<bool, std::pair<IID<IPid>, IID<IPid>>> conflictingRWpair = primary_seq.conflicts_with(other_seq);
  while (conflictingRWpair.first) { // primary_seq and other_seq conflict
    if (primary_seq.view_adjust(conflictingRWpair.second.first, conflictingRWpair.second.second)) {
      // other_seq adjusted ensuring same view of reads
      // now look for next conflicting RW pair
      conflictingRWpair = primary_seq.conflicts_with(other_seq);
    } 
    else if(other_seq.view_adjust(conflictingRWpair.second.second, conflictingRWpair.second.first)) {
      // primary_seq adjusted ensuring same view of reads
      // now look for next conflicting RW pair
      conflictingRWpair = primary_seq.conflicts_with(other_seq);
    }
    else { // conflicting and cannot be adjusted
      // restrore original sequences (before any view-adjusts) and 
      // return empty sequence representing no feasible merge
      primary_seq = original_primary_seq;
      other_seq   = original_other_seq;
      
      original_primary_seq.clear(); // dummy seq for returning empty sequence
      return original_primary_seq;
    }
  }

  // llvm::outs() << primary_seq.to_string() << " (+) " << other_seq.to_string() << " = ";
  // llvm::outs() << join(primary_seq, other_seq).to_string() << "\n";
  Sequence result = primary_seq.join(other_seq);
  if (!result.co_sanity(primary_seq, other_seq)) // co: causal order = --po--> U --rf-->
    result.clear(); // co of result does not contain co of primary and other

  return result;
}

std::vector<std::pair<IID<IPid>, IID<IPid>>> ViewEqTraceBuilder::Sequence::get_causal_order() {
  std::unordered_map<unsigned, std::unordered_map<unsigned, IID<IPid>>> last_write; // memory object -> last write event
  std::unordered_map<int, IID<IPid>> previous_thread_event; // thread-id -> last event

  std::vector<std::pair<IID<IPid>, IID<IPid>>> co;

  for (auto it = begin(); it != end(); it++) {
    if (previous_thread_event.find(it->get_pid()) != previous_thread_event.end()) {
      co.push_back(std::make_pair(previous_thread_event[it->get_pid()], (*it)));
    }
    previous_thread_event[it->get_pid()] = (*it);

    Event e = container->get_event(*it);
    if (e.is_read()) {
      if (last_write[e.object.first].find(e.object.second) != last_write[e.object.first].end()) {
        co.push_back(std::make_pair(last_write[e.object.first][e.object.second], (*it)));
      }
    }

    if (e.is_write()) {
      last_write[e.object.first][e.object.second] = (*it);
    }
  }

  return co;
}

bool ViewEqTraceBuilder::Sequence::co_sanity(Sequence& seq1, Sequence& seq2) {
  if (empty()) 
    return false; // if this sequence is empty then co of seq1 and seq2 are trivially not contained in it

  std::vector<std::pair<IID<IPid>, IID<IPid>>> co = get_causal_order();

  std::vector<std::pair<IID<IPid>, IID<IPid>>> co1 = seq1.get_causal_order();
  std::vector<std::pair<IID<IPid>, IID<IPid>>> co2 = seq2.get_causal_order();

  for (auto it = co1.begin(); it != co1.end(); it++) {
    if (std::find(co.begin(), co.end(), (*it)) == co.end())
      return false; // co1 not contained in co
  }

  for (auto it = co2.begin(); it != co2.end(); it++) {
    if (std::find(co.begin(), co.end(), (*it)) == co.end())
      return false; // co2 not contained in co
  }

  for (auto it = co.begin(); it != co.end(); it++) {
    if (std::find(co1.begin(), co1.end(), (*it)) == co1.end())
      if (std::find(co2.begin(), co2.end(), (*it)) == co2.end())
        return false; // co not contained in co1 U co2
  }

  return true;
}

std::string ViewEqTraceBuilder::EventSequence::to_string() {
  if (events.empty()) return "<>";

  std::string s = "<";
  s += "(" + std::to_string(events[0].get_pid()) + ":" + std::to_string(events[0].get_index()) + ")";
  if (events[0].is_write()) 
    s += "=" + std::to_string(events[0].value);
  else s += "[" + std::to_string(events[0].value) + "]";

  for (auto it = events.begin() + 1; it != events.end(); it++) {
    s = s + ", (" + std::to_string(it->get_pid()) + ":" + std::to_string(it->get_index()) +")";
    if (it->is_write()) 
      s += "=" + std::to_string(it->value);
    else s += "[" + std::to_string(it->value) + "]";
  }

  s = s + ">";
  return s;
}

ViewEqTraceBuilder::EventSequence ViewEqTraceBuilder::Sequence::to_event_sequence(
                                        std::unordered_map<std::pair<unsigned, unsigned>, int, HashFn>& mem) {
  EventSequence event_sequence(container);
  std::unordered_map<std::pair<unsigned, unsigned>, int, HashFn> mem_map;

  for (auto it = begin(); it != end(); it++) {
    Event e = container->get_event(*it);

    if (e.is_write()) {
      if (e.is_rmw) {
        int rmw_read_value = event_sequence.last().value;
        e.value = container->compute_modified_value(e.rmw_operation, rmw_read_value, e.rmw_value, e.expected_value);
      }
      mem_map[e.object] = e.value;
    }

    else if (e.is_read()) {
      if (mem_map.find(e.object) != mem_map.end()) {
        e.value = mem_map[e.object];
      }
      else if (mem.find(e.object) != mem.end()) {
        e.value = mem[e.object];
      }
      else {
        e.value = 0; // every variable is expected to be initialized, algo must not reach here if that is followed
      }
    }

    event_sequence.push_back(e);
  }

  return event_sequence;
}

ViewEqTraceBuilder::EventSequence ViewEqTraceBuilder::Sequence::to_event_sequence(EventSequence& source,
                                        std::unordered_map<std::pair<unsigned, unsigned>, int, HashFn>& mem) {
  EventSequence event_sequence(container);
  std::unordered_map<std::pair<unsigned, unsigned>, int, HashFn> mem_map;

  for (auto it = begin(); it != end(); it++) {
    Event e;
    if (container->threads.size() > it->get_pid()) {
      if (container->threads[it->get_pid()].size() > it->get_index()) {
        e = container->get_event(*it);
      }
    }
    else {
      e = (*(source.find_iid(*it)));
    }

    if (e.is_write()) {
      if (e.is_rmw) {
        int rmw_read_value = event_sequence.last().value;
        e.value = container->compute_modified_value(e.rmw_operation, rmw_read_value, e.rmw_value, e.expected_value);
      }
      mem_map[e.object] = e.value;
    }

    else if (e.is_read()) {
      if (mem_map.find(e.object) != mem_map.end()) {
        e.value = mem_map[e.object];
      }
      else if (mem.find(e.object) != mem.end()) {
        e.value = mem[e.object];
      }
      else {
        e.value = 0; // every variable is expected to be initialized, algo must not reach here if that is followed
      }
    }

    event_sequence.push_back(e);
  }

  return event_sequence;
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::subsequence(ViewEqTraceBuilder::sequence_iterator begin, ViewEqTraceBuilder::sequence_iterator end) {
  Sequence s(container);

  for (auto it = begin; it != end; it++) {
    s.push_back(*it);
  }

  return s;
}

std::string ViewEqTraceBuilder::Sequence::to_string() {
  if (events.empty()) return "<>";

  std::string s = "<";
  s += "(" + std::to_string(events[0].get_pid()) + ":" + std::to_string(events[0].get_index()) +")";
  for (auto it = events.begin() + 1; it != events.end(); it++) {
    s = s + ", (" + std::to_string(it->get_pid()) + ":" + std::to_string(it->get_index()) +")";
  }

  s = s + ">";
  return s;
}

ViewEqTraceBuilder::event_sequence_iterator ViewEqTraceBuilder::EventSequence::find_iid(IID<IPid> iid) {
  for (auto it = begin(); it != end(); it++) {
    if (it->iid == iid) 
      return it;
  }
  return end();
}

void ViewEqTraceBuilder::EventSequence::add_rmw_write(IID<IPid> rmw_read, IID<IPid> rmw_write) {
  assert(find_iid(rmw_read) != end());
  Event rmw_write_event = container->get_event(rmw_write);

  auto it = find_iid(rmw_read);
  push_at(it + 1, rmw_write_event);

  std::unordered_map<std::pair<unsigned, unsigned>, int, HashFn> mem_map;
  for (auto it = begin(); it != end(); it++)
  { // rmw write to end
    if (it->is_write()) {
      if (it->is_rmw) {
        int rmw_read_value = (it-1)->value;
        it->value = container->compute_modified_value(it->rmw_operation, rmw_read_value, it->rmw_value, it->expected_value);
      }
      mem_map[it->object] = it->value;
    }

    else if (it->is_read())
      if (mem_map.find(it->object) != mem_map.end())
        it->value = mem_map[it->object];
  }
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::EventSequence::to_iid_sequence() {
  Sequence seq(container);
  for (auto it = begin(); it != end(); it++) {
    seq.push_back(it->iid);
  }

  return seq;
}

std::unordered_map<IID<IPid>, int> ViewEqTraceBuilder::EventSequence::read_value_map() {
  std::unordered_map<IID<IPid>, int> rv_map;
  for (auto it = begin(); it != end(); it++) {
    if (!it->is_read()) continue;

    rv_map[it->iid] = it->value;
  }

  return rv_map;
}

bool ViewEqTraceBuilder::EventSequence::VA_equal(EventSequence& seq) {
  EventSequence s = seq;

  std::pair<bool, std::pair<Event, Event>> conflictingRWpair = conflicts_with(s);
  while (conflictingRWpair.first) { 
    if(s.view_adjust(conflictingRWpair.second.second, conflictingRWpair.second.first)) {
      // seq adjusted ensuring same view of reads
      // now look for next conflicting RW pair
      conflictingRWpair = conflicts_with(s);
    }
    else { // conflicting and cannot be adjusted
      return false;
    }
  }

  // no more conflicts
  if (to_iid_sequence() == s.to_iid_sequence()) // equal after view_adjustment
    return true;

  return false;
}

bool ViewEqTraceBuilder::EventSequence::view_adjust(Event e1, Event e2) {
  std::vector<Event> original_events = events;

  event_sequence_iterator it1 = find_iid(e1.iid);
  event_sequence_iterator it2 = find_iid(e2.iid);
  int n1 = it1 - begin();
  int n2 = it2 - begin();

  for (int i = n2 - 1, delim = 0; i >= n1; i--) { // from before e2 till e1
    if (events[i].get_pid() != e1.get_pid()) continue; // shift only events from e1's thread

    for (int j = i; j < n2 - delim; j++) {
      Event ecurr = events[j];
      Event enext = events[j+1];

      if (ecurr.RWpair(enext)) { // if cannot exchange
        events = original_events; // restrore original sequence
        return false;
      }

      // can exchange
      Event tmp = events[j];
      events[j] = events[j+1];
      events[j+1] = tmp;
    }
    
    delim++; // shifted 1 event of e1's thread, now 1 location fixed
  }

  return true;
}

std::pair<bool, std::pair<ViewEqTraceBuilder::Event, ViewEqTraceBuilder::Event>> 
ViewEqTraceBuilder::EventSequence::conflicts_with(EventSequence& other_seq) {
  for(int i = 0; i < events.size(); i++){
    for(int j = i + 1; j < events.size() ; j++){
      event_sequence_iterator it1 = other_seq.find_iid(events[i].iid);
      event_sequence_iterator it2 = other_seq.find_iid(events[j].iid);
      if (it1 != other_seq.end() && it2 != other_seq.end()) {
        if ((it1 - it2) > 0) {
          return std::make_pair(true, std::make_pair((*it1), (*it2)));
        }
      }
    }
  }

  IID<IPid> dummy;
  return std::make_pair(false, std::make_pair(dummy, dummy));
}

bool ViewEqTraceBuilder::EventSequence::operator==(EventSequence seq) {
  for (auto e1 = begin(), e2 = seq.begin(); e1 != end() && e2 != seq.end(); e1++, e2++) {
    if (e1->iid != e2->iid)
      return false;
  }

  return true;
}

bool ViewEqTraceBuilder::EventSequence::operator!=(EventSequence seq) {
  return !((*this) == seq);
}

std::unordered_set<IID<IPid>> ViewEqTraceBuilder::unexploredInfluencers(Event er, SOPFormula& f){
  assert(er.is_read());
  std::unordered_set<IID<IPid>> ui;
  std::unordered_set<int> values,forbidden_values;

  for (int i = 0; i < Enabled.size(); i++){
    Event e = get_event(Enabled[i]);
    
    // not a write event or not same object or already taken value or already checked forbidden value
    if(!e.is_write() || !er.same_object(e) || values.find(e.value) != values.end() || forbidden_values.find(e.value) != forbidden_values.end()) continue;

    // check if value of e is forbidden for er
    if (f.check_evaluation(std::make_pair(er.iid, e.value))== RESULT::TRUE) {
      forbidden_values.insert(e.value);
      continue;
    }
    
    ui.insert(e.iid);
    values.insert(e.value);
  }

  return ui;
}

std::unordered_set<IID<IPid>> ViewEqTraceBuilder::exploredInfluencers(Event er, SOPFormula &f){
  std::unordered_set<IID<IPid>> ei;
  std::unordered_set<int> forbidden_values;

  for (int i = execution_sequence.size()-1; i >= 0; i--) {
    Event ievent = get_event(execution_sequence[i]);
    if (er.RWpair(ievent)) {
      if (forbidden_values.find(ievent.value) != forbidden_values.end()) 
        continue;

      if (f.check_evaluation(std::make_pair(er.iid, ievent.value)) == RESULT::TRUE ) {
        forbidden_values.insert(ievent.value);
        continue;
      }

      ei.insert(ievent.iid); // insert is not forbidden
    }
  }
  
  return ei;
}

std::unordered_set<IID<IPid>> ViewEqTraceBuilder::exploredWitnesses(Event ew, SOPFormula &f){
  assert(ew.is_write());
  
  std::unordered_set<IID<IPid>> explored_witnesses;
  for(int i = 0; i < execution_sequence.size(); i++){
    IID<IPid> er = execution_sequence[i];
    
    Event e = get_event(er);
    if(! e.is_read() || ! e.same_object(ew) || e.get_pid() == ew.get_pid() ) continue;

    SOPFormula forbidden_at_read; 
    if (prefix_state[i] >= 0) {
      forbidden_at_read = states[prefix_state[i]].leads[states[prefix_state[i]].alpha].forbidden;
    }

    if(forbidden_at_read.check_evaluation(std::make_pair(er, ew.value)) != RESULT::TRUE) {
      explored_witnesses.insert(er);
    }
  }
  
  return explored_witnesses;
}

bool ViewEqTraceBuilder::Lead::VA_equivalent(Lead& l) {
  if (merged_sequence.to_iid_sequence() == l.merged_sequence.to_iid_sequence())
    return true; // sequences are equal => sequences are equivalent

  if (merged_sequence.VA_equal(l.merged_sequence)) // equal after view adjust ie resolving conflicts
    return true;
  
  std::unordered_map<IID<IPid>, int> rv_map = merged_sequence.read_value_map();
  std::unordered_map<IID<IPid>, int> l_rv_map = l.merged_sequence.read_value_map();

  if (rv_map.size() != l_rv_map.size()) return false; // number of reads is unequal

  for (auto it = rv_map.begin(); it != rv_map.end(); it++) {
    if (l_rv_map.find(it->first) == l_rv_map.end())
      return false; // current read in this->merged_sequence does not exist in l
    if (it->second != l_rv_map[it->first])
      return false; // current read in this->merged_sequence exists in l but values in the 2 sequences don't match
  }

  return true;
}

void ViewEqTraceBuilder::Lead::add_rmw_write(IID<IPid> rmw_read, IID<IPid> rmw_write) {
  if (merged_sequence.find_iid(rmw_write) != merged_sequence.end())
    return; // already added
  if (merged_sequence.find_iid(rmw_read) == merged_sequence.end())
    return; // lead of some other read

  start.add_rmw_write(rmw_read, rmw_write);
  if (!constraint.empty())
    constraint.add_rmw_write(rmw_read, rmw_write);
  merged_sequence.add_rmw_write(rmw_read, rmw_write);
}

void ViewEqTraceBuilder::Lead::add_rmw_write() {
  if (!pending_addition_of_rmw_write) return;

  IID<IPid> rmw_read(pending_rmw_write.get_pid(), pending_rmw_write.get_index()-1);
  merged_sequence.add_rmw_write(rmw_read, pending_rmw_write);
}

void ViewEqTraceBuilder::Lead::update_pending_rmw_write(IID<IPid> rmw_write) {
  IID<IPid> rmw_read(rmw_write.get_pid(), rmw_write.get_index()-1);

  if (merged_sequence.find_iid(rmw_write) != merged_sequence.end())
    return; // already added
  if (merged_sequence.find_iid(rmw_read) == merged_sequence.end())
    return; // lead of some other read

  pending_addition_of_rmw_write = true;
  pending_rmw_write = rmw_write;

  merged_sequence.add_rmw_write(rmw_read, rmw_write);
}

bool ViewEqTraceBuilder::Lead::VA_isprefix(Lead& l) {
  Sequence this_iid_seq = merged_sequence.to_iid_sequence();
  Sequence l_iid_seq = l.merged_sequence.to_iid_sequence();
  if (this_iid_seq.isprefix(l_iid_seq))
    return true; // lead sequence is a prefix wihtout needing view-adjustment

  EventSequence suffix = l.merged_sequence;
  std::unordered_map<std::pair<unsigned, unsigned>, int, HashFn> post_prefix_memory_map;

  for (auto it = merged_sequence.begin(); it != merged_sequence.end(); it++) {
    auto its = suffix.find_iid(it->iid);
    if (its == suffix.end()) 
      return false; // if event of current lead not in l then current lead is not a prefix of l

    if (it->is_read() && its->value != it->value) 
      return false; // if event of current lead exists in l but values don't match

    if (it->is_write()) {
      post_prefix_memory_map[it->object] = it->value;
    }

    // remove event *it from its current index to create suffix
    suffix.erase(its->iid);
  }

  for (auto it = suffix.begin(); it != suffix.end(); it++) {
    if (it->is_read()) {
      if (post_prefix_memory_map.find(it->object) != post_prefix_memory_map.end()) {
        if (post_prefix_memory_map[it->object] != it->value) {
          // values don't match in original and modified sequences
          return false;
        }
      }
    }
    else { // it->is_write()
      post_prefix_memory_map[it->object] = it->value;
    }
  }

  return true;
}

bool ViewEqTraceBuilder::Lead::VA_isweakly_prefix(Lead& l) {
  Sequence this_iid_seq = merged_sequence.to_iid_sequence();
  Sequence l_iid_seq = l.merged_sequence.to_iid_sequence();
  EventSequence suffix = l.merged_sequence;

  std::unordered_map<std::pair<unsigned, unsigned>, int, HashFn> memory_map;

  for (auto it = merged_sequence.begin(); it != merged_sequence.end(); it++) {
    auto its = suffix.find_iid(it->iid);
    if (its != suffix.end()) {
      if (it->is_read() && its->value != it->value)
          return false; // if event of current read exists in l but values don't match

      // remove event *it from its current index to create suffix
      suffix.erase(its->iid);
    }

    if (it->is_write())
      memory_map[it->object] = it->value;
  }

  for (auto it = suffix.begin(); it != suffix.end(); it++) {
    if (it->is_read()) {
      if (memory_map.find(it->object) != memory_map.end()) {
        if (memory_map[it->object] != it->value) {
          // values don't match in original and modified sequences
          return false;
        }
      }
    }
    else { // it->is_write()
      memory_map[it->object] = it->value;
    }
  }

  return true;
}

bool ViewEqTraceBuilder::Lead::VA_isweakly_equivalent(Lead& l) {
  if (!VA_isweakly_prefix(l)) return false;
  if (!l.VA_isweakly_prefix(*this)) return false;

  return true;
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Lead::VA_suffix(Lead& prefix) {
  assert(prefix.VA_isprefix(*this)); // 'prefix' is a VA prefix of *this (suffix is only called after this check)
  EventSequence suffix = merged_sequence;

  /* since wkt 'prefix' is a prefix of *this, thus we only need to remove
     the events of 'prefix' to get the suffix
  */
  for (auto it = prefix.merged_sequence.begin(); it != prefix.merged_sequence.end(); it++) {
    suffix.erase(it->iid);
  }

  if (pending_addition_of_rmw_write && suffix.has(pending_rmw_write)) 
    suffix.erase(pending_rmw_write);

  return suffix.to_iid_sequence();
}

ViewEqTraceBuilder::Lead ViewEqTraceBuilder::Lead::VA_weak_prefix(Lead& l) {
  EventSequence prefix = merged_sequence;

  for (auto it = merged_sequence.begin(); it != merged_sequence.end(); it++) {
    if (l.merged_sequence.find_iid(it->iid) == l.merged_sequence.end()) {
      prefix.erase(it->iid);
    }
  }

  SOPFormula f = forbidden;
  f && l.forbidden;

  Lead l_;
  l_.start = prefix.to_iid_sequence();
  l_.merged_sequence = prefix;
  l_.forbidden = f;
  l_.container = container;

  return l;
}

bool ViewEqTraceBuilder::Lead::operator==(Lead l) {
  if (constraint != l.constraint)
    return false;

  if (start != l.start)
    return false;

  return true;
}

std::string ViewEqTraceBuilder::Lead::to_string() {
  return ("(" + constraint.to_string() + ", " + start.to_string() + ", " + forbidden.to_string() + ") = " + merged_sequence.to_string());
}

bool ViewEqTraceBuilder::State::has_unexplored_leads() {
  for (auto itl = leads.begin(); itl != leads.end(); itl++) {
    if (!itl->is_done) return true;
  }

  return false;
}

std::vector<ViewEqTraceBuilder::Lead> ViewEqTraceBuilder::State::unexplored_leads() {
  std::vector<Lead> ul;

  for (auto itl = leads.begin(); itl != leads.end(); itl++) {
    if (!itl->is_done) ul.push_back((*itl));
  }

  return ul;
}

ViewEqTraceBuilder::Lead ViewEqTraceBuilder::State::next_unexplored_lead() {
  assert(has_unexplored_leads());

  for (int i = 0; i < leads.size(); i++) {
    if (!leads[i].is_done) {
      leads[i].is_done = true;
      alpha = i;

      if (performed_fwd_analysis) {
        if (leads[i].merged_sequence.find_iid(fwd_read) == leads[i].merged_sequence.end()) // all fwd leads are done
          performed_fwd_analysis = false;
      }

      return leads[i];
    }
  }

  assert(false);
  Lead dummy;
  return dummy;
}

std::string ViewEqTraceBuilder::State::print_leads() {
  std::string s = "State[" + std::to_string(sequence_prefix) + "] leads:\n";
  for (auto l : leads) {
    s = s + l.to_string() + ",\n";
  }

  return s;
}

bool ViewEqTraceBuilder::forward_lead(std::unordered_map<int, std::vector<Lead>>& forward_state_leads, int state, Lead lead) {
  if (states[state].alpha_empty()) return false;

  if (states[state].leads[states[state].alpha].VA_isprefix(lead)) { // alpha sequence is prefix
    int fwd_state = prefix_state[execution_sequence.index_of(states[state].alpha_sequence().last().iid)] + 1;
    assert(fwd_state >= 0 && fwd_state < states.size());
    
    SOPFormula f = (lead.forbidden);
    EventSequence fwd_const(this);
    if (fwd_state != current_state) { 
      f || states[fwd_state].forbidden; // combine forbidden with forbidden of the state after current start
      fwd_const = states[fwd_state].alpha_sequence();
      
      Lead fwd_lead(fwd_const, lead.VA_suffix(states[state].leads[states[state].alpha]), f, states[fwd_state].mem);
      if (lead.pending_addition_of_rmw_write) {
        fwd_lead.update_pending_rmw_write(lead.pending_rmw_write);
      }
      forward_state_leads[fwd_state].push_back(std::move(fwd_lead)); 
    }

    return true;
  }

  return false;
}

void ViewEqTraceBuilder::forward_suffix_leads(std::unordered_map<int, std::vector<Lead>>& forward_state_leads, int state, std::vector<Lead>& L) {
  for (auto it = L.begin(); it != L.end();) {
    if (forward_lead(forward_state_leads, state, (*it)))
      it = L.erase(it);
    else
      it++;
  }
}

void ViewEqTraceBuilder::remove_duplicate_leads(std::vector<Lead>& L) {
  for (auto it = L.begin(); it != L.end(); it++) {
    for (auto it2 = it+1; it2 != L.end();) {
      if (it->merged_sequence == it2->merged_sequence) { // found two instances of the same lead
        it2 = L.erase(it2); // remove one instance
        continue;
      }

      if (it->VA_equivalent(*it2)) {
        it2 = L.erase(it2); // remove one instance
        continue;
      }

      it2++; // lead it2 is not a duplicate of lead it, move to next
    }
  }
}

void ViewEqTraceBuilder::consistent_union(int state, Lead& l) {
  std::vector<Lead> L;
  L.push_back(l);
  consistent_union(state, L);
}

void ViewEqTraceBuilder::consistent_union(int state, std::vector<Lead>& L) {
  std::unordered_map<int, std::vector<Lead>> forward_state_leads;
  std::vector<Lead> L_;

  remove_duplicate_leads(L); // remove any duplicate leads

  if(states[state].leads.empty()) {
    forward_suffix_leads(forward_state_leads, state, L);
    for (auto itfl = forward_state_leads.begin(); itfl != forward_state_leads.end(); itfl++) {
      consistent_union(itfl->first, itfl->second);
    }

    states[state].leads = L;
    return;
  }

  // ////
  // out << "state" << state << " leads:\n";
  // for (auto sl = states[state].leads.begin(); sl != states[state].leads.end(); sl++) {
  //   out << sl->to_string() << " = " << sl->merged_sequence.to_string() << "\n";
  // }
  // out << "new leads:\n";
  // for (auto l = L.begin(); l != L.end(); l++) {
  //   out << l->to_string() << " = " << l->merged_sequence.to_string() << "\n";
  // }
  // ////

  std::set<int, std::greater<int>> rem; // list of indices to be removed from current leads in descending order
  std::vector<Lead> add; // list of indices of L to be added to state leads

  for (auto l = L.begin(); l != L.end(); ) {
    if (l->merged_sequence.empty()) { // no coherent merge of start and constraint
      l = L.erase(l);
      continue;
    }

    bool combined_with_existing = false;
    for (auto sl = states[state].leads.begin(); sl != states[state].leads.end(); sl++) {
      if ((*l) == (*sl)) {
        // out << "same " << sl->to_string() << " and " << l->to_string() << "\n";
        rem.insert(sl - states[state].leads.begin());
        SOPFormula f = l->forbidden;
        f || sl->forbidden;
        add.push_back(Lead((*sl), f));
        combined_with_existing = true;
        break;
      }

      if (sl->VA_equivalent(*l)) { // new lead has the same view as an existing lead
        // out << "equivalent " << sl->to_string() << " and " << l->to_string() << "\n";
        if (!sl->is_done) {
          rem.insert(sl - states[state].leads.begin());
          SOPFormula f = l->forbidden;
          f || sl->forbidden;
          add.push_back(Lead((*sl), f));
        }

        combined_with_existing = true;
        break;
      }

      if (sl->merged_sequence != states[state].alpha_sequence() && sl->VA_isprefix(*l)) {
        // out << "prefix " << sl->to_string() << " of " << l->to_string() << "\n";
        if (!sl->is_done) {
          rem.insert(sl - states[state].leads.begin());
          SOPFormula f = l->forbidden;
          f && sl->forbidden;
          add.push_back(Lead((*sl), f));
        }

        combined_with_existing = true;
        break;
      }

      if (sl->merged_sequence != states[state].alpha_sequence() && sl->VA_isweakly_equivalent(*l)) {
        prefix_analysis((*sl), (*l), L_, state);
        for (auto sl_ = sl+1; sl_ != states[state].leads.end(); sl_++) {
          if (sl_->merged_sequence != states[state].alpha_sequence() && sl_->VA_isweakly_equivalent(*l))
            prefix_analysis((*sl_), (*l), L_, state);
        }

        combined_with_existing = true;
        break;
      }
    }

    if (combined_with_existing) {
      l = L.erase(l);
      continue;
    }
    
    if (forward_lead(forward_state_leads, state, (*l))) {
      l = L.erase(l);
      continue;
    }
    
    l++;
  }

  // ////
  // out << "remaining state " << state << " leads:\n";
  // for (auto it = states[state].leads.begin(); it != states[state].leads.end(); it++) {
  //   out << it->to_string() << "\n";
  // }
  // ////

  for (auto it = rem.begin(); it != rem.end(); it++) {
    // out << "removing index=" << (*it) << ": " << (states[state].leads.begin() + (*it))->to_string() << "\n";
    states[state].leads.erase(states[state].leads.begin() + (*it));
  }

  for (auto it = add.begin(); it != add.end(); it++) {
    // out << "1.adding lead: " << it->to_string() << "\n";
    states[state].leads.push_back(*it);
  }

  for (auto it = L.begin(); it != L.end(); it++) {
    // out << "2.adding lead: " << it->to_string() << "\n";
    states[state].leads.push_back(*it);
  }

  for (auto itfl = forward_state_leads.begin(); itfl != forward_state_leads.end(); itfl++) {
    // ////
    // out << "forwarded to state " << itfl->first << "\n";
    // for (auto it = itfl->second.begin(); it != itfl->second.end(); it++) {
    //   out << it->to_string() << "\n";
    // }
    // ////
    consistent_union(itfl->first, itfl->second);
  }

  if (!L_.empty()) {
    // //
    // out << "added leads \n";
    // for (auto it = L_.begin(); it != L_.end(); it++) {
    //   out << it->to_string() << "\n";
    // }
    // //
    consistent_union(state, L_);
  }
}

long long StringHash::polynomialRollingHash() {
  int p = 31;
  int m = 1e9 + 9;
  long long power_of_p = 1;
  long long hash_val = 0;

  // Loop to calculate the hash value by iterating over the elements of string
  for (int i = 0; i < str.length(); i++) {
    hash_val = (hash_val + (str[i] - 'a' + 1) * power_of_p) % m;
    power_of_p = (power_of_p * p) % m;
  }

  return (hash_val%m + m) % m; // return positive remainder only
}