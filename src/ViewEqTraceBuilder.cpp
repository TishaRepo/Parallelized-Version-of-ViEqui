#include "ViewEqTraceBuilder.h"

ViewEqTraceBuilder::ViewEqTraceBuilder(const Configuration &conf) : TSOPSOTraceBuilder(conf)
{
  threads.push_back(Thread(CPid(), -1));
  prefix_idx = -1;
  round  = 3;
}

ViewEqTraceBuilder::~ViewEqTraceBuilder() {}

/*
    [rmnt] : This function is meant to replace the schedule function used in Interpreter::run in Execution.cpp. It searches for an available real thread (we are storing thread numbers at even indices since some backend functions rely on that). Once it finds one we push its index (prefix_idx is a global index keeping track of the number of events we have executed) to the respective thread's event indices. We also create its event object. Now here we have some work left. What they do is insert this object into prefix. We have to figure out where the symbolic event member of the Event object is being filled up (its empty on initialisation) as that will get us to how they find the type of event. Also, we have to implement certain functions like mark_available, mark_unavailable, reset etc which are required for this to replace the interface of the current schedule function. There are also other functions like metadata() and fence() that we are not sure whether our implementation would need.
             We needed to keep the signature same to maintain legacy, even though we don't use either auxilliary threads, or dryruns, or alternate events in our algorithm.
*/
bool ViewEqTraceBuilder::schedule(int *proc, int *aux, int *alt, bool *dryrun)
{
  // snj: For compatibility with existing design
  *alt = 0;
  *dryrun = false;
  // // //

  // [rmnt] : When creating a new event, initialize sym_idx to 0
  sym_idx = 0;

  const unsigned size = threads.size();
  unsigned i;

  for (i = 0; i < size; i ++)
  { 
    if (threads[i].available)// && (conf.max_search_depth < 0 || threads[i].event_indices.size() < conf.max_search_depth))
    {
      threads[i].event_indices.push_back(++prefix_idx);
      Event event = Event(IID<IPid>(IPid(i), threads[i].event_indices.size()));
      execution_sequence.push_back(event);
      *proc = i;
      *aux = -1; // [snj]: required for maintaining compatibility with run() in Execution.cpp
      return true;
    }
  }

  debug_print();

  return false;
}

void ViewEqTraceBuilder::mark_available(int proc, int aux)
{
  threads[proc].available = true;
}

void ViewEqTraceBuilder::mark_unavailable(int proc, int aux)
{
  threads[proc].available = false;
}

void ViewEqTraceBuilder::metadata(const llvm::MDNode *md)
{
  // [rmnt]: Originally, they check whether dryrun is false before updating the current event's metadata and also maintain a last_md object inside TSOTraceBuilder. Since we don't use dryrun, we have omitted the checks and also last_md
  assert(curev().md == 0);
  curev().md = md;
}

bool ViewEqTraceBuilder::spawn()
{
  IPid parent_ipid = curev().iid.get_pid();
  CPid child_cpid = CPS.spawn(threads[parent_ipid].cpid);
  /* TODO: First event of thread happens before parents spawn */
  threads.push_back(Thread(child_cpid, prefix_idx));
  //[snj]: TODO remove this statement
  // threads.push_back(Thread(CPS.new_aux(child_cpid), prefix_idx));
  return record_symbolic(SymEv::Spawn(threads.size())); // [snj]: chenged from Spawn(threads.size() / 2 - 1)
}

bool ViewEqTraceBuilder::store_pre(const SymData &sd)
{
  if (!record_symbolic(SymEv::Store(sd)))
    return false;
  return true;
}

