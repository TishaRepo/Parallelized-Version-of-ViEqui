#include "ViewEqTraceBuilder.h"

typedef int IPid;

ViewEqTraceBuilder::ViewEqTraceBuilder(const Configuration &conf) : TSOPSOTraceBuilder(conf)
{
  threads.push_back(Thread(CPid(), -1));
  current_thread = -1;
  prefix_idx = 0;
  std::vector<IID<IPid>> evs;
  Sequence s(evs,&threads);
  execution_sequence = s;
  round  = 3; // [snj]: TODO temp remove eventually
}

ViewEqTraceBuilder::~ViewEqTraceBuilder() {
  threads.push_back(Thread(CPid(), -1));
  current_thread = -1;
  prefix_idx = 0;

  round  = 3; // [snj]: TODO temp remove eventually
}

bool ViewEqTraceBuilder::schedule(int *proc, int *aux, int *alt, bool *DryRun)
{
  // snj: For compatibility with existing design
  *aux = -1;
  *alt = 0;
  *DryRun = false;
  // // //

  // [snj]: peak next read/write event / execute next non-read/write event
  // reverse loop to prioritize newly created threads
  for (int i = threads.size()-1; i >=0 ; i--) {
    if (threads[i].available && !threads[i].awaiting_load_store) {   // && (conf.max_search_depth < 0 || threads[i].events.size() < conf.max_search_depth)){
      IID<IPid> iid = IID<IPid>(IPid(i), threads[i].events.size());
      threads[i].push_back(no_load_store);
      execution_sequence.push_back(iid);
      prefix_idx++;

      current_thread = i;
      *proc = i;
      return true;
    }
  }

  if (Enabled.empty()) {
    debug_print();
    return false; // [snj]: maximal trace explored
  }

  // [snj]: TODO algo goes here
  std::pair<bool, std::pair<IID<IPid>,IID<IPid>>> RW = enabaled_RWpair();
  IID<IPid> enabled_event = Enabled.front();
  Enabled.erase(Enabled.begin());
  current_thread = enabled_event.get_pid();
  current_event = threads[current_thread][enabled_event.get_index()];
  assert(current_event.is_write() || current_event.is_read());
  assert(0 <= current_thread && current_thread < long(threads.size()));

  // [snj]: record current event as next in execution sequence
  // TODO only read write in exn seq but need all or atleat tid + how to set read values
  execution_sequence.push_back(current_event.iid);
  prefix_idx++;

  // [snj]: execute current event from Interpreter::run() in Execution.cpp
  threads[current_thread].awaiting_load_store = false; // [snj]: next event after current may not be load or store
  *proc = current_thread;
  return true;
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

std::pair<bool, std::pair<IID<IPid>, IID<IPid>>> ViewEqTraceBuilder::enabaled_RWpair() {
  for (auto i = Enabled.begin(); i != Enabled.end(); i++) {
    for (auto j = i+1; j != Enabled.end(); j++) {
      int tid1 = (*i).get_pid();
      int tid2 = (*j).get_pid();

      int eid1 = (*i).get_index();
      int eid2 = (*j).get_index();

      if (threads[tid1][eid1].same_object(threads[tid2][eid2])) {
        if (threads[tid1][eid1].is_read() && threads[tid2][eid2].is_write())
          return std::make_pair(true, std::make_pair((*i),(*j)));
        if (threads[tid1][eid1].is_write() && threads[tid2][eid2].is_read())
          return std::make_pair(true, std::make_pair((*j),(*i)));
      }
    }
  }
    
  return std::make_pair(true, std::make_pair(dummy_id,dummy_id));
}

int ViewEqTraceBuilder::current_value(unsigned obj) {
  return mem[obj];
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

bool ViewEqTraceBuilder::reset() {
  if (round == 0) return false;
  llvm::outs() << "reset round " << round << "\n";
  round--;
  execution_sequence.clear();

  CPS = CPidSystem();
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
  IID<CPid> i;
  return i;
}

void ViewEqTraceBuilder::refuse_schedule() { //[snj]: TODO check if pop_back from exn_seq has to be done
  execution_sequence.pop_back();
  mark_unavailable(current_thread);
}

bool ViewEqTraceBuilder::is_replaying() const {
  return prefix_idx < replay_point;
}

void ViewEqTraceBuilder::cancel_replay() {replay = false;} //[snj]: needed?

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
  mem[event.object] = event.value;
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

bool ViewEqTraceBuilder::atomic_store(const SymData &ml) {
  // [snj]: visitStoreInst in Execution.cpp lands here not in store
  Event event(SymEv::Store(ml));
  event.make_write();
  mem[event.object] = event.value;
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
} //[snj]: TODO

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

bool ViewEqTraceBuilder::Event::same_object(ViewEqTraceBuilder::Event event) {
  return (object == event.object);
}

bool ViewEqTraceBuilder::Event::operator==(ViewEqTraceBuilder::Event event) {
  if (value != event.value)
    return false;

  return (symEvent == event.symEvent);
}

bool ViewEqTraceBuilder::Event::operator!=(ViewEqTraceBuilder::Event event) {
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
  return (sym_event().to_string() + " *** (" + type_to_string() + "(" + std::to_string(object) + "," + std::to_string(value) + "))");
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::prefix(IID<IPid> ev) {
  assert(this->has(ev));
  sequence_iterator it = find(begin(), end(), ev);
  std::vector<IID<IPid>> pre(begin(), it);
  Sequence sprefix(pre, threads);
  return sprefix;
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::suffix(IID<IPid> ev) {
  assert(this->has(ev));
  sequence_iterator it = find(begin(), end(), ev);
  std::vector<IID<IPid>> suf(it+1, end());
  Sequence ssuffix(suf, threads);
  return ssuffix;
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::suffix(ViewEqTraceBuilder::Sequence &seq) {
  sequence_iterator it = find(begin(), end(), *(seq.end()-1));
  std::vector<IID<IPid>> suf(it+1, end());
  Sequence ssuffix(suf, threads);
  return ssuffix;
}

bool ViewEqTraceBuilder::Sequence::conflicting(ViewEqTraceBuilder::Sequence &other_seq){
  for(int i = 0; i < events.size(); i++){
    for(int j = i + 1; j < events.size() ; j++){
      if(other_seq.has(events[i]) && other_seq.has(events[j])){
        sequence_iterator it1 = find(other_seq.begin(),other_seq.end(),events[i]);
        sequence_iterator it2 = find(other_seq.begin(),other_seq.end(),events[j]);
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

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::poPrefix(IID<IPid> ev){
  assert(this->has(ev));
  sequence_iterator it;
  std::vector<IID<IPid>> poPre;
  for(it = events.begin(); (*it) != ev; it++){
    if(it->get_pid() == ev.get_pid()) poPre.push_back(*it);
  }
  Sequence spoPrefix(poPre, threads);
  return spoPrefix;
}

ViewEqTraceBuilder::Sequence ViewEqTraceBuilder::Sequence::backseq(IID<IPid> e1, IID<IPid> e2){
  assert(this->has(e1) && this->has(e2));
  std::vector<IID<IPid>> taupr{e1};
  ViewEqTraceBuilder::Sequence tauPrime(taupr, threads);
  tauPrime.concatenate(this->suffix(e1));
  tauPrime.push_back(e2);
  ViewEqTraceBuilder::Sequence res = tauPrime.poPrefix(e2);
  ViewEqTraceBuilder::Event event1 = threads->at(e1.get_pid())[e1.get_index()];
  ViewEqTraceBuilder::Event event2 = threads->at(e2.get_pid())[e2.get_index()];
  if(event1.is_write()) res.push_back(e1);
  if(event2.is_write()) res.push_back(e2);
  if(event1.is_read()) res.push_back(e1);
  if(event2.is_read()) res.push_back(e2);

  return res;
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

    ViewEqTraceBuilder::Event ev = threads->at(e.get_pid())[e.get_index()];
    if (ev.is_write()) { // algo 6-17
      if (!other.has(e)) { // e not in both primary and other [algo 6-12]
        IID<IPid> er;
        ViewEqTraceBuilder::sequence_iterator it;
        for (it = other.events.begin(); it != other.events.end(); it++) {
          er = *it;
          ViewEqTraceBuilder::Event event_er = threads->at(er.get_pid())[er.get_index()];
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

    if (ev.is_read()) { // algo 18-21
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

  IID<IPid> dummy;
  Sequence joined;
  std::tuple<Sequence, Sequence, Sequence> triple = join(current_seq, other_seq, dummy, joined);

  assert(std::get<0>(triple).size() == 0);
  assert(std::get<1>(triple).size() == 0);

  return std::get<2>(triple);
}

// [snj]: TODO function not tested
bool ViewEqTraceBuilder::Sequence::hasRWpairs(Sequence &seq) {
  for (sequence_iterator it1 = (*this).events.begin(); it1 != (*this).events.end(); it1++) {
    for (sequence_iterator it2 = seq.events.begin(); it2 != seq.events.end(); it2++) {
      ViewEqTraceBuilder::Event e1 = (*threads)[it1->get_pid()][it1->get_index()];
      ViewEqTraceBuilder::Event e2 = (*threads)[it2->get_pid()][it2->get_index()];
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
    sequence_iterator it2 = find(other.begin(),other.end(),(*it1));
    if (it2 == other.end()) continue; // event not in other

    for (sequence_iterator it12 = current.begin(); it12 != it1; it12++) {
      for (sequence_iterator it22 = it2+1; it22 != other.end(); it22++)
        if ((*it12) == (*it22))
          return true;
    }
  }

  return false;
}

bool Lead::operator==(Lead l) {
  if (constraint != l.constraint)
    return false;

  if (strat != l.strat)
    return false;

  return true;
}

void State::add_done(Sequence d) {
  for (auto it = done.begin(); it != done.end(); it++) {
    if (d == (*it))
      return; // sequence alrready added
    
    if ((*it).size() > d.size())
      done.insert(it, d); // sorted by length
      return;
  }

  done.push_back(d);
}

bool State::is_done(Sequence seq) {
  for (auto it = done.begin(); it != done.end(); it++) {
    if (seq == (*it))
      return true;

    if ((*it).size() > seq.size())
      return false;
  }

  return false;
}