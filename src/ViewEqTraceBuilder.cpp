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
  // llvm::outs() << "prefix_idx=" << prefix_idx << ", replay_point=" << replay_point << "\n";

  if (is_replaying()) {
    llvm::outs() << "REPLAYING: ";
    replay_schedule(proc);
    return true;
  }

  if (at_replay_point()) {
    llvm::outs() << "AT REPLAY PT: ";
    
    current_state = prefix_state[prefix_idx];
    assert(states[current_state].has_unexplored_leads());

    execution_sequence.pop_back();
    prefix_state.pop_back();
    
    execute_next_lead();
    *proc = current_thread;
    return true;
  }

  // [snj]: peak next read/write event / execute next non-read/write event
  if (exists_non_memory_access(proc))
    return true;

  if (Enabled.empty()) {
    assert(forbidden.evaluate != RESULT::TRUE);
    debug_print();
    return false; // [snj]: maximal trace explored
  }

  // llvm::outs() << "exploring R or W (forbidden=" << forbidden.to_string() << ")\n";

  // [snj]: Explore Algo function
  make_new_state(); // llvm::outs() << "made new state\n";
  compute_new_leads(); // llvm::outs() << "made new leads\n"; // next event for exploring forward

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
  // for (int i = threads.size()-1; i >=0 ; i--) {
  //   if (threads[i].available && !threads[i].awaiting_load_store) {   // && (conf.max_search_depth < 0 || threads[i].events.size() < conf.max_search_depth)){
  //     IID<IPid> iid = IID<IPid>(IPid(i), threads[i].events.size());
  //     threads[i].push_back(no_load_store);
  //     prefix_idx++;

  //     current_thread = i;
  //     *proc = i;
  //     return true;
  //   }
  // }

  // return false;
}

void ViewEqTraceBuilder::replay_memory_access(int next_replay_thread, IID<IPid> next_replay_event) {
  assert(prefix_state[prefix_idx] >= 0);
  assert(next_replay_event.get_index() == (threads[next_replay_thread].size()-1));

  // llvm::outs() << "Replay RW event=" << next_replay_event.to_string() << "\n";
  auto it = std::find(Enabled.begin(), Enabled.end(), next_replay_event);
  assert(it != Enabled.end());
  Enabled.erase(it);

  current_state = prefix_state[prefix_idx];
  current_thread = next_replay_thread;
  current_event = get_event(next_replay_event);
  threads[current_thread].awaiting_load_store = false; // [snj]: next event after current may not be load or store
  assert(current_event.is_write() || current_event.is_read());
  assert(0 <= current_thread && current_thread < long(threads.size()));
  // llvm::outs() << "replay: set current thread and event \n";

  //update last_write
  if(current_event.is_write()){
    last_write[current_event.object] = current_event.iid;
    mem[current_event.object] = current_event.value;
    // llvm::outs() << "replay: set mem and last write \n";
  }
  //update vpo when read is done
  if(current_event.is_read()) { 
    visible[current_event.object].execute_read(current_event.get_pid(), last_write[current_event.object]);
    current_event.value = mem[current_event.object];
    threads[current_thread][current_event.get_id()].value = current_event.value;
  }
}