bool ViewEqTraceBuilder::store_post(const SymData &sd)
{
  const SymAddrSize &ml = sd.get_ref();
  IPid ipid = curev().iid.get_pid();
  
  // [snj]: TODO remove this part eventually
  // bool is_update = ipid % 2;
  // if (is_update)
  // {
  //   llvm::outs() << "snj: FIX NEEDED: aux event is being accessed";
  //   return true;
  // }
  // remove till here

  //   IPid uipid = ipid;                        // ID of the thread changing the memory
  //   IPid tipid = is_update ? ipid - 1 : ipid; // ID of the (real) thread that issued the store

  // [snj]: I think seen_accesses is not needed
  //   VecSet<int> seen_accesses;

  /* See previous updates reads to ml */
  for (SymAddr b : ml)
  {
    ByteInfo &bi = mem[b];
    int lu = bi.last_update;
    assert(lu < int(execution_sequence.len()));
    // [snj]: I think seen_accesses is not needed
    // if (0 <= lu)
    // {
    //   IPid lu_ipid = execution_sequence[lu].iid.get_pid();
    //   if (lu_ipid != ipid)
    //   {
    //     seen_accesses.insert(bi.last_update);
    //   }
    // }
    // for (int i : bi.last_read)
    // {
    //   if (0 <= i && execution_sequence[i].iid.get_pid() != ipid)
    //     seen_accesses.insert(i);
    // }

    /* Register in memory */
    bi.last_update = prefix_idx;
    bi.last_update_ml = ml;
  }

  // seen_accesses.insert(last_full_memory_conflict);

  // see_events(seen_accesses); [snj]: updates race information

  return true;
}

bool ViewEqTraceBuilder::load_pre(const SymAddrSize &ml)
{
  if (!record_symbolic(SymEv::Load(ml)))
    return false;
  return true;
}

bool ViewEqTraceBuilder::load_post(const SymAddrSize &ml)
{
  IPid ipid = curev().iid.get_pid();

  /* Load from memory */
  VecSet<int> seen_accesses;

  /* See all updates to the read bytes. */
  for (SymAddr b : ml)
  {
    //load_post(mem[b]); // [snj]: TODO why the recursive call also its a overloaded diff function

    /* Register load in memory */
    // mem[b].last_read[ipid] = prefix_idx; //[snj]: I don't think its needed
  }

  //   seen_accesses.insert(last_full_memory_conflict);

  //   see_events(seen_accesses);

  return true;
}

// [rmnt]: TODO : Implement the notion of replay and associated functions
bool ViewEqTraceBuilder::record_symbolic(SymEv event)
{
  if (!replay)
  {
    assert(sym_idx == curev().symEvent.size());
    /* New event */
    curev().symEvent.push_back(event);
    sym_idx++;
  }
  else
  {
    /* Replay. SymEv::set() asserts that this is the same event as last time. */
    assert(sym_idx < curev().symEvent.size());
    SymEv &last = curev().symEvent[sym_idx++];
    if (!last.is_compatible_with(event))
    {
      auto pid_str = [this](IPid p) { return threads[p].cpid.to_string(); };
      nondeterminism_error("Event with effect " + last.to_string(pid_str) + " became " + event.to_string(pid_str) + " when replayed");
      return false;
    }
    last = event;
  }
  return true;
}

bool ViewEqTraceBuilder::reset() {
  if (round == 1) return false;
  llvm::outs() << "reset round " << round << "\n";
  round--;
  execution_sequence.clear();
  
  // CPS = CPidSystem();
  threads.clear();
  threads.push_back(Thread(CPid(), -1));
  // mutexes.clear();
  // cond_vars.clear();
  mem.clear();
  // last_full_memory_conflict = -1;
  prefix_idx = -1;
  // dryrun = false;
  replay = true;
  // dry_sleepers = 0;
  last_md = 0;
  
  return true;
}

IID<CPid> ViewEqTraceBuilder::get_iid() const{
  IPid pid = curev().iid.get_pid();
  int idx = curev().iid.get_index();
  return IID<CPid>(threads[pid].cpid, idx);
}

void ViewEqTraceBuilder::refuse_schedule() {
  IPid last_pid = execution_sequence.last().iid.get_pid();
  execution_sequence.pop_back();
  threads[last_pid].event_indices.pop_back();
  --prefix_idx;
  mark_unavailable(last_pid);
} 

bool ViewEqTraceBuilder::is_replaying() const {
  return prefix_idx < replay_point;
}

void ViewEqTraceBuilder::cancel_replay() {replay = false;} //[snj]: needed?

bool ViewEqTraceBuilder::full_memory_conflict() {return false;} //[snj]: TODO
bool ViewEqTraceBuilder::join(int tgt_proc) {
  if (!record_symbolic(SymEv::Join(tgt_proc)))
    return false;

  return true;
}

