#include "ViewEqTraceBuilder.h"


ViewEqTraceBuilder::ViewEqTraceBuilder(const Configuration &conf) : TSOPSOTraceBuilder(conf)
{
  threads.push_back(Thread(CPid(), -1));
  prefix_idx = -1;
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

bool ViewEqTraceBuilder::reset() {llvm::outs() << "[snj]: reached reset\n"; replay_point = 0; return false;} //[snj]: TODO -- find replay point, setup for replay (next events etc)
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
  llvm::outs() << "[snj]: in store\n";
  bool pre_ret = store_pre(ml);
  return store_post(ml) && pre_ret;
}

bool ViewEqTraceBuilder::atomic_store(const SymData &ml) {
  bool pre_ret = store_pre(ml);
  return store_post(ml) && pre_ret;
}

bool ViewEqTraceBuilder::fence() { 
  // [snj]: invoked at pthread create
  // llvm::outs() << "[snj]: fence being invoked!!"; 
  return true;
}

Trace* ViewEqTraceBuilder::get_trace() const {Trace *t = NULL; return t;} //[snj]: TODO
void ViewEqTraceBuilder::debug_print() const {
  llvm::outs() << "Debug_print: execution_sequence.size()=" << execution_sequence.size() << "\n";
  for (int i = 0; i < execution_sequence.size(); i++) {
    if (execution_sequence[i].symEvent.size() < 1) {
      llvm::outs() << "[" << i << "]: --\n";
      continue;
    }
    llvm::outs() << "[" << i << "]: " << "<" << execution_sequence[i].symEvent[0] << ">\n";
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