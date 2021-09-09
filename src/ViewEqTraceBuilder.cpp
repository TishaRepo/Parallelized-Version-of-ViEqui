#include "ViewEqTraceBuilder.h"

typedef int IPid;

ViewEqTraceBuilder::ViewEqTraceBuilder(const Configuration &conf) : TSOPSOTraceBuilder(conf)
{
  threads.push_back(Thread(CPid(), -1));
  current_thread = -1;
  current_state = -1;
  replay_point = -1;
  prefix_idx = 0;
  execution_sequence.update_threads(&threads);
}

ViewEqTraceBuilder::~ViewEqTraceBuilder() {}

bool ViewEqTraceBuilder::schedule(int *proc, int *aux, int *alt, bool *DryRun) {
  // snj: For compatibility with existing design
  *aux = -1; *alt = 0; *DryRun = false;

  assert(execution_sequence.size() == prefix_state.size());
  if (is_replaying()) {
    // llvm::outs() << "REPLAYING: ";
    replay_schedule(proc);
    return true;
  }

  if (at_replay_point()) {
    // llvm::outs() << "AT REPLAY PT: \n";
    
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

  // [snj]: peak next read/write event / execute next non-read/write event
  if (exists_non_memory_access(proc)) {
    return true;
  }

  if (Enabled.empty()) {
    assert(forbidden.evaluate != RESULT::TRUE);
    return false; // [snj]: maximal trace explored
  }

  // [snj]: Explore Algo function
  make_new_state(); 
  compute_new_leads(); 
  
  if (states[current_state].has_unexplored_leads()) { // [snj]: TODO should be assert not check
    execute_next_lead();
    // [snj]: execute current event from Interpreter::run() in Execution.cpp
    *proc = current_thread;
    return true;
  }
  
  assert(false); //must not reach here
  return false;
}

void ViewEqTraceBuilder::replay_non_memory_access(int next_replay_thread, IID<IPid> next_replay_event) {
  assert(!threads[next_replay_thread].awaiting_load_store);
  assert(next_replay_event.get_index() == threads[next_replay_thread].size());

  threads[next_replay_thread].push_back(no_load_store);
  current_thread = next_replay_thread;
}

void ViewEqTraceBuilder::replay_memory_access(int next_replay_thread, IID<IPid> next_replay_event) {
  assert(prefix_state[prefix_idx] >= 0);
  assert(next_replay_event.get_index() == (threads[next_replay_thread].size()-1));

  auto it = std::find(Enabled.begin(), Enabled.end(), next_replay_event);
  assert(it != Enabled.end());
  Enabled.erase(it);

  current_state = prefix_state[prefix_idx];
  current_thread = next_replay_thread;
  current_event = get_event(next_replay_event);
  threads[current_thread].awaiting_load_store = false; // [snj]: next event after current may not be load or store
  assert(current_event.is_write() || current_event.is_read());
  assert(0 <= current_thread && current_thread < long(threads.size()));
  
  //update last_write
  if(current_event.is_write()){
    last_write[current_event.object.first][current_event.object.second] = current_event.iid;
    mem[current_event.object.first][current_event.object.second] = current_event.value;
  }
  //update vpo when read is done
  if(current_event.is_read()) { 
    visible[current_event.object.first][current_event.object.second].execute_read(current_event.get_pid(), last_write[current_event.object.first][current_event.object.second], mem[current_event.object.first][current_event.object.second]);
    current_event.value = mem[current_event.object.first][current_event.object.second];
    threads[current_thread][current_event.get_id()].value = current_event.value;
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
      IID<IPid> iid = IID<IPid>(IPid(i), threads[i].events.size());
      threads[i].push_back(no_load_store);
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
    }
    else { //no read of RW-pair available in enabled
      next_event = Enabled.front(); // pick any event from enabled
    }

    update_leads(next_event, forbidden);
    
    if (!states[current_state].has_unexplored_leads()) {
      Sequence seq(next_event, &threads);
      Lead l(seq, forbidden); 
      consistent_union(current_state, l);
      states[current_state].lead_head_execution_prefix = prefix_idx;
      alpha_head = prefix_idx;
    }
  }
  else { // has a lead to explore
    states[current_state].executing_alpha_lead = true;
    states[current_state].lead_head_execution_prefix = alpha_head;

    Lead l(to_explore, forbidden);
    consistent_union(current_state, l);
    to_explore.pop_front();
  }
}