void ViewEqTraceBuilder::replay_schedule(int *proc) {
  // if (replay_non_memory_access(proc))
  //   return;

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
    if (threads[i].available && !threads[i].awaiting_load_store) {   // && (conf.max_search_depth < 0 || threads[i].events.size() < conf.max_search_depth)){
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
  next_state.done = done;
  
  current_state = states.size();
  states.push_back(next_state);
}

void ViewEqTraceBuilder::compute_new_leads() {
  if (to_explore.empty()) {
    IID<IPid> next_event;
    // llvm::outs() << "to_explore empty\n";
    // read of an enabled read-write pair
    std::pair<bool, IID<IPid>> enabled_read_of_RW = enabaled_RWpair_read();
    if (enabled_read_of_RW.first == true) {
      next_event = enabled_read_of_RW.second; // read event of RW pair (event removed from enabled)
      // llvm::outs() << "got read of RW\n";
      analyse_unexplored_influenecers(next_event);
    }
    else { //no read of RW-pair available in enabled
      // llvm::outs() << "no RW pair\n";
      next_event = Enabled.front(); // pick any event from enabled
    }

    // llvm::outs() << "updating leads (forbidden=" << forbidden.to_string() << ")\n";
    update_leads(next_event, forbidden);
    // llvm::outs() << "updated leads (forbidden=" << forbidden.to_string() << ")\n";
    // llvm::outs() << "checking if state[" << current_state << "] has unexploredleads (states.siez=" << states.size() << ") \n";
    if (!states[current_state].has_unexplored_leads()) {
      // llvm::outs() << "making seq of next event (forbidden=" << forbidden.to_string() << ")\n";
      Sequence seq(next_event, &threads);
      Lead l(seq, forbidden); 
      states[current_state].consistent_join(l);
    }
  }
  else { // has a lead to explore
    // llvm::outs() << "to_explore not empty = " << to_explore.to_string() << "\n";
    Lead l(to_explore, forbidden);
    states[current_state].consistent_join(l);
    to_explore.pop_front();
  }
}

void ViewEqTraceBuilder::execute_next_lead() {
  llvm::outs() << "has unexplored leads\n";
  // [snj]: TODO remove - only for debug
  llvm::outs() << "unexplored leads:\n";
  std::vector<Lead> unexl = states[current_state].unexplored_leads();
  for (auto it = unexl.begin(); it != unexl.end(); it++) {
    llvm::outs() << "lead: ";
    llvm::outs() << (*it).to_string() << "\n";
  }
   //
  Lead next_lead = states[current_state].next_unexplored_lead();
  // llvm::outs() << "got lead=" << next_lead.to_string() << "\n";
  states[current_state].alpha = next_lead;
  // llvm::outs() << "updated alphaseq=" << states[current_state].alpha_sequence().to_string() << "\n";
  IID<IPid> next_event = states[current_state].alpha_sequence().head();
  to_explore = states[current_state].alpha_sequence().tail();
  // llvm::outs() << "set at states[" << current_state << "]: alpha=" << states[current_state].alpha_sequence().to_string() << ", to_explore=" << to_explore.to_string() << "\n";

  auto it = std::find(Enabled.begin(), Enabled.end(), next_event);
  assert(it != Enabled.end());
  Enabled.erase(it);
 
  update_done(next_event);
  update_forbidden(&next_lead);
  // llvm::outs() << "updated done, set forbidden \n";
  current_thread = next_event.get_pid();
  current_event = threads[current_thread][next_event.get_index()];
  assert(current_event.is_write() || current_event.is_read());
  assert(0 <= current_thread && current_thread < long(threads.size()));
  // llvm::outs() << "set current thread=" << current_thread << " and event \n";
  //update last_write
  if(current_event.is_write()){
    last_write[current_event.object] = current_event.iid;
    mem[current_event.object] = current_event.value;
    // llvm::outs() << "set mem and last write \n";
  }
  //update vpo when read is done
  if(current_event.is_read()) { 
    visible[current_event.object].execute_read(current_event.get_pid(), last_write[current_event.object]);
    current_event.value = mem[current_event.object];
    threads[current_thread][current_event.get_id()].value = current_event.value;
  }
  // [snj]: record current event as next in execution sequence
  execution_sequence.push_back(current_event.iid);
  prefix_state.push_back(current_state);
  // llvm::outs() << "pushed: " << current_state << " at prefix_state[" << execution_sequence.indexof(current_event.iid) << "]\n";
  prefix_idx++;
  threads[current_thread].awaiting_load_store = false; // [snj]: next event after current may not be load or store
  // llvm::outs() << "pref_idx=" << prefix_idx << "\n";
  // llvm::outs() << "pushed to execution, prefix_state[" << (prefix_state.size()-1) << "]=" << current_state << " \n";
}

void ViewEqTraceBuilder::analyse_unexplored_influenecers(IID<IPid> read_event) {
  std::unordered_set<IID<IPid>> ui = unexploredInfluencers(get_event(read_event), forbidden);

  for (auto it = ui.begin(); it != ui.end(); it++) {
    Event ui_event = threads[it->get_pid()][it->get_index()];
    update_leads(ui_event, forbidden);
  }
}

void ViewEqTraceBuilder::forward_analysis(Event event, SOPFormula<IID<IPid>>& forbidden) {
  std::unordered_set<IID<IPid>> ui = unexploredInfluencers(event, forbidden);
  Sequence uiseq(ui, &threads);

  // SOPFormula<IID<IPid>> fui_all = forbidden;
  // for (auto it = ui.begin(); it != ui.end(); it++) {
  //   fui_all || std::make_pair(event.iid, threads[(*it).get_pid()][(*it).get_index()].value);
  // }

  uiseq.push_front(event.iid);
  // std::vector<Lead> L{Lead(empty_sequence, uiseq, fui_all)};
  std::vector<Lead> L;

  std::unordered_map<IID<IPid>, int> valueEnv{{event.iid, mem[event.object]}};
  if (!(forbidden.check_evaluation(valueEnv) == RESULT::TRUE))
    L.push_back(Lead(empty_sequence, uiseq, forbidden, std::make_pair(event.iid, mem[event.object])));

  for (auto it = ui.begin(); it != ui.end(); it++) {
    if (get_event(*it).value == mem[event.object]) 
      continue;

    Sequence start = uiseq;
    start.erase((*it));
    start.push_front((*it));

    // SOPFormula<IID<IPid>> fui(std::make_pair(event.iid, mem[event.object]));
    // for (auto it2 = ui.begin(); it2 != ui.end(); it2++) {
    //   if (it2 != it)
    //     fui || std::make_pair(event.iid, get_event((*it2)).value);
    // }

    // fui || forbidden;
    // Lead l(empty_sequence, start, fui);
    Lead l(empty_sequence, start, forbidden, std::make_pair(event.iid, get_event(*it).value));
    L.push_back(l);
  }

  states[current_state].consistent_join(L); // add leads at current state
}

void ViewEqTraceBuilder::backward_analysis_read(Event event, SOPFormula<IID<IPid>>& forbidden, std::unordered_map<int, std::vector<Lead>>& L) {
  // std::unordered_set<IID<IPid>> ui = unexploredInfluencers(event, forbidden);
  std::unordered_set<IID<IPid>> ei = exploredInfluencers(event, forbidden);
  
  SOPFormula<IID<IPid>> fui = (std::make_pair(event.iid, mem[event.object]));
  // for (auto it = ui.begin(); it != ui.end(); it++) {
  //   fui || std::make_pair(event.iid, get_event((*it)).value);
  // }

  for (auto it = ei.begin(); it != ei.end(); it++) {
    if (get_event((*it)).value == mem[event.object])
      continue;

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

    SOPFormula<IID<IPid>> inF;
    for (auto it = states[event_index].alpha.start.begin(); it != states[event_index].alpha.start.end(); it++) {
      std::pair<bool, ProductTerm<IID<IPid>>> forbidden_term_of_event = states[event_index].alpha.forbidden.term_of_object((*it));
      if (forbidden_term_of_event.first) {
        inF || forbidden_term_of_event.second;
      }
    }
    
    inF || fui;
    L[event_index].push_back(Lead(states[event_index].alpha_sequence(), execution_sequence.backseq((*it), event.iid), inF, std::make_pair(event.iid, get_event(*it).value)));
  }
}

void ViewEqTraceBuilder::backward_analysis_write(Event event, SOPFormula<IID<IPid>>& forbidden, std::unordered_map<int, std::vector<Lead>>& L) {
  std::unordered_set<IID<IPid>> ew = exploredWitnesses(event, forbidden);

  for (auto it = ew.begin(); it != ew.end(); it++) {
    int it_val = get_event((*it)).value;
    if (it_val == event.value)
      continue;

    int event_index = prefix_state[execution_sequence.indexof((*it))];

    SOPFormula<IID<IPid>> inF;
    for (auto it = states[event_index].alpha.start.begin(); it != states[event_index].alpha.start.end(); it++) {
      std::pair<bool, ProductTerm<IID<IPid>>> forbidden_term_of_event = states[event_index].alpha.forbidden.term_of_object((*it));
      if (forbidden_term_of_event.first) {
        inF || forbidden_term_of_event.second;
      }
    }
    (inF || std::make_pair((*it),it_val));
    L[event_index].push_back(Lead(states[event_index].alpha_sequence(), execution_sequence.backseq((*it), event.iid), inF, std::make_pair((*it), event.value)));
  }
}

void ViewEqTraceBuilder::backward_analysis(Event event, SOPFormula<IID<IPid>>& forbidden) {
  std::unordered_map<int, std::vector<Lead>> L; // map of execution index -> leads to be added at state[execution index]

  if (event.is_read()) {
    backward_analysis_read(event, forbidden, L);
  }
  else if (event.is_write())  {
    backward_analysis_write(event, forbidden, L);
  }

  for (auto it = L.begin(); it != L.end(); it++) {
    states[it->first].consistent_join(it->second); // consistent join at respective execution prefix
  }
}

void ViewEqTraceBuilder::update_leads(Event event, SOPFormula<IID<IPid>>& forbidden) {
  assert(event.is_read() || event.is_write());

  if (event.is_read())
    forward_analysis(event, forbidden); // add leads at current state
  
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

int ViewEqTraceBuilder::current_value(unsigned obj) {
  return mem[obj];
}

void ViewEqTraceBuilder::update_done(IID<IPid> ev) {
  std::vector<int> remove_indices;
  int idx = 0;

  for (auto it = done.begin(); it != done.end(); it++, idx++) {
    if ((*it).head() != ev)
      remove_indices.push_back(idx);
  }

  for (auto it = remove_indices.end(); it != remove_indices.begin();) {
    it--;
    done.erase(done.begin() + (*it));
  }
}

void ViewEqTraceBuilder::update_forbidden(Lead *l) {
  llvm::outs() << "updating forbidden: original=" << forbidden.to_string();
  forbidden || l->forbidden;
  llvm::outs() << ", with lead=" << forbidden.to_string();

  for (auto it = states[current_state].leads.begin(); it != states[current_state].leads.end(); it++) {
    if (it->key.first == l->key.first && it->key.second != l->key.second) { // key of same event but diff value
      // if diff value of this event has been covered by some other lead then forbid it in current run
      llvm::outs() << ", add=(" << it->key.first.to_string() + "," + std::to_string(it->key.second) + ")";
      forbidden || it->key;
    }
  }
  llvm::outs() << " -- FINAL=" << forbidden.to_string();
}

void ViewEqTraceBuilder::metadata(const llvm::MDNode *md)
{
  // [rmnt]: Originally, they check whether dryrun is false before updating the current event's metadata and also maintain a last_md object inside TSOTraceBuilder. Since we don't use dryrun, we have omitted the checks and also last_md
  assert(current_event.md == 0);
  current_event.md = md;
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
    // llvm::outs() << "state[" << replay_state_prefix << "] has more leads? ";

    it->add_done(it->alpha_sequence());
    // llvm::outs() << "added" << it->alpha_sequence().to_string() << " to done of states[" << replay_state_prefix << "]\n";
    if (it->has_unexplored_leads()) { // found replay state
      // llvm::outs() << "LEADS:\n";
      // for (auto i = states[replay_state_prefix].leads.begin(); i != states[replay_state_prefix].leads.end(); i++) {
      //   llvm::outs() << i->to_string() <<"\n";
      // }
      // llvm::outs() << "\nDONE:\n";
      // for (auto i = states[replay_state_prefix].done.begin(); i != states[replay_state_prefix].done.end(); i++) {
      //   llvm::outs() << i->to_string() <<"\n";
      // }
      
      break;
    }

    // llvm::outs() << "NO\n";
    replay_state_prefix --;
  }

  // llvm::outs() << "returning " << replay_state_prefix << "\n";
  return replay_state_prefix;
}

bool ViewEqTraceBuilder::reset() {
  llvm::outs() << "\nRESETTING ";

  int replay_state_prefix = find_replay_state_prefix();
  if (replay_state_prefix < 0) {// no more leads to explore, model checking complete
    replay_point = -1;
    return false; 
  }

  int replay_execution_prefix = states[replay_state_prefix].sequence_prefix;
  llvm::outs() << "replaying till " << (replay_execution_prefix-1) << " (st:" << replay_state_prefix << ")\n" ;
  execution_sequence.erase(execution_sequence.begin() + replay_execution_prefix + 1, execution_sequence.end());
  prefix_state.erase(prefix_state.begin() + replay_execution_prefix + 1, prefix_state.end());
  states.erase(states.begin() + replay_state_prefix + 1, states.end());
  // llvm::outs() << "trimmed ex seq size=" << execution_sequence.size() << ", prefix_state size=" << prefix_state.size() << ", states size=" << states.size() << "\n";

  to_explore.clear();
  // CPS = CPidSystem();
  
  threads.clear();
  threads.push_back(Thread(CPid(), -1));
  // mutexes.clear();
  // cond_vars.clear();
  
  mem.clear();
  visible.clear();
  last_write.clear();

  done = states[replay_state_prefix].done;
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
  llvm::outs() << "refuse schedule\n";
  execution_sequence.pop_back();
  prefix_state.pop_back();
  prefix_idx--;
  mark_unavailable(current_thread);
}

bool ViewEqTraceBuilder::is_replaying() const {
  return prefix_idx < replay_point;
}

bool ViewEqTraceBuilder::at_replay_point() {
  return prefix_idx == replay_point;
}

void ViewEqTraceBuilder::cancel_replay() {llvm::outs() << "REPLAY CANCELLED\n";  replay_point = -1;} //[snj]: needed?

bool ViewEqTraceBuilder::full_memory_conflict() {return false;} //[snj]: TODO

bool ViewEqTraceBuilder::spawn()
{
  Event event(SymEv::Spawn(threads.size()));
  event.make_spawn();

  // [snj]: create event in thread that is spawning a new event
  current_event = event;
  current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].events.size());
  threads[current_thread].push_back(current_event);

  // [snj]: setup new program thread
  IPid parent_ipid = current_event.iid.get_pid();
  CPid child_cpid = CPS.spawn(threads[parent_ipid].cpid);
  /* Spwan event of thread is dummy */
  threads.push_back(Thread(child_cpid, -42));

  for( auto it = visible.begin(); it != visible.end(); it++){
    (it->second).add_thread();
  }
  return true;
}