bool ViewEqTraceBuilder::load(const SymAddrSize &ml) {
  bool pre_ret = load_pre(ml);
  return load_post(ml) && pre_ret;
}

bool ViewEqTraceBuilder::store(const SymData &ml) {
  // [snj]: visitStoreInst in Execution.cpp lands in atomic_store not her
  bool pre_ret = store_pre(ml);
  return store_post(ml) && pre_ret;
}

bool ViewEqTraceBuilder::atomic_store(const SymData &ml) {
  // [snj]: visitStoreInst in Execution.cpp lands here not in store
  bool pre_ret = store_pre(ml);
  return store_post(ml) && pre_ret;
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
    if (execution_sequence[i].symEvent.size() < 1) {
      llvm::outs() << "[" << i << "]: --\n";
      continue;
    }
    if (execution_sequence[i].sym_event().addr().addr.block.is_stack()) {
      llvm::outs() << "[" << i << "]: Stack Operation\n";
      continue;
    }
    if (execution_sequence[i].sym_event().addr().addr.block.is_heap()) {
      llvm::outs() << "[" << i << "]: Heap Operation\n";
      continue;
    }
    llvm::outs() << "[" << i << "]: " << execution_sequence[i].to_string() << "\n";
  }
} //[snj]: TODO

bool ViewEqTraceBuilder::compare_exchange(const SymData &sd, const SymData::block_type expected, bool success) 
                                                    {llvm::outs() << "[snj]: cmp_exch being invoked!!"; assert(false); return false;}
bool ViewEqTraceBuilder::sleepset_is_empty() const{llvm::outs() << "[snj]: sleepset_is_empty being invoked!!"; assert(false); return true;}
bool ViewEqTraceBuilder::check_for_cycles(){llvm::outs() << "[snj]: check_for_cycles being invoked!!"; assert(false); return false;}
bool ViewEqTraceBuilder::mutex_lock(const SymAddrSize &ml){llvm::outs() << "[snj]: mutex_lock being invoked!!"; assert(false); return true;}
bool ViewEqTraceBuilder::mutex_lock_fail(const SymAddrSize &ml){llvm::outs() << "[snj]: mutex_lock_fail being invoked!!"; assert(false); return false;}
bool ViewEqTraceBuilder::mutex_trylock(const SymAddrSize &ml){llvm::outs() << "[snj]: mutex_trylock being invoked!!"; assert(false); return false;}
bool ViewEqTraceBuilder::mutex_unlock(const SymAddrSize &ml){llvm::outs() << "[snj]: mutex_unlock being invoked!!"; assert(false); return false;}
bool ViewEqTraceBuilder::mutex_init(const SymAddrSize &ml){llvm::outs() << "[snj]: mutex_init being invoked!!"; assert(false); return false;}
bool ViewEqTraceBuilder::mutex_destroy(const SymAddrSize &ml){llvm::outs() << "[snj]: mutex_ destroy being invoked!!"; assert(false); return false;}
bool ViewEqTraceBuilder::cond_init(const SymAddrSize &ml){llvm::outs() << "[snj]: cond_init being invoked!!"; assert(false); return false;}
bool ViewEqTraceBuilder::cond_signal(const SymAddrSize &ml){llvm::outs() << "[snj]: cond_signal being invoked!!"; assert(false); return false;}
bool ViewEqTraceBuilder::cond_broadcast(const SymAddrSize &ml){llvm::outs() << "[snj]: cond_broadcast being invoked!!"; assert(false); return false;}
bool ViewEqTraceBuilder::cond_wait(const SymAddrSize &cond_ml,
                        const SymAddrSize &mutex_ml){llvm::outs() << "[snj]: cond_wait being invoked!!"; assert(false); return false;}
bool ViewEqTraceBuilder::cond_awake(const SymAddrSize &cond_ml,
                        const SymAddrSize &mutex_ml){llvm::outs() << "[snj]: cond_awake being invoked!!"; assert(false); return false;}
