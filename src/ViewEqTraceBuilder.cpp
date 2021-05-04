#include "ViewEqTraceBuilder.h"

ViewEqTraceBuilder::ViewEqTraceBuilder(const Configuration &conf = Configuration::default_conf) : TSOPSOTraceBuilder(conf)
{
  threads.push_back(Thread(CPid(), -1));
  prefix_idx = -1;
}
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

  for (i = 0; i < size; i += 2)
  {
    if (threads[i].available && (conf.max_search_depth < 0 || threads[i].event_indices.size() < conf.max_search_depth))
    {
      threads[i].event_indices.push_back(++prefix_idx);
      Event event = Event(IID<IPid>(IPid(i), threads[i].event_indices.size()));
      execution_sequence.push_back(event);
      *proc = i / 2;
      *aux = -1;
      return true;
    }
  }

  return false;
}

void ViewEqTraceBuilder::mark_available(int proc, int aux)
{
  threads[ipid(proc, aux)].available = true;
}

void ViewEqTraceBuilder::mark_unavailable(int proc, int aux)
{
  threads[ipid(proc, aux)].available = false;
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
  threads.push_back(Thread(CPS.new_aux(child_cpid), prefix_idx));
  threads.back().available = false; // Empty store buffer
  curev().may_conflict = true;
  return record_symbolic(SymEv::Spawn(threads.size() / 2 - 1));
}

bool ViewEqTraceBuilder::store_pre(const SymData &sd)
{
  curev().may_conflict = true; /* prefix_idx might become bad otherwise */

  if (!record_symbolic(SymEv::Store(sd)))
    return false;

  return true;
}

bool ViewEqTraceBuilder::store_post(const SymData &sd)
{
  const SymAddrSize &ml = sd.get_ref();
  IPid ipid = curev().iid.get_pid();
  curev().may_conflict = true;

  // [snj]: TODO remove this part eventually
  bool is_update = ipid % 2;
  if (is_update)
  {
    llvm::outs() << "snj: FIX NEEDED: aux event is being accessed";
    return true;
  }
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
}

bool ViewEqTraceBuilder::load_pre(const SymAddrSize &ml)
{
  if (!record_symbolic(SymEv::Load(ml)))
    return false;

  return true;
}

bool ViewEqTraceBuilder::load_post(const SymAddrSize &ml)
{
  curev().may_conflict = true; // [snj]: TODO do we need may_conflict
  IPid ipid = curev().iid.get_pid();

  /* Load from memory */
  VecSet<int> seen_accesses;

  /* See all updates to the read bytes. */
  for (SymAddr b : ml)
  {
    //load_post(mem[b]); // [snj]: TODO why the recursive call also its a overloaded diff function

    /* Register load in memory */
    mem[b].last_read[ipid] = prefix_idx;
  }

  //   seen_accesses.insert(last_full_memory_conflict);

  //   see_events(seen_accesses);
}

// [rmnt]: TODO : Implement the notion of replay and associated functions
bool ViewEqTraceBuilder::record_symbolic(SymEv event)
{
  llvm::outs() << "rmnt: Inside record_symbolic where we have a symbolic event \n";
  llvm::outs() << event.to_string() << "\n";

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