void ViewEqTraceBuilder::execute_next_lead() {
  // ////
  // llvm::outs() << "has unexplored leads\n";
  // // [snj]: TODO remove - only for debug
  // llvm::outs() << "unexplored leads:\n";
  // std::vector<Lead> unexl = states[current_state].unexplored_leads();
  // for (auto it = unexl.begin(); it != unexl.end(); it++) {
  //   llvm::outs() << "lead: ";
  //   llvm::outs() << (*it).to_string() << "\n";
  // }
  // ////
  Lead next_lead = states[current_state].next_unexplored_lead();
  IID<IPid> next_event = states[current_state].alpha_sequence().head();
  Event next_Event = get_event(next_event);
  to_explore = states[current_state].alpha_sequence().tail();
  
  auto it = std::find(Enabled.begin(), Enabled.end(), next_event);
  assert(it != Enabled.end());
  Enabled.erase(it);
  
  // update forbidden with lead forbidden and read event executed
  forbidden = next_lead.forbidden;
  if(next_Event.is_read()){ // update formula for value read
      forbidden.reduce(std::make_pair(next_Event.iid, mem[next_Event.object.first][next_Event.object.second]));
  }

  current_thread = next_event.get_pid();
  current_event = threads[current_thread][next_event.get_index()];
  assert(current_event.is_write() || current_event.is_read());
  assert(0 <= current_thread && current_thread < long(threads.size()));

  //update last_write
  if(current_event.is_write()){
    last_write[current_event.object.first][current_event.object.second] = current_event.iid;
    mem[current_event.object.first][current_event.object.second] = current_event.value;
  }
  //update vpo when read is done
  if(current_event.is_read()) { 
    visible[current_event.object.first][current_event.object.second].execute_read(current_event.get_pid(), last_write[current_event.object.first][current_event.object.second], mem[current_event.object.first][current_event.object.second]);
    current_event.value = mem[current_event.object.first][current_event.object.second];
    threads[current_thread][current_event.get_id()].value = current_event.value;    
  }
  // [snj]: record current event as next in execution sequence
  execution_sequence.push_back(current_event.iid);
  prefix_state.push_back(current_state);
  prefix_idx++;
  threads[current_thread].awaiting_load_store = false; // [snj]: next event after current may not be load or store
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
  Sequence uiseq(ui, &threads);

  SOPFormula fui_all = forbidden;
  for (auto it = ui.begin(); it != ui.end(); it++) {
    if (threads[(*it).get_pid()][(*it).get_index()].value == mem[event.object.first][event.object.second]) 
      continue; // current value is not forbidden 
    fui_all || std::make_pair(event.iid, threads[(*it).get_pid()][(*it).get_index()].value);
  }

  uiseq.push_front(event.iid);
  std::vector<Lead> L;

  if (!(forbidden.check_evaluation(std::make_pair(event.iid, mem[event.object.first][event.object.second])) == RESULT::TRUE))
    L.push_back(Lead(empty_sequence, uiseq, fui_all));

  for (auto it = ui.begin(); it != ui.end(); it++) {
    if (get_event(*it).value == mem[event.object.first][event.object.second]) 
      continue;

    Sequence start = uiseq;
    start.erase((*it));
    start.push_front((*it));

    SOPFormula fui = forbidden; 
    fui || (std::make_pair(event.iid, mem[event.object.first][event.object.second]));
    for (auto it2 = ui.begin(); it2 != ui.end(); it2++) {
      if (it2 != it)
        fui || std::make_pair(event.iid, get_event((*it2)).value);
    }

    Lead l(empty_sequence, start, fui);
    L.push_back(l);
  }

  consistent_union(current_state, L); // add leads at current state
}

void ViewEqTraceBuilder::backward_analysis_read(Event event, SOPFormula& forbidden, std::unordered_map<int, std::vector<Lead>>& L) {
  std::unordered_set<IID<IPid>> ui = unexploredInfluencers(event, forbidden);
  std::unordered_set<IID<IPid>> ei = exploredInfluencers(event, forbidden);
  
  std::unordered_set<int> ui_values;
  SOPFormula fui(std::make_pair(event.iid, mem[event.object.first][event.object.second]));
  for (auto it = ui.begin(); it != ui.end(); it++) {
    fui || std::make_pair(event.iid, get_event((*it)).value);
    ui_values.insert(get_event((*it)).value);
  }
  
  for (auto it = ei.begin(); it != ei.end(); it++) {
    if (get_event((*it)).value == mem[event.object.first][event.object.second]
        || ui_values.find(get_event((*it)).value) != ui_values.end())
      continue; // skip current value, it is covered in fwd analysis

    int es_idx = execution_sequence.indexof((*it)); // index in execution_sequnce of ei
    if (execution_sequence[es_idx].get_pid() == 0) { // if ei is init event
      while (execution_sequence[es_idx].get_pid() != event.get_pid()) { // move down to state id where thread of event created
        es_idx++;
      }
    }

    while (prefix_state[es_idx] == -1) { // next state to add lead
      es_idx++;
    }

    int event_index = prefix_state[es_idx];
    Sequence start = execution_sequence.backseq((*it), event.iid);

    if (states[event_index].executing_alpha_lead) { // part of of alpha, alpha cannot be modified, move to head of alpha      
      Sequence alpha_prefix(execution_sequence[states[event_index].lead_head_execution_prefix], &threads);
      int ex_idx = states[event_index].lead_head_execution_prefix + 1;
      while (execution_sequence[ex_idx] != (*it)) {
        if (prefix_state[ex_idx] >= 0) // this event has a state => this event is a global R/W
          alpha_prefix.push_back(execution_sequence[ex_idx]);
        ex_idx++;
      }
      
      event_index = prefix_state[states[event_index].lead_head_execution_prefix];
      alpha_prefix.concatenate(start);
      start = alpha_prefix;
    }
    
    Sequence constraint = states[event_index].alpha_sequence();
    SOPFormula fei;
    for (auto i = ei.begin(); i != ei.end(); i++) {
      if (i == it) continue;
      fei || std::make_pair(event.iid, get_event((*i)).value);
    }
    
    SOPFormula inF = states[event_index].leads[states[event_index].alpha].forbidden;
    
    inF || fui; inF || fei;
    L[event_index].push_back(Lead(constraint, start, inF));  
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

    int event_index = prefix_state[execution_sequence.indexof((*it))];
    Sequence start = execution_sequence.backseq((*it), event.iid);

    if (states[event_index].executing_alpha_lead) { // part of of alpha, alpha cannot be modified, move to head of alpha      
      Sequence alpha_prefix(execution_sequence[states[event_index].lead_head_execution_prefix], &threads);
      int ex_idx = states[event_index].lead_head_execution_prefix + 1;
      while (execution_sequence[ex_idx] != (*it)) {
        if (prefix_state[ex_idx] >= 0) // this event has a state => this event is a global R/W
          alpha_prefix.push_back(execution_sequence[ex_idx]);
        ex_idx++;
      }
      
      event_index = prefix_state[states[event_index].lead_head_execution_prefix];
      alpha_prefix.concatenate(start);
      start = alpha_prefix;
    }

    Sequence constraint = states[event_index].alpha_sequence();
    SOPFormula inF = states[event_index].leads[states[event_index].alpha].forbidden;
    (inF || std::make_pair((*it),it_val));
    L[event_index].push_back(Lead(constraint, start, inF));

    // add to the list of EW reads values covered
    covered_read_values[(*it)].push_back(event.value);
  }
}

void ViewEqTraceBuilder::backward_analysis(Event event, SOPFormula& forbidden) {
  std::unordered_map<int, std::vector<Lead>> L; // map of execution index -> leads to be added at state[execution index]

  if (event.is_read()) {
    backward_analysis_read(event, forbidden, L);
  }
  else if (event.is_write())  {
    backward_analysis_write(event, forbidden, L);
  }

  for (auto it = L.begin(); it != L.end(); it++) {
    consistent_union(it->first, it->second); // consistent join at respective execution prefix
  }
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
  return mem[obj.first][obj.second];
}