bool ViewEqTraceBuilder::join(int tgt_proc) {
  Event event(SymEv::Join(tgt_proc));
  event.make_join();

  current_event = event;
  current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].events.size());
  threads[current_thread].push_back(current_event);

  return true;
}

bool ViewEqTraceBuilder::load(const SymAddrSize &ml) {
  Event event(SymEv::Load(ml));
  event.make_read();
  event.value = mem[event.object];
  current_event = event;

  if (current_event.is_global()) {
    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].events.size());
    threads[current_thread].push_back(current_event);
    threads[current_thread].awaiting_load_store = true;

    assert(!is_enabled(current_thread)); // [snj]: only 1 event of each thread is enabled
    Enabled.push_back(current_event.iid);
  }
  else {
    threads[current_thread].pop_back();
    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].events.size());
    threads[current_thread].push_back(current_event);
    execution_sequence.pop_back();
    execution_sequence.push_back(current_event.iid);
  }


  return true;
}

bool ViewEqTraceBuilder::store(const SymData &ml) {
  // [snj]: visitStoreInst in Execution.cpp lands in atomic_store not here
  Event event(SymEv::Store(ml));
  event.make_write();
  current_event = event;

  if (current_event.is_global()) {
    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].events.size());
    threads[current_thread].push_back(current_event);
    threads[current_thread].awaiting_load_store = true;

    assert(!is_enabled(current_thread)); // [snj]: only 1 event of each thread is enabled
    Enabled.push_back(current_event.iid);
    if(current_thread == 0){
      Visible vis(current_event.iid);
      visible.insert({current_event.object, vis});
    }
    else
      visible[current_event.object].add_enabled_write(current_event.iid);
  }
  else {
    threads[current_thread].pop_back();
    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].events.size());
    threads[current_thread].push_back(current_event);
    execution_sequence.pop_back();
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
    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].events.size());
    threads[current_thread].push_back(current_event);
    threads[current_thread].awaiting_load_store = true;

    assert(!is_enabled(current_thread)); // [snj]: only 1 event of each thread is enabled
    Enabled.push_back(current_event.iid);

    if(current_thread == 0){
      Visible vis( current_event.iid);
      visible.insert({current_event.object, vis});
    }
    else
      visible[current_event.object].add_enabled_write(current_event.iid);
  }
  else {
    threads[current_thread].pop_back();
    current_event.iid = IID<IPid>(IPid(current_thread), threads[current_thread].events.size());
    threads[current_thread].push_back(current_event);
    execution_sequence.pop_back();
    execution_sequence.push_back(current_event.iid);
  }

  return true;
}