int ViewEqTraceBuilder::cond_destroy(const SymAddrSize &ml){llvm::outs() << "[snj]: cond_destroy being invoked!!"; assert(false); return false;}
bool ViewEqTraceBuilder::register_alternatives(int alt_count){llvm::outs() << "[snj]: register_alternatives being invoked!!"; assert(false); return false;}

bool ViewEqTraceBuilder::Event::same_object(ViewEqTraceBuilder::Event event) {
  if (symEvent.size() < 1) return false;
  return (sym_event().addr().addr.block.get_no() == event.sym_event().addr().addr.block.get_no());
}

bool ViewEqTraceBuilder::Event::operator==(ViewEqTraceBuilder::Event event) {
  if (value != event.value)
    return false;

  return (symEvent == event.symEvent);
}

bool ViewEqTraceBuilder::Event::operator!=(ViewEqTraceBuilder::Event event) {
  return !(*this == event);
}

std::string ViewEqTraceBuilder::Event::to_string() const {
  return (sym_event().to_string() + "(" + std::to_string(value) + ")");
}

void  ViewEqTraceBuilder::Sequence::project(std::tuple<ViewEqTraceBuilder::Sequence, ViewEqTraceBuilder::Sequence, ViewEqTraceBuilder::Sequence> &triple, 
ViewEqTraceBuilder::Sequence &seq1, ViewEqTraceBuilder::Sequence &seq2, ViewEqTraceBuilder::Sequence &seq3) {
  seq1 = std::get<0>(triple);
  seq2 = std::get<1>(triple);
  seq3 = std::get<2>(triple);
}

std::tuple<ViewEqTraceBuilder::Sequence, ViewEqTraceBuilder::Sequence, ViewEqTraceBuilder::Sequence> ViewEqTraceBuilder::Sequence::join(
  ViewEqTraceBuilder::Sequence &primary, ViewEqTraceBuilder::Sequence &other, 
  ViewEqTraceBuilder::Event delim, ViewEqTraceBuilder::Sequence &joined) 
{
  typedef std::tuple<ViewEqTraceBuilder::Sequence, ViewEqTraceBuilder::Sequence, ViewEqTraceBuilder::Sequence> return_type;

  if (primary.empty()) {
    joined.concatenate(other);
    primary.clear();
    other.clear();
    return std::make_tuple(primary, other, joined);
  }

    ViewEqTraceBuilder::Event e = primary.head();

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

    if (e.is_write()) { // algo 6-17
      if (!other.has(e)) { // e not in both primary and other [algo 6-12]
        Event er;
        ViewEqTraceBuilder::sequence_iterator it;
        for (it = other.events.begin(); it != other.events.end(); it++) {
          er = *it;
          if (!er.is_read()) continue;
          if (primary.has(er)) continue;
          if (e.same_object(er)) break; // found a read er that is not in primary s.t. obj(e) == obj(er)
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

    if (e.is_read()) { // algo 18-21
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

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::cmerge(ViewEqTraceBuilder::Sequence &other_seq) {
  Sequence current_seq = *this;

  if (current_seq.hasRWpairs(other_seq)) {
    other_seq.concatenate(current_seq);
    return other_seq;
  }

  sequence_iterator it;
  for (it = current_seq.events.begin(); it != (*this).events.end(); it++) {
    if (other_seq.has(*it))
      break;
  }
  if (it == current_seq.events.end()) { // no common event
    current_seq.concatenate(other_seq);
    return current_seq;
  }

  Event dummy;
  Sequence joined;
  std::tuple<Sequence, Sequence, Sequence> triple = join(current_seq, other_seq, dummy, joined);
  
  assert(std::get<0>(triple).size() == 0);
  assert(std::get<1>(triple).size() == 0);

  return std::get<2>(triple);
}

bool ViewEqTraceBuilder::Sequence::hasRWpairs(Sequence &seq) {
  for (sequence_iterator it1 = (*this).events.begin(); it1 != (*this).events.end(); it1++) {
    for (sequence_iterator it2 = seq.events.begin(); it2 != seq.events.end(); it2++) {
      if ((*it1).same_object(*it2)) {
        if ((*it1).is_read() && (*it2).is_write()) return true;
        if ((*it1).is_write() && (*it2).is_read()) return true;
      }
    }
  }

  return false;
}