void ViewEqTraceBuilder::metadata(const llvm::MDNode *md)
{
  // [rmnt]: Originally, they check whether dryrun is false before updating the current event's metadata and also maintain a last_md object inside TSOTraceBuilder. Since we don't use dryrun, we have omitted the checks and also last_md
  assert(current_event.md == 0);
  current_event.md = md;
  last_md = md;
}

bool ViewEqTraceBuilder::record_symbolic(SymEv event)
{
  llvm::outs() << "in record symbolic\n";
  assert(false);
  return false;
}

int ViewEqTraceBuilder::find_replay_state_prefix() {
  int replay_state_prefix = states.size() - 1;
  for (auto it = states.end(); it != states.begin();) {
    it--;
    
    if (it->has_unexplored_leads()) { // found replay state      
      break;
    }

    replay_state_prefix --;
  }

  return replay_state_prefix;
}

bool ViewEqTraceBuilder::reset() {
  int replay_state_prefix = find_replay_state_prefix();
  if (replay_state_prefix < 0) {// no more leads to explore, model checking complete
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
  visible.clear();
  last_write.clear();
  covered_read_values.clear();

  forbidden = states[replay_state_prefix].forbidden;

  prefix_idx = 0;
  replay_point = replay_execution_prefix;
  
  // last_full_memory_conflict = -1;
  // dryrun = false;
  // dry_sleepers = 0;
  last_md = 0; // [snj]: TODO ??
  reset_cond_branch_log();

  return true;
}

IID<CPid> ViewEqTraceBuilder::get_iid() const{
  IID<CPid> i;
  return i;
}

void ViewEqTraceBuilder::refuse_schedule() {
  // llvm::outs() << "refuse schedule\n";
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

bool ViewEqTraceBuilder::spawn()
{
  Event event(SymEv::Spawn(threads.size()));
  event.make_spawn();
  event.object = std::make_pair(threads.size(), 0);

  // remove the no_load_store event added by exists_non_memory access 
  // add replace with the event crated in this fucntion.
  threads[current_thread].pop_back();
  execution_sequence.pop_back();
  
  // [snj]: create event in thread that is spawning a new event
  current_event = event;
  current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].events.size());
  
  threads[current_thread].push_back(current_event);
  execution_sequence.push_back(current_event.iid);

  // [snj]: setup new program thread
  IPid parent_ipid = current_event.iid.get_pid();
  CPid child_cpid = CPS.spawn(threads[parent_ipid].cpid);
  /* Spawn event of thread is dummy */
  threads.push_back(Thread(child_cpid, -42));

  for(auto it1 = visible.begin(); it1 != visible.end(); it1++) {
    for (auto it = it1->second.begin(); it != it1->second.end(); it++) {
      (it->second).add_thread();
    }
  }
  return true;
}

bool ViewEqTraceBuilder::join(int tgt_proc) {
  Event event(SymEv::Join(tgt_proc));
  event.make_join();
  event.object = std::make_pair(tgt_proc, 0);

  // remove the no_load_store event added by exists_non_memory access 
  // add replace with the event crated in this fucntion.
  threads[current_thread].pop_back();
  execution_sequence.pop_back();

  current_event = event;
  current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].events.size());
  
  threads[current_thread].push_back(current_event);
  execution_sequence.push_back(current_event.iid);

  return true;
}

bool ViewEqTraceBuilder::store(const SymData &ml) {
  // [snj]: visitStoreInst in Execution.cpp lands in atomic_store not here
  Event event(SymEv::Store(ml));
  event.make_write();
  current_event = event;

  if (current_event.is_global()) {
    /* a global event is peeked from Execution.cpp run(), this is function
       call is a result of the peek. Hence, a no_load_store event was not adeed 
       by exists_non_memory_access function and thus there is no pop in case
       of a global event.
    */
    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].events.size());
    threads[current_thread].push_back(current_event);
    threads[current_thread].awaiting_load_store = true;

    assert(!is_enabled(current_thread)); // [snj]: only 1 event of each thread is enabled
    Enabled.push_back(current_event.iid);
    if(current_thread == 0){
      Visible vis(current_event.iid, current_event.value);
      visible[current_event.object.first].insert({current_event.object.second, vis});
    }
    else
      visible[current_event.object.first][current_event.object.second].add_enabled_write(current_event.iid, current_event.value);
  }
  else {
    /* exists_non_memory_access adds a no_load_store event in thread before executing
       the next event of thread, if the next event is a non-global load or strore
       then the no_load_store event is popped and load/store event in added to the
       thread of the event.
    */
    threads[current_thread].pop_back();
    execution_sequence.pop_back();

    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].events.size());
    threads[current_thread].push_back(current_event);
    execution_sequence.push_back(current_event.iid);
  }

  return true;
}

bool ViewEqTraceBuilder::atomic_store(const SymData &ml) {
  // [snj]: visitStoreInst in Execution.cpp lands here not in store
  Event event(SymEv::Store(ml));
  event.make_write();
  current_event = event;

  if (current_event.is_global()) {
    /* a global event is peeked from Execution.cpp run(), this is function
       call is a result of the peek. Hence, a no_load_store event was not adeed 
       by exists_non_memory_access function and thus there is no pop in case
       of a global event.
    */
    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].events.size());
    threads[current_thread].push_back(current_event);
    threads[current_thread].awaiting_load_store = true;

    assert(!is_enabled(current_thread)); // [snj]: only 1 event of each thread is enabled
    Enabled.push_back(current_event.iid);

    if(current_thread == 0){
      Visible vis( current_event.iid, current_event.value);
      visible[current_event.object.first].insert({current_event.object.second, vis});
    }
    else
      visible[current_event.object.first][current_event.object.second].add_enabled_write(current_event.iid, current_event.value);
  }
  else {
    /* exists_non_memory_access adds a no_load_store event in thread before executing
       the next event of thread, if the next event is a non-global load or strore
       then the no_load_store event is popped and load/store event in added to the
       thread of the event.
    */
    threads[current_thread].pop_back();
    execution_sequence.pop_back();

    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].events.size());
    threads[current_thread].push_back(current_event);
    execution_sequence.push_back(current_event.iid);
  }

  return true;
}