bool ViewEqTraceBuilder::fence() {
  // [snj]: invoked at pthread create
  // llvm::outs() << "[snj]: fence being invoked!!";
  return true;
}

Trace* ViewEqTraceBuilder::get_trace() const {Trace *t = NULL; return t;} //[snj]: TODO

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
    if (event.symEvent.size() < 1) {
      llvm::outs() << "[" << i << "]: --\n";
      continue;
    }
    if (event.sym_event().addr().addr.block.is_stack()) {
      llvm::outs() << "[" << i << "]: Stack Operation\n";
      continue;
    }
    if (event.sym_event().addr().addr.block.is_heap()) {
      llvm::outs() << "[" << i << "]: Heap Operation\n";
      continue;
    }
    llvm::outs() << "[" << i << "]: " << event.to_string() << "\n";
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
  object = sym_event().addr().addr.block.get_no();
}

void ViewEqTraceBuilder::Event::make_write() {
  type = WRITE;
  uint8_t *valptr = sym_event()._written.get();
  value = (*valptr);
  object = sym_event().addr().addr.block.get_no();
}

bool ViewEqTraceBuilder::Event::same_object(Event event) {
  return (object == event.object);
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
    case WRITE: return "Write";
    case READ: return "Read";
  }
  return "";
}

std::string ViewEqTraceBuilder::Event::to_string() const {
  // return (sym_event().to_string() + " *** (" + type_to_string() + "(" + std::to_string(object) + "," + std::to_string(value) + "))");
  return ("[" + std::to_string(get_pid()) + ":" + std::to_string(get_id()) + "] (" + type_to_string() + "(" + std::to_string(object) + "," + std::to_string(value) + "))");
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::prefix(IID<IPid> ev) {
  assert(this->has(ev));
  sequence_iterator it = find(ev);
  std::vector<IID<IPid>> pre(begin(), it);
  Sequence sprefix(pre, threads);
  assert(!sprefix.has(ev));
  return sprefix;
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::suffix(IID<IPid> ev) {
  assert(this->has(ev));
  sequence_iterator it = find(ev);
  std::vector<IID<IPid>> suf(it+1, end());
  Sequence ssuffix(suf, threads);
  return ssuffix;
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::suffix(ViewEqTraceBuilder::Sequence &seq) {
  sequence_iterator it = find(*(seq.end()-1));
  std::vector<IID<IPid>> suf(it+1, end());
  Sequence ssuffix(suf, threads);
  return ssuffix;
}

bool ViewEqTraceBuilder::Sequence::conflicting(ViewEqTraceBuilder::Sequence &other_seq){
  for(int i = 0; i < events.size(); i++){
    for(int j = i + 1; j < events.size() ; j++){
      if(other_seq.has(events[i]) && other_seq.has(events[j])){
        sequence_iterator it1 = other_seq.find(events[i]);
        sequence_iterator it2 = other_seq.find(events[j]);
        if ((it1 - it2) > 0) return true;

      }
    }
  }
  return false;
}

bool ViewEqTraceBuilder::Sequence::isPrefix(ViewEqTraceBuilder::Sequence &seq){
  for(int i = 0; i<events.size(); i++){
    if(events[i] != seq.events[i]) return false;
  }
  return true;
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::commonPrefix(ViewEqTraceBuilder::Sequence &seq){
  int i = 0;
  for( ; i<events.size(); i++){
    if(events[i] != seq.events[i]) break;
  }
  std::vector<IID<IPid>> cPre(events.begin(), events.begin()+i );
  Sequence comPrefix(cPre, threads);
  assert(comPrefix.isPrefix(*this) && comPrefix.isPrefix(seq));
  return comPrefix;
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::poPrefix(IID<IPid> ev, sequence_iterator begin, sequence_iterator end){
  assert(has(ev));
  sequence_iterator it;
  std::vector<IID<IPid>> poPre;

  for(it = begin; it != end && it != events.end(); it++){
    if(it->get_pid() == ev.get_pid()) {
      Event ite = threads->at(it->get_pid())[it->get_index()];
      if (ite.is_global())
        poPre.push_back(*it);
    }
  }

  Sequence spoPrefix(poPre, threads);
  return spoPrefix;
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::backseq(IID<IPid> e1, IID<IPid> e2){
  assert(this->has(e1) && this->has(e2));
  ViewEqTraceBuilder::Sequence po_pre = poPrefix(e2, find(e1)+1, find(e2));

  if (e1.get_pid() == 0) { // event from thread zero ie init write event
    po_pre.push_back(e2);
    return po_pre; // init event is not added to backseq
  }
  
  Event event1 = threads->at(e1.get_pid())[e1.get_index()];
  Event event2 = threads->at(e2.get_pid())[e2.get_index()];

  // [snj]: po prefix followed by write of {e1,e2} followed by read of {e1,e2}
  if(event1.is_write()) po_pre.push_back(e1);
  if(event2.is_write()) po_pre.push_back(e2);
  if(event1.is_read()) po_pre.push_back(e1);
  if(event2.is_read()) po_pre.push_back(e2);

  return po_pre;
}


void  ViewEqTraceBuilder::Sequence::project(std::tuple<ViewEqTraceBuilder::Sequence, ViewEqTraceBuilder::Sequence, ViewEqTraceBuilder::Sequence> &triple,
ViewEqTraceBuilder::Sequence &seq1, ViewEqTraceBuilder::Sequence &seq2, ViewEqTraceBuilder::Sequence &seq3) {
  seq1 = std::get<0>(triple);
  seq2 = std::get<1>(triple);
  seq3 = std::get<2>(triple);
}

std::tuple<ViewEqTraceBuilder::Sequence, ViewEqTraceBuilder::Sequence, ViewEqTraceBuilder::Sequence>
ViewEqTraceBuilder::Sequence::join(Sequence &primary, Sequence &other, IID<IPid> delim, Sequence &joined)
{
  typedef std::tuple<Sequence, Sequence, Sequence> return_type;
  // std::vector<Thread> threads = *threads;
  if (primary.empty()) {
    joined.concatenate(other);
    primary.clear();
    other.clear();
    return std::make_tuple(primary, other, joined);
  }

  IID<IPid> e = primary.head();
  Event ev = threads->at(e.get_pid())[e.get_index()];

  assert(ev.is_global()); // is a global read write

  if (e == delim) {
    if (joined.last() == e) {
      primary = primary.tail();
      return std::make_tuple(primary, other, joined);
    }
    else {
      primary = primary.tail();
      joined.push_back(e);
      return std::make_tuple(primary, other, joined);
    }
  }

  if (ev.is_write()) { // algo 6-17
    if (!other.has(e)) { // e not in both primary and other [algo 6-12]
      IID<IPid> er;
      sequence_iterator it;
      for (it = other.events.begin(); it != other.events.end(); it++) {
        er = *it;
        Event event_er = threads->at(er.get_pid())[er.get_index()];
        if (!event_er.is_read()) continue;
        if (primary.has(er)) continue;
        if (ev.same_object(event_er)) break; // found a read er that is not in primary s.t. obj(e) == obj(er)
      }

      if (it != other.events.end()) { // there exists a read er that is not in primary s.t. obj(e) == obj(er)
        return_type triple = join(other, primary, er, joined);
        project(triple, other, primary, joined);

        if (joined.last() != er) joined.push_back(er); // [snj]: TODO diff from algo line 9 -- check if fine
        if (joined.last() != e) joined.push_back(e); // [snj]: TODO diff from algo line 10 -- check if fine
      }
      else { // there does not exist a read er that is not in primary s.t. obj(e) == obj(er)
        joined.push_back(e);
        primary.pop_front();
      }
    }
    else { // e in both primary and other [algo 13-17]
      return_type triple = join(other, primary, e, joined);
      project(triple, other, primary, joined);
      if (joined.last() != e) joined.push_back(e);
      if (primary.head() == e) primary = primary.tail(); // [snj]: TODO is the check needed?
    }
  }

  else if (ev.is_read()) { // algo 18-21
    if (!other.has(e)) { // algo 18
      joined.push_back(e);
      primary.pop_front();
    }
    else { // algo 19-21
      primary.pop_front();
      return_type triple = join(other, primary, e, joined);
      project(triple, other, primary, joined);
      if (!joined.has(e)) joined.push_back(e);
    }
  }

  return join(primary, other, delim, joined);
}

// [snj]: TODO function not tested
ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::cmerge(Sequence &other_seq) {
  Sequence current_seq = *this;

  assert(!current_seq.empty());
  if (other_seq.empty()) return current_seq;

  if (!current_seq.hasRWpairs(other_seq)) {
    other_seq.concatenate(current_seq);
    return other_seq;
  }
  
  sequence_iterator it;
  for (it = current_seq.begin(); it != current_seq.end(); it++) {
    if (other_seq.has(*it))
      break;
  }
  if (it == current_seq.end()) { // no common event
    current_seq.concatenate(other_seq);
    return current_seq;
  }

  IID<IPid> dummy;
  Sequence joined;
  std::tuple<Sequence, Sequence, Sequence> triple = join(current_seq, other_seq, dummy, joined);

  assert(std::get<0>(triple).size() == 0);
  assert(std::get<1>(triple).size() == 0);

  return std::get<2>(triple);
}

// [snj]: TODO function not tested
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

// [snj]: TODO function not tested
bool ViewEqTraceBuilder::Sequence::conflits_with(Sequence &other) {
  Sequence current = (*this);
  for (sequence_iterator it1 = current.begin(); it1 != current.end(); it1++) {
    sequence_iterator it2 = other.find((*it1));
    if (it2 == other.end()) continue; // event not in other

    for (sequence_iterator it12 = current.begin(); it12 != it1; it12++) {
      for (sequence_iterator it22 = it2+1; it22 != other.end(); it22++)
        if ((*it12) == (*it22))
          return true;
    }
  }

  return false;
}

std::string ViewEqTraceBuilder::Sequence::to_string() {
  if (events.empty()) return "<>";

  std::string s = "<";
  for (auto ev : events) {
    if (ev == last()) break;
    s = s + "(" + std::to_string(ev.get_pid()) + ":" + std::to_string(ev.get_index()) +"), ";
  }

  s = s + "(" + std::to_string(last().get_pid()) + ":" + std::to_string(last().get_index()) + ")>";
  return s;
}

std::unordered_set<IID<IPid>> ViewEqTraceBuilder::unexploredInfluencers(Event er, SOPFormula<IID<IPid>>& f){
  assert(er.is_read());
  std::unordered_set<IID<IPid>> ui;
  std::unordered_set<int> values,forbidden_values;

  for(int i = 0; i < Enabled.size(); i++){
    Event e = get_event(Enabled[i]);
    
    //[nau]:not a write event or not same object or already taken value or already checked forbidden value
    if(!e.is_write() || !er.same_object(e) || values.find(e.value) != values.end() || forbidden_values.find(e.value) != forbidden_values.end()) continue;

    //check if value forbidden for er
    std::unordered_map<IID<IPid>, int> valueEnv{{er.iid, e.value}};
    if(f.check_evaluation(valueEnv)== RESULT::TRUE) {
      forbidden_values.insert(e.value);
      continue;
    }
    
    ui.insert(e.iid);
    values.insert(e.value);
  }

  return ui;
}

std::unordered_set<IID<IPid>> ViewEqTraceBuilder::exploredInfluencers(Event er, SOPFormula<IID<IPid>> &f){
  std::unordered_set<IID<IPid>> ei;
  std::unordered_set<int> values, forbidden_values;
  
  unsigned o_id = er.object;
  int pid = er.get_pid();
  
  assert(pid > 0);
  auto it = visible.find(o_id);
  assert(it!= visible.end());
  assert(visible[o_id].mpo[0].size() == 1); //only the init event in first row
  assert(visible[o_id].mpo.size() == visible[o_id].visible_start.size() + 1);
  
  //[nau]: check if init event is visible to er
  if(visible[o_id].check_init_visible(pid)) {
    IID<IPid> initid = visible[o_id].mpo[0][0];
    Event init = get_event(initid);
    assert(init.value == 0);
    std::unordered_map<IID<IPid>, int> valueEnv{{initid, init.value}};
    if(f.check_evaluation(valueEnv) != RESULT::TRUE ) ei.insert(visible[o_id].mpo[0][0]);
    else forbidden_values.insert(init.value);
  }
  
  //for writes other than init
  for(int i = 0; i < visible[o_id].visible_start[pid - 1].size(); i++){
      int j = visible[o_id].visible_start[pid - 1][i];
      for( int k = j; k < visible[o_id].mpo[i + 1].size() + 1; k++){
          if(k == 0) continue;
          IID<IPid> e_id = visible[o_id].mpo[i + 1][k - 1];

          Event e = get_event(e_id);
          //repeated value || already checked forbidden value || unexplored event
          if(values.find(e.value) != values.end() || forbidden_values.find(e.value) != forbidden_values.end() || find(Enabled.begin(), Enabled.end(), e_id) != Enabled.end() ) continue;

          std::unordered_map<IID<IPid>, int> valueEnv{{e_id, e.value}};

          if(f.check_evaluation(valueEnv) == RESULT::TRUE ) forbidden_values.insert(e.value);
          else{
            ei.insert(e_id);
            values.insert(e.value);
          }
      }
  }
  
  return ei;
}

std::unordered_set<IID<IPid>> ViewEqTraceBuilder::exploredWitnesses(Event ew, SOPFormula<IID<IPid>> &f){
  assert(ew.is_write());
  //assert(find(Enabled.begin(), Enabled.end(), ew.iid) != Enabled.end() );
  std::unordered_set<IID<IPid>> explored_witnesses;
  //llvm::outs()<<"in ew\n";
  for(int i = 0; i < execution_sequence.size(); i++){
    IID<IPid> er = execution_sequence[i];
    Event e = get_event(er);
    //llvm::outs()<<"in ew1\n";
    //[nau]:not a read or not the same object or poprefix of the write event or ew forbidden for this read
    if(! e.is_read() || ! e.same_object(ew) || e.get_pid() == ew.get_pid() ) continue;

    std::unordered_map<IID<IPid>, int> valueEnv{{er, ew.value}};
    if(f.check_evaluation(valueEnv) != RESULT::TRUE)
      explored_witnesses.insert(er);
  }
  //llvm::outs()<<"in ew2\n";
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
  return ("(" + constraint.to_string() + ", " + start.to_string() + ", " + 
    forbidden.to_string() + ", (" + key.first.to_string() + "," + std::to_string(key.second) + ")");
}

void ViewEqTraceBuilder::State::add_done(Sequence d) {  
  for (auto it = done.begin(); it != done.end(); it++) {
    if (d == (*it)) {
      return; // sequence alrready added
    }

    if ((*it).size() > d.size()) {
      done.insert(it, d); // sorted by length
      return;
    }
  }

  done.push_back(d);
}

bool ViewEqTraceBuilder::State::is_done(Sequence seq) {
  for (auto it = done.begin(); it != done.end(); it++) {
    if (seq == (*it))
      return true;

    if ((*it).size() > seq.size())
      return false;
  }

  return false;
}

bool ViewEqTraceBuilder::State::has_unexplored_leads() {
  for (auto itl = leads.begin(); itl != leads.end(); itl++) {
    bool is_unexplored = true;
    
    for (auto itd = done.begin(); itd != done.end(); itd++) {
      if (itd->isPrefix(itl->merged_sequence)) {
        is_unexplored = false;
        break; // itl is not unexplored
      }
    }

    if (is_unexplored) return true;
  }

  return false;
}

std::vector<ViewEqTraceBuilder::Lead> ViewEqTraceBuilder::State::unexplored_leads() {
  std::vector<Lead> ul;

  for (auto itl = leads.begin(); itl != leads.end(); itl++) {
    bool is_unexplored = true;
    
    for (auto itd = done.begin(); itd != done.end(); itd++) {
      if (itd->isPrefix(itl->merged_sequence)) {
        is_unexplored = false;
        break; // itl is not unexplored
      }
    }

    if (is_unexplored) ul.push_back((*itl));
  }

  llvm::outs() << "returning unexp leads\n";
  return ul;
}

ViewEqTraceBuilder::Lead ViewEqTraceBuilder::State::next_unexplored_lead() {
  assert(has_unexplored_leads());

  for (auto itl = leads.begin(); itl != leads.end(); itl++) {
    bool is_unexplored = true;
    
    for (auto itd = done.begin(); itd != done.end(); itd++) {
      if (itd->isPrefix(itl->merged_sequence)) {
        is_unexplored = false;
        break; // itl is not unexplored
      }
    }

    if (is_unexplored) return((*itl));
  }

  assert(false);
  Lead dummy;
  return dummy;
}

void ViewEqTraceBuilder::State::consistent_join(Lead& l) {
  std::vector<Lead> L;
  L.push_back(l);
  consistent_join(L);
}

// [snj]: TODO dummy implementation
void ViewEqTraceBuilder::State::consistent_join(std::vector<Lead>& L) {
  for (auto it = L.begin(); it != L.end(); it++) {
    leads.push_back((*it));
  }
}

std::ostream & ViewEqTraceBuilder::State::print_leads(std::ostream &os) {
  std::string s = "State[" + std::to_string(sequence_prefix) + "] leads:\n";
  for (auto l : leads) {
    s = s + l.to_string() + ",\n";
  }

  return os << s;
}