bool ViewEqTraceBuilder::load(const SymAddrSize &ml) {
  Event event(SymEv::Load(ml));
  event.make_read();
  event.value = mem[event.object.first][event.object.second];
  current_event = event;

  if (current_event.is_global()) {
    /* a global event is peeked from Execution.cpp run(), this is function
       call is a result of the peek. Hence, a no_load_store event was not adeed 
       by exists_non_memory_access function and thus there is no pop in case
       of a global event.
    */
    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].events.size());
    threads[current_thread].push_back(current_event);
    threads[current_thread].awaiting_load_store = true;

    assert(!is_enabled(current_thread)); // [snj]: only 1 event of each thread is enabled
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

    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].events.size());
    threads[current_thread].push_back(current_event);
    execution_sequence.push_back(current_event.iid);
  }


  return true;
}

bool ViewEqTraceBuilder::fence() {
  // [snj]: invoked at pthread create
  return true;
}

Trace* ViewEqTraceBuilder::get_trace() const {
  std::vector<IID<CPid>> cmp;
  SrcLocVectorBuilder cmp_md;
  std::vector<Error *> errs;
  
  for (unsigned i = 0; i < execution_sequence.size(); i++) {
    cmp.push_back(IID<CPid>(threads[execution_sequence[i].get_pid()].cpid, execution_sequence[i].get_index()));
    cmp_md.push_from(get_event(execution_sequence[i]).md);
  };

  for (unsigned i = 0; i < errors.size(); ++i)
  {
    errs.push_back(errors[i]->clone());
  }

  Trace *t = new IIDSeqTrace(cmp, cmp_md.build(), errs);
  
  return t;
} 

// [snj]: HOW TO READ
//  - stack operations have been abstracted ( as they are mostly parameters 'void *arg')
//  - heap operations also abstracted for now
//  - Load(Global(object_id)(block_size))(Event::value)
//  - Store(Global(object_id)(block_size),value)(Event::value)
void ViewEqTraceBuilder::debug_print() const {
  llvm::outs() << "Debug_print: execution_sequence.size()=" << execution_sequence.size() << "\n";
  for (int i = 0; i < execution_sequence.size(); i++) {
    int thread = execution_sequence[i].get_pid();
    Event event = threads[thread][execution_sequence[i].get_index()];
   
    if (i < 10) llvm::outs() << " [" << i << "]: "; 
    else llvm::outs() << "[" << i << "]: ";

    if (event.symEvent.size() < 1) {
      llvm::outs() << "--\n";
      continue;
    }
    if (event.sym_event().addr().addr.block.is_stack()) {
      llvm::outs() << "Stack Operation \n";
      continue;
    }
    if (event.sym_event().addr().addr.block.is_heap()) {
      llvm::outs() << "Heap Operation \n";
      continue;
    }
    llvm::outs() << event.to_string() << " = ";
    llvm::outs() << event.sym_event().to_string() << "\n";
  }
} 

bool ViewEqTraceBuilder::compare_exchange(const SymData &sd, const SymData::block_type expected, bool success)
                                                    {llvm::outs() << "[snj]: cmp_exch being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::sleepset_is_empty() const{llvm::outs() << "[snj]: sleepset_is_empty being invoked!!\n"; assert(false); return true;}
bool ViewEqTraceBuilder::check_for_cycles(){llvm::outs() << "[snj]: check_for_cycles being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::mutex_lock(const SymAddrSize &ml){llvm::outs() << "[snj]: mutex_lock being invoked!!\n"; assert(false); return true;}
bool ViewEqTraceBuilder::mutex_lock_fail(const SymAddrSize &ml){llvm::outs() << "[snj]: mutex_lock_fail being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::mutex_trylock(const SymAddrSize &ml){llvm::outs() << "[snj]: mutex_trylock being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::mutex_unlock(const SymAddrSize &ml){llvm::outs() << "[snj]: mutex_unlock being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::mutex_init(const SymAddrSize &ml){llvm::outs() << "[snj]: mutex_init being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::mutex_destroy(const SymAddrSize &ml){llvm::outs() << "[snj]: mutex_ destroy being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::cond_init(const SymAddrSize &ml){llvm::outs() << "[snj]: cond_init being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::cond_signal(const SymAddrSize &ml){llvm::outs() << "[snj]: cond_signal being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::cond_broadcast(const SymAddrSize &ml){llvm::outs() << "[snj]: cond_broadcast being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::cond_wait(const SymAddrSize &cond_ml,
                        const SymAddrSize &mutex_ml){llvm::outs() << "[snj]: cond_wait being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::cond_awake(const SymAddrSize &cond_ml,
                        const SymAddrSize &mutex_ml){llvm::outs() << "[snj]: cond_awake being invoked!!\n"; assert(false); return false;}
int ViewEqTraceBuilder::cond_destroy(const SymAddrSize &ml){llvm::outs() << "[snj]: cond_destroy being invoked!!\n"; assert(false); return false;}
bool ViewEqTraceBuilder::register_alternatives(int alt_count){llvm::outs() << "[snj]: register_alternatives being invoked!!\n"; assert(false); return false;}

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

bool ViewEqTraceBuilder::Event::operator==(Event event) {
  if (value != event.value)
    return false;

  return (symEvent == event.symEvent);
}

bool ViewEqTraceBuilder::Event::operator!=(Event event) {
  return !(*this == event);
}

std::string ViewEqTraceBuilder::Event::type_to_string() const {
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
      return ("[" + std::to_string(get_pid()) + ":" + std::to_string(get_id()) + "] (" + type_to_string() + "(" + std::to_string(object.first) + "," + std::to_string(value) + "))");
    return ("[" + std::to_string(get_pid()) + ":" + std::to_string(get_id()) + "] (" + type_to_string() + "(" + std::to_string(object.first) + "+" + std::to_string(object.second) + "," + std::to_string(value) + "))");  
  }
  
  return ("[" + std::to_string(get_pid()) + ":" + std::to_string(get_id()) + "] (" + type_to_string() + ":thread" + std::to_string(object.first) + ")");
}

std::unordered_map<IID<IPid>, int> ViewEqTraceBuilder::Sequence::read_value_map() {
  std::unordered_map<IID<IPid>, int> reads; // read -> value
  std::unordered_map<unsigned, std::unordered_map<unsigned, int>> writes; // object -> last updated value
  
  for (auto it = begin(); it != end(); it++) {
    Event ev = threads->at(it->get_pid())[it->get_index()];
    if (ev.is_write()) {
      writes[ev.object.first][ev.object.second] = ev.value;
      continue;
    }
    if (ev.is_read()) {
      if (writes[ev.object.first].find(ev.object.second) != writes[ev.object.first].end())
        reads[(*it)] = writes[ev.object.first][ev.object.second];
      else
        reads[(*it)] = 0; // init
    }
  }

  return reads;
}

bool ViewEqTraceBuilder::Sequence::VA_isprefix(Sequence s) {
  if (isprefix(s)) return true; // this is a prefix wihtout needing view-adjustment

  Sequence s_original = s;

  for (auto ethis = end(); ethis != begin();) { ethis--;
    if (s.find(*ethis) == s.end()) return false; // if next event not in s then this is not a prefix

    // remove event ethis from its current index in s
    s.erase(*ethis);
  }
  
  // add this to the front of remaining s
  Sequence s_modified = *this;
  s_modified.concatenate(s);
  
  std::unordered_map<IID<IPid>, int> original_values_read = s_original.read_value_map(); // values read in original s
  std::unordered_map<IID<IPid>, int> modified_values_read = s_modified.read_value_map(); // values read in modified s

  // compare if the same values were read
  for (auto it = original_values_read.begin(); it != original_values_read.end(); it++) {
    if (it->second != modified_values_read[it->first]) // values must match
      return false; // returns false even when it->first not in modified_values_read
  }

  return true;
}

bool ViewEqTraceBuilder::Sequence::VA_equivalent(Sequence s) {
  if ((*this) == s) return true; // this and s are equiavelent wihtout needing view-adjustment
  
  std::unordered_map<IID<IPid>, int> thisreads, sreads; // read -> value
  thisreads = read_value_map();
  sreads = s.read_value_map();

  if (thisreads.size() != sreads.size()) return false; // no of reads must be same for being equivalent

  for (auto it = thisreads.begin(); it != thisreads.end(); it++) {
    if (sreads.find(it->first) == sreads.end()) return false;

    if (it->second != sreads[it->first]) { // values must match
      return false; // returns false even when it->first not in sreads
    }
  }

  return true;
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::VA_suffix(Sequence prefix) {
  assert(prefix.VA_prefix(*this)); // 'prefix' is a VA prefix of *this (suffix is only called after this check)
  Sequence suffix = *this;

  /* since wkt 'prefix' is a prefix of *this, thus we only need to remove
     the events of 'prefix' to get the suffix
  */
  for (auto it = suffix.begin(); it != suffix.end();) {
    if (prefix.has(*it)) {
      it = suffix.erase(it);
      continue;
    }

    it++;
  }

  return suffix;
}

bool ViewEqTraceBuilder::Sequence::view_adjust(IID<IPid> e1, IID<IPid> e2) {
  std::vector<IID<IPid>> original_events = events;

  Event ev1 = threads->at(e1.get_pid())[e1.get_index()];
  Event ev2 = threads->at(e2.get_pid())[e2.get_index()];

  int n2 = indexof(e2);
  int n1 = indexof(e1);
  for (int i = n2 - 1, delim = 0; i >= n1; i--) { // from before e2 till e1
    if (events[i].get_pid() != e1.get_pid()) continue; // shift only events from e1's thread

    for (int j = i; j < n2 - delim; j++) {
      Event ecurr = threads->at(events[j].get_pid())[events[j].get_index()];
      Event enext = threads->at(events[j+1].get_pid())[events[j+1].get_index()];

      if (ecurr.RWpair(enext)) { // if cannot exchange
        events = original_events; // restrore original sequence
        return false;
      }

      // can exchange
      IID<IPid> tmp = events[j];
      events[j] = events[j+1];
      events[j+1] = tmp;

      delim++; // shifted 1 event of e1's thread, now 1 location fixed
    }
  }

  return true;
}

bool ViewEqTraceBuilder::Sequence::conflicts_with(ViewEqTraceBuilder::Sequence &other_seq) {
  return conflicts_with(other_seq, true).first;
}

std::pair<bool, std::pair<IID<IPid>, IID<IPid>>> ViewEqTraceBuilder::Sequence::conflicts_with(ViewEqTraceBuilder::Sequence &other_seq, bool returnRWpair) {
  if (!returnRWpair) conflicts_with(other_seq);

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

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::po_prefix_master(IID<IPid> e1, IID<IPid> e2, sequence_iterator begin, sequence_iterator end){ 
  std::vector<unsigned> joining_threads;
  Sequence local_suffix(threads);
  Sequence po_pre(threads);
  
  for (auto it = end; it != begin; it--) {
    auto i = it-1;
    Event event = threads->at(i->get_pid())[i->get_index()];

    if (i->get_pid() == e2.get_pid()) {
      if (event.is_global())
        local_suffix.push_front(*i);
      if (event.type == Event::ACCESS_TYPE::JOIN) {
        joining_threads.push_back(event.object.first);
      }
    }
    else if (i->get_pid() == e1.get_pid()) {
      if (event.is_global())
        local_suffix.push_front(*i);
    }
    else {
      if (std::find(joining_threads.begin(), joining_threads.end(), i->get_pid()) != joining_threads.end()) {
        // events from joined threads, not from thread of e1
        if (event.is_global())
          po_pre.push_front(*i);
      }
    }
  }

  local_suffix.push_front(e1);
  po_pre.concatenate(local_suffix);
  
  return po_pre;
}

std::pair<std::pair<bool, bool>, ViewEqTraceBuilder::Sequence> ViewEqTraceBuilder::Sequence::po_prefix(IID<IPid> e1, IID<IPid> e2, sequence_iterator begin, sequence_iterator end){
  assert(has(e2));
  sequence_iterator it;
  Sequence po_pre(threads);

  bool included_e1 = false;
  bool included_e1_thread_events = false;

  std::unordered_map<unsigned, std::unordered_set<unsigned>> objects_for_source; // list of objects whose last source has not been found yet
  std::vector<IPid> back_threads; // other threads who are a part of backseq to enable event (in case of conditionals)

  if (e2.get_pid() == 0) { // read event in thread 0 after join to check assertion
    return std::make_pair(std::make_pair(included_e1, included_e1_thread_events), po_prefix_master(e1, e2, begin, end));
  }

  // iterate 
  for(it = end; it != begin;){ it--;
    if (it->get_pid() == 0) continue; // init or non-global event
    
    Event ite = threads->at(it->get_pid())[it->get_index()];

    if (it->get_pid() == e2.get_pid() || 
      std::find(back_threads.begin(), back_threads.end(), it->get_pid()) != back_threads.end()) {
      
      if (ite.is_global()) {
        if ((*it) == e1) included_e1 = true;
        po_pre.push_front(*it);

        if (ite.is_read()) {
          objects_for_source[ite.object.first].insert(ite.object.second);
        }
        else if (ite.is_write()) {
          objects_for_source[ite.object.first].erase(ite.object.second);    
        }
      }
    }

    else if (ite.is_write() && objects_for_source[ite.object.first].find(ite.object.second) != objects_for_source[ite.object.first].end()) {
      if (it->get_pid() == e1.get_pid()) { // e2 dependent on events of e1
        included_e1_thread_events = true;
      }

      objects_for_source[ite.object.first].erase(ite.object.second);
      back_threads.push_back(it->get_pid());
      po_pre.push_front(*it);
      if ((*it) == e1) included_e1 = true;
    } 
  }

  return std::make_pair(std::make_pair(included_e1, included_e1_thread_events), po_pre);
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::backseq(IID<IPid> e1, IID<IPid> e2){
  assert(this->has(e1) && this->has(e2));
  
  std::pair<std::pair<bool, bool>, ViewEqTraceBuilder::Sequence> po_pre_return = po_prefix(e1, e2, find(e1)+1, find(e2));
  bool included_e1 = po_pre_return.first.first;
  bool has_e1_thread_events = po_pre_return.first.second;
  ViewEqTraceBuilder::Sequence po_pre = po_pre_return.second;
  po_pre.push_back(e2);

  if (e1.get_pid() == 0 || e2.get_pid() == 0) { // event from thread zero ie init write event
    return po_pre; // init event is not added to backseq
  }

  if (included_e1) return po_pre;
  
  // if !included_e1 then include at appropriate location
  Event event = threads->at(e1.get_pid())[e1.get_index()];
  if(event.is_write()) {
    bool conflict = false;

    sequence_iterator loc = po_pre.begin();
    bool found_racing_write = false;
    for (auto it = po_pre.begin(); it != po_pre.end()-1; it++) {
      Event eit = threads->at(it->get_pid())[it->get_index()];
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
    po_pre.push_at(loc, e1);
  }
  else {
    if (has_e1_thread_events) { // events po after e1 are in po_pre then its not prefix of e1
      Sequence empty_sequence(threads); 
      return empty_sequence;
    }

    po_pre.push_back(e1);
  }

  return po_pre;
}


void  ViewEqTraceBuilder::Lead::project(std::tuple<ViewEqTraceBuilder::Sequence, ViewEqTraceBuilder::Sequence, ViewEqTraceBuilder::Sequence> &triple,
ViewEqTraceBuilder::Sequence &seq1, ViewEqTraceBuilder::Sequence &seq2, ViewEqTraceBuilder::Sequence &seq3) {
  seq1 = std::get<0>(triple);
  seq2 = std::get<1>(triple);
  seq3 = std::get<2>(triple);
}

std::tuple<ViewEqTraceBuilder::Sequence, ViewEqTraceBuilder::Sequence, ViewEqTraceBuilder::Sequence>
ViewEqTraceBuilder::Lead::join(Sequence &primary, Sequence &other, IID<IPid> delim, Sequence &joined)
{
  std::vector<Thread>* threads = primary.threads;
  typedef std::tuple<Sequence, Sequence, Sequence> return_type;
  
  if (primary.empty()) {
    joined.concatenate(other);
    primary.clear();
    other.clear();
    return std::make_tuple(primary, other, joined);
  }

  if (other.empty()) {
    joined.concatenate(primary);
    primary.clear();
    other.clear();
    return std::make_tuple(primary, other, joined);
  }
  
  IID<IPid> e = primary.head();
  
  if (e == delim) {
    if (other.head() == delim)
      other.pop_front();

    if (joined.last() == e) {
      primary.pop_front();
      return std::make_tuple(primary, other, joined);
    }
    else {
      primary.pop_front();
      joined.push_back(e);
      return std::make_tuple(primary, other, joined);
    }
  }

  Event ev = threads->at(e.get_pid())[e.get_index()];
  assert(ev.is_global()); // is a global read write

  if (ev.is_write()) { // algo 6-17
    if (!other.has(e)) { // e not in both primary and other [algo 6-12]
      IID<IPid> er;
      sequence_iterator it;
      for (it = other.begin(); it != other.end(); it++) {
        er = *it;
        Event event_er = threads->at(er.get_pid())[er.get_index()];
        if (!event_er.is_read()) continue;
        if (primary.has(er)) continue;
        if (ev.same_object(event_er)) break; // found a read er that is not in primary s.t. obj(e) == obj(er)
      }

      if (it != other.end()) { // there exists a read er in other that is not in primary s.t. obj(e) == obj(er)
        if (other.head() == delim && primary.has(delim)) { //other.front() is also waiting to be pushed later in joined seq
          Event edelim = threads->at(delim.get_pid())[delim.get_index()];
          if (edelim.is_write()) {
            primary.pop_front();
            other.pop_front();
            joined.push_back(e);
          }
          else {
            primary.clear();
            other.clear();
            joined.clear();
            return std::make_tuple(primary, other, joined);
          }
        }
        else {
          return_type triple = join(other, primary, er, joined);
          if (primary.empty() && other.empty())
            return std::make_tuple(primary, other, joined);

          project(triple, other, primary, joined);
          if (!joined.has(er)) joined.push_back(er); 
          if (!joined.has(e)) joined.push_back(e); 
        }
      }
      else { // there does not exist a read er that is not in primary s.t. obj(e) == obj(er)
        primary.pop_front();
        joined.push_back(e);
      }
    }
    else { // e in both primary and other [algo 13-17]
      if (other.head() == e) {
        primary.pop_front();
        other.pop_front();
        joined.push_back(e);
      }
      else {
        return_type triple = join(other, primary, e, joined);
        if (primary.empty() && other.empty())
          return std::make_tuple(primary, other, joined);

        project(triple, other, primary, joined);
        if (joined.last() != e) joined.push_back(e);
        if (primary.head() == e) primary.pop_front(); // [snj]: TODO is the check needed?
      }
    }
  }

  else if (ev.is_read()) { // algo 18-21
    if (!other.has(e)) { // algo 18
      joined.push_back(e);
      primary.pop_front();
    }
    else { // algo 19-21
      if (e == other.head()) { // [snj]: TODO this if not in algo
        primary.pop_front();
        other.pop_front();
        joined.push_back(e);
      } 
      else {
        return_type triple = join(other, primary, e, joined);
        if (primary.empty() && other.empty())
          return std::make_tuple(primary, other, joined);

        project(triple, other, primary, joined);
        if (!joined.has(e)) joined.push_back(e);
      }
    }
  }
  
  return join(primary, other, delim, joined);
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Lead::cmerge(Sequence &primary_seq, Sequence &other_seq) {
  std::vector<Thread>* threads = primary_seq.threads;
  
  assert(!primary_seq.empty());
  if (other_seq.empty()) return primary_seq;

  if (primary_seq.last().get_pid() == 0) return primary_seq; // lead created for assert check in master thread

  std::pair<bool, std::pair<IID<IPid>, IID<IPid>>> conflictingRWpair = primary_seq.conflicts_with(other_seq, true);
  while (conflictingRWpair.first) { // primary_seq and other_seq conflict
    if (primary_seq.view_adjust(conflictingRWpair.second.first, conflictingRWpair.second.second)) { // other_seq adjusted ensuring same view of reads
      conflictingRWpair = primary_seq.conflicts_with(other_seq, true);
    } 
    else if(other_seq.view_adjust(conflictingRWpair.second.second, conflictingRWpair.second.first)) {
      conflictingRWpair = primary_seq.conflicts_with(other_seq, true);
    }
    else { // conflicting and cannot be adjusted
      Sequence dummy(threads);
      return dummy;
    }
  }
  
  sequence_iterator it;
  for (it = primary_seq.begin(); it != primary_seq.end(); it++) {
    if (other_seq.has((*it)))
      break;
  }

  if (it == primary_seq.end()) { // no common events
    if (!primary_seq.hasRWpairs(other_seq)) { // no RW pair either
      other_seq.concatenate(primary_seq);
      return other_seq;
    }

    primary_seq.concatenate(other_seq);
    return primary_seq;
  }

  IID<IPid> dummy;
  Sequence joined(threads);
  std::tuple<Sequence, Sequence, Sequence> triple = join(primary_seq, other_seq, dummy, joined);
  assert(std::get<0>(triple).size() == 0);
  assert(std::get<1>(triple).size() == 0);

  return std::get<2>(triple);
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::subsequence(ViewEqTraceBuilder::sequence_iterator begin, ViewEqTraceBuilder::sequence_iterator end) {
  Sequence s(threads);

  for (auto it = begin; it != end; it++) {
    s.push_back(*it);
  }

  return s;
}

bool ViewEqTraceBuilder::Sequence::hasRWpairs(Sequence &seq) {
  for (sequence_iterator it1 = begin(); it1 != end(); it1++) {
    Event e1 = threads->at(it1->get_pid())[it1->get_index()];
    for (sequence_iterator it2 = seq.begin(); it2 != seq.end(); it2++) {
      Event e2 = threads->at(it2->get_pid())[it2->get_index()];
      if (e1.same_object(e2)) {
        if (e1.is_read() && e2.is_write()) return true;
        if (e1.is_write() && e2.is_read()) return true;
      }
    }
  }

  return false;
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

std::unordered_set<IID<IPid>> ViewEqTraceBuilder::unexploredInfluencers(Event er, SOPFormula& f){
  assert(er.is_read());
  std::unordered_set<IID<IPid>> ui;
  std::unordered_set<int> values,forbidden_values;

  for (int i = 0; i < Enabled.size(); i++){
    Event e = get_event(Enabled[i]);
    
    //[nau]:not a write event or not same object or already taken value or already checked forbidden value
    if(!e.is_write() || !er.same_object(e) || values.find(e.value) != values.end() || forbidden_values.find(e.value) != forbidden_values.end()) continue;

    //check if value of e is forbidden for er
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
  std::unordered_set<int> values, forbidden_values;

  std::pair<unsigned, unsigned> o_id = er.object;
  int pid = er.get_pid();
  
  auto it = visible.find(o_id.first);
  assert(it != visible.end());
  auto it1 = visible[o_id.first].find(o_id.second);
  assert(it1 != visible[o_id.first].end());
  assert(visible[o_id.first][o_id.second].mpo[0].size() == 1); //only the init event in first row
  assert(visible[o_id.first][o_id.second].mpo.size() == visible[o_id.first][o_id.second].visible_start.size() + 1);
  
  //[nau]: check if init event is visible to er
  if(visible[o_id.first][o_id.second].check_init_visible(pid)) {
    IID<IPid> initid = visible[o_id.first][o_id.second].mpo[0][0].first;
    int init_value = visible[o_id.first][o_id.second].mpo[0][0].second;
    
    assert(init_value == 0);
    if(f.check_evaluation(std::make_pair(initid, init_value)) != RESULT::TRUE ) ei.insert(initid);
    else forbidden_values.insert(init_value);
  }
  
  //for writes other than init
  // check if read is from thr0 after join
  if(pid == 0){
    if( ! visible[o_id.first][o_id.second].first_read_after_join ){
      ei.insert(last_write[o_id.first][o_id.second]);
      return ei;
    }
    for(int i = 1; i < visible[o_id.first][o_id.second].mpo.size(); i++){
      if(visible[o_id.first][o_id.second].mpo[i].size() != 0){
        std::pair<IID<IPid>, int> e = visible[o_id.first][o_id.second].mpo[i][visible[o_id.first][o_id.second].mpo[i].size() - 1];
        IID<IPid> e_id = e.first;
        int e_val = e.second;
        if(values.find(e_val) != values.end() || forbidden_values.find(e_val) != forbidden_values.end() || find(Enabled.begin(), Enabled.end(), e_id) != Enabled.end() ) continue;

        if(f.check_evaluation(std::make_pair(er.iid, e_val)) == RESULT::TRUE ) forbidden_values.insert(e_val);
        else{
          ei.insert(e_id);
          values.insert(e_val);
        }
      }
    }
  }
  else{
    for(int i = 0; i < visible[o_id.first][o_id.second].visible_start[pid - 1].size(); i++){
          int j = visible[o_id.first][o_id.second].visible_start[pid - 1][i];
          for( int k = j; k < visible[o_id.first][o_id.second].mpo[i + 1].size() + 1; k++){
              if(k == 0) continue;
              std::pair<IID<IPid>, int> e = visible[o_id.first][o_id.second].mpo[i + 1][k - 1];
              IID<IPid> e_id = e.first;
              int e_val = e.second;

              //repeated value || already checked forbidden value || unexplored event
              if(values.find(e_val) != values.end() || 
                forbidden_values.find(e_val) != forbidden_values.end() || 
                find(Enabled.begin(), Enabled.end(), e_id) != Enabled.end() ) continue;

              if(f.check_evaluation(std::make_pair(er.iid, e_val)) == RESULT::TRUE ) forbidden_values.insert(e_val);
              else{
                ei.insert(e_id);
                values.insert(e_val);
              }
          }
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

bool ViewEqTraceBuilder::Lead::operator==(Lead l) {
  if (constraint != l.constraint)
    return false;

  if (start != l.start)
    return false;

  return true;
}

std::string ViewEqTraceBuilder::Lead::to_string() {
  return ("(" + constraint.to_string() + ", " + start.to_string() + ", " + forbidden.to_string() + ")");
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

  if (states[state].alpha_sequence().VA_isprefix(lead.merged_sequence)) { // alpha sequence is prefix
    int fwd_state = prefix_state[execution_sequence.indexof(states[state].alpha_sequence().last())] + 1;
    assert(fwd_state >= 0 && fwd_state < states.size());
    
    SOPFormula f = (lead.forbidden);
    f || states[fwd_state].forbidden; // combine forbidden with forbidden of the state after current start
    
    Lead fwd_lead(states[fwd_state].alpha_sequence(), (lead.merged_sequence).VA_suffix(states[state].alpha_sequence()), f);
    forward_state_leads[fwd_state].push_back(std::move(fwd_lead));

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

void ViewEqTraceBuilder::consistent_union(int state, Lead& l) {
  std::vector<Lead> L;
  L.push_back(l);
  consistent_union(state, L);
}

void ViewEqTraceBuilder::consistent_union(int state, std::vector<Lead>& L) {
  std::unordered_map<int, std::vector<Lead>> forward_state_leads;

  if(states[state].leads.empty()) {
    forward_suffix_leads(forward_state_leads, state, L);
    for (auto itfl = forward_state_leads.begin(); itfl != forward_state_leads.end(); itfl++) {
      consistent_union(itfl->first, itfl->second);
    }

    states[state].leads = L;
    return;
  }

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
        combined_with_existing = true;
        break;
      }

      if (l->merged_sequence.VA_equivalent(sl->merged_sequence)) { // new lead has the same view as an existing lead
        // llvm::outs() << "VAequivalent " << sl->merged_sequence.to_string() << " and " << l->merged_sequence.to_string() << "\n";
        if (!sl->is_done) {
          rem.insert(sl - states[state].leads.begin());
          SOPFormula f = l->forbidden;
          f || sl->forbidden;
            add.push_back(Lead(sl->constraint, sl->start, f));
        }

        combined_with_existing = true;
        break;
      }

      if (sl->merged_sequence != states[state].alpha_sequence() && sl->merged_sequence.VA_isprefix(l->merged_sequence)) {
        // llvm::outs() << "prefix " << sl->merged_sequence.to_string() << " of " << l->merged_sequence.to_string() << "\n";
        if (!sl->is_done) {
          rem.insert(sl - states[state].leads.begin());
          SOPFormula f = l->forbidden;
          f && sl->forbidden;
            add.push_back(Lead(sl->constraint, sl->start, f));
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

  ////
  // llvm::outs() << "remaining state " << state << " leads:\n";
  // for (auto it = states[state].leads.begin(); it != states[state].leads.end(); it++) {
  //   llvm::outs() << it->to_string() << "\n";
  // }
  ////

  for (auto it = rem.begin(); it != rem.end(); it++) {
    // llvm::outs() << "removing index=" << (*it) << ": " << (states[state].leads.begin() + (*it))->to_string() << "\n";
    states[state].leads.erase(states[state].leads.begin() + (*it));
  }

  for (auto it = add.begin(); it != add.end(); it++) {
    // llvm::outs() << "adding lead: " << it->to_string() << "\n";
    states[state].leads.push_back(*it);
  }

  for (auto it = L.begin(); it != L.end(); it++) {
    // llvm::outs() << "adding lead: " << it->to_string() << "\n";
    states[state].leads.push_back(*it);
  }

  for (auto itfl = forward_state_leads.begin(); itfl != forward_state_leads.end(); itfl++) {
    consistent_union(itfl->first, itfl->second);
  }
}