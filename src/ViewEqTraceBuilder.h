#include <config.h>
#ifndef __VIEW_EQ_TRACE_BUILDER_H__
#define __VIEW_EQ_TRACE_BUILDER_H__

#include <unordered_map>

#include "TSOPSOTraceBuilder.h"
#include "SymEv.h"
#include "TraceUtil.h"
#include "SOPFormula.h"
#include "Visible.h"

typedef llvm::SmallVector<SymEv, 1> sym_ty;

class HashFn{
    public:
        size_t operator()(const std::pair<unsigned, unsigned> p) const{
            return ( ((p.first + p.second) * (p.first + p.second + 1))/2 + p.second);
        }
};

class StringHash {
    std::string str;
    long long hash_value;

    long long polynomialRollingHash();

public:
    StringHash() {}
    StringHash(std::string s) : str(s)  {}

    long long hash() {return polynomialRollingHash();}
};


class ViewEqTraceBuilder : public TSOPSOTraceBuilder
{
protected:
    class Event;
    class Thread;
    class Sequence;
    class EventSequence;
    class Lead;
    class State;

public:
    typedef int IPid;

    ViewEqTraceBuilder(const Configuration &conf = Configuration::default_conf);
    virtual ~ViewEqTraceBuilder() override;

    virtual bool schedule(int *proc, int *type, int *alt, bool *doexecute) override;
            void replay_schedule(int *proc);
    virtual void refuse_schedule() override;
            void replay_memory_access(int next_replay_thread, IID<IPid> next_replay_event);
            void replay_non_memory_access(int next_replay_thread, IID<IPid> next_replay_event);
    
    virtual void metadata(const llvm::MDNode *md) override;
    virtual void mark_available(int proc, int aux = -1) override;
    virtual void mark_unavailable(int proc, int aux = -1) override;
            bool is_enabled(int thread_id);
    /* [snj]: if read & write of same object is enabled, return the read */
            std::pair<bool, IID<IPid>> enabaled_RWpair_read();
    
    virtual NODISCARD bool full_memory_conflict() override;
    virtual NODISCARD bool join(int tgt_proc) override;
    virtual NODISCARD bool spawn() override;
    virtual NODISCARD bool store(const SymData &ml) override;
    virtual NODISCARD bool atomic_store(const SymData &ml) override;
    virtual NODISCARD bool load(const SymAddrSize &ml) override;

            void finish_up_lead(int replay_state_prefix);
            void finish_up_state(int replay_state_prefix);
            int  find_replay_state_prefix();
    virtual bool reset() override;
    virtual void cancel_replay() override;
    virtual bool is_replaying() const override;
            bool at_replay_point();

    // get id of an event
    virtual IID<CPid> get_iid() const override;
    // get event from id
    Event get_event(IID<IPid> event_id) {return threads[event_id.get_pid()][event_id.get_index()];}
    Event get_event(IID<IPid> event_id) const {return threads[event_id.get_pid()][event_id.get_index()];}

    // verbose information of a trace
    virtual Trace *get_trace() const override;
    virtual void debug_print() const override;

    // detect and record redundant explorations
    void record_redundant();
    // report summary of redundant explorations
    void report_redundant();

    // current value in memory for an object
    int current_value(std::pair<unsigned, unsigned> obj);

    void update_join_summary(Event event);
    void update_spawn_summary(Event event);

    // returns a list of dependent (R-W, W-R) and non-forbidden events for an event
    std::unordered_set<IID<IPid>> unexploredInfluencers(Event er, SOPFormula& f);
    std::unordered_set<IID<IPid>> exploredInfluencers(Event er, SOPFormula &f);
    std::unordered_set<IID<IPid>> exploredWitnesses(Event ew, SOPFormula &f);
    
    // update leads at for a new encountered event (calls forward and backward analysis)
    void update_leads(IID<IPid> event_id, SOPFormula& forbidden) {update_leads(get_event(event_id), forbidden);}
    void update_leads(Event event, SOPFormula& forbidden);
    
    // helper functions of backward analysis
    // compute the start of the next lead and compute the state to add it to
    int  union_state_start(int prefix_idx, IID<IPid> event, Sequence& start);
    // check if an event in a start has different causal dependence in different extensions
    bool indepenent_event_in_leads(int state, IID<IPid> event);
    // check if the causal dependence all events in the lead-start remain the same for all extensions
    bool is_independent_EW_lead(Sequence& start, IID<IPid> write_event, IID<IPid> read_event);
    // perform union of state leads with pending EW_leads
    void add_EW_leads(int state);
    // for leads of ei at 'state' for fwd analysed 'read' add 'value' to forbidden
    // since, 'value' was not known to be covered at time of forward-analysis
    void forbid_value_for_ei_leads(int state, IID<IPid> read, int value);

    // forward and backward analysis for builfing view-starts
    void forward_analysis(Event event, SOPFormula& forbidden);
    void backward_analysis_read(Event event, SOPFormula& forbidden, std::unordered_map<int, std::vector<Lead>>& L);
    void backward_analysis_write(Event event, SOPFormula& forbidden, std::unordered_map<int, std::vector<Lead>>& L);
    void backward_analysis(Event event, SOPFormula& forbidden);

    // is e1 ordered before due to a corresponding join event
    bool ordered_by_join(IID<IPid> e1, IID<IPid> e2);
    // is e1 ordered before due to a corresponding spawn event
    bool ordered_by_spawn(IID<IPid> e1, IID<IPid> e2);

    // remove duplicate occurances in the vector
    void remove_duplicate_leads(std::vector<Lead>& L);
    // combining new lead with existing leads of a state
    void consistent_union(int state, Lead& l);
    // combining new leads with existing leads of a state
    void consistent_union(int state, std::vector<Lead>& L);

    // forwardind a lead from a state S_i to a states S_{i+x} because alpha of S_i is a prefix of the lead
    bool forward_lead(std::unordered_map<int, std::vector<Lead>>& forward_state_leads, int state, Lead lead);
    void forward_suffix_leads(std::unordered_map<int, std::vector<Lead>>& forward_state_leads, int state, std::vector<Lead>& L);    

    // basic steps followed bu scedule
    bool exists_non_memory_access(int * proc);
    void make_new_state();
    void compute_new_leads();
    void execute_next_lead();

    void analyse_unexplored_influenecers(IID<IPid> read_event);

    // enable load corresponding to the rmw
    void enable_rmw(const SymAddrSize &ml);
    // add rmw to execution, do necessary analysis
    void complete_rmw(const SymData &ml);
    // complete local rmw
    virtual NODISCARD bool atomic_rmw(const SymData &ml);

    //[nau]: added virtual function definitions for the sake of compiling
    //[snj]: added some more to the list
    virtual NODISCARD bool compare_exchange(const SymData &sd, const SymData::block_type expected, bool success) override;
    virtual NODISCARD bool fence() override;
    virtual bool sleepset_is_empty() const override;
    virtual bool check_for_cycles() override;
    virtual NODISCARD bool mutex_lock(const SymAddrSize &ml) override;
    virtual NODISCARD bool mutex_lock_fail(const SymAddrSize &ml) override;
    virtual NODISCARD bool mutex_trylock(const SymAddrSize &ml) override;
    virtual NODISCARD bool mutex_unlock(const SymAddrSize &ml) override;
    virtual NODISCARD bool mutex_init(const SymAddrSize &ml) override;
    virtual NODISCARD bool mutex_destroy(const SymAddrSize &ml) override;
    virtual NODISCARD bool cond_init(const SymAddrSize &ml) override;
    virtual NODISCARD bool cond_signal(const SymAddrSize &ml) override;
    virtual NODISCARD bool cond_broadcast(const SymAddrSize &ml) override;
    virtual NODISCARD bool cond_wait(const SymAddrSize &cond_ml,
                                     const SymAddrSize &mutex_ml) override;
    virtual NODISCARD bool cond_awake(const SymAddrSize &cond_ml,
                                      const SymAddrSize &mutex_ml) override;
    virtual NODISCARD int cond_destroy(const SymAddrSize &ml) override;
    virtual NODISCARD bool register_alternatives(int alt_count) override;

    // [snj]: not in use
    // bool NODISCARD record_symbolic(Event event, SymEv symevent);
    bool NODISCARD record_symbolic(SymEv event);

protected:

    class Event
    {
    public:
        enum ACCESS_TYPE {READ, WRITE, SPAWN, JOIN} type;
        IID<IPid> iid;
        sym_ty symEvent;
        const llvm::MDNode *md;
        // [snj]: value signifies the value read by a read event or written by a write event in the current
        // execution sequence, all value are initialized to 0 by default
        int value;
        std::pair<unsigned, unsigned> object; // <base, offset> - offset for arrays
        bool is_rmw = false; // is a load or store of an rmw event
        
        Event() {}
        Event(SymEv sym) {symEvent.push_back(sym); md = 0;}
        Event(const IID<IPid> &iid, sym_ty sym = {}) : iid(iid), symEvent(std::move(sym)), md(0){};
        void make_spawn();
        void make_join();
        void make_read();
        void make_write();
        SymEv sym_event() const {return symEvent[0];}

        /* is read of shared object */
        bool is_read() {return (symEvent.size()==1 && sym_event().addr().addr.block.is_global() && type == READ);}
        /* is write of shared object */
        bool is_write() {return (symEvent.size()==1 && sym_event().addr().addr.block.is_global() && type == WRITE);}
        bool is_global() {return (symEvent.size()==1 && sym_event().addr().addr.block.is_global());}
        bool same_object(Event e);
        bool RWpair(Event e);

        IID<IPid> get_iid() const {return iid;}
        int get_index() {return iid.get_index();}
        IPid get_pid() {return iid.get_pid();}
        int get_index() const {return iid.get_index();}
        IPid get_pid() const {return iid.get_pid();}


        std::vector<Event> unexploredInfluencers(Sequence &seq);
        std::vector<Event> exploredInfluencers(Sequence &seq);
        std::vector<Event> exploredWitnesses(Sequence &seq);
        Sequence prefix(Sequence &seq);

        std::string to_string() const;
        std::string type_to_string() const;
        
        bool operator==(Event event);
        bool operator!=(Event event);
    };

    typedef std::vector<IID<IPid>>::iterator sequence_iterator;
    
    class Sequence
    {
    private:
        // [snj]: required by consistent_merge function
        Sequence join(Sequence& other_seq);
        // auxiliary function for join
        void join_prefix(std::vector<IID<IPid>>::iterator, std::vector<IID<IPid>>::iterator,
                         std::vector<IID<IPid>>::iterator, std::vector<IID<IPid>>::iterator);

        enum PREFIX_TYPE {JOIN, JOINe1, CAUSAL};
        void assign_thread_causality(Event& event, IPid& causal_before, IPid& causal_after,
            Sequence* prefix, std::vector<unsigned>* threads_of_prefix, IID<IPid>& e1_delim, IID<IPid>& e1,IID<IPid>& e2);
        void update_thread_causality(Event& event, IPid& causal_before, IPid& causal_after,
            Sequence* prefix, std::vector<unsigned>* threads_of_prefix, IID<IPid>& e1_delim, IID<IPid>& e1,IID<IPid>& e2);
        void add_causal_event(Sequence* prefix, std::vector<unsigned>* threads_of_prefix,
            std::unordered_map<unsigned, std::unordered_set<unsigned>>* objects_for_source, bool& added_to_prefix, 
            IID<IPid>& e1, IID<IPid>& e1_delim, Event event, int idx);
        void prefix_rf_source(Sequence* prefix, std::vector<unsigned>* threads_of_prefix, 
            std::unordered_map<unsigned, std::unordered_set<unsigned>>* objects_for_source, Event& event, 
            bool& added_to_prefix, int idx);
    public:
        std::vector<IID<IPid>> events;
        ViewEqTraceBuilder* container;

        Sequence(){assert(false);}
        Sequence(ViewEqTraceBuilder* c){container = c;}
        Sequence(std::vector<IID<IPid>> &seq, ViewEqTraceBuilder* c){events = seq; container = c;}
        Sequence(IID<IPid>& e, ViewEqTraceBuilder* c) {events.push_back(e); container = c;}
        Sequence(std::unordered_set<IID<IPid>> a, ViewEqTraceBuilder* c) {events.insert(events.begin(), a.begin(), a.end()); container = c;}
        
        void set_container_reference(ViewEqTraceBuilder* c) {container = c;}
        EventSequence to_event_sequence(std::unordered_map<std::pair<unsigned, unsigned>, int, HashFn>& mem);
        EventSequence to_event_sequence(EventSequence& source, std::unordered_map<std::pair<unsigned, unsigned>, int, HashFn>& mem);

        bool empty() {return (size() == 0);}
        std::size_t size() const {return events.size();}
        IID<IPid> last() {if (empty()) return IID<IPid>(-1,-1); return events.back();}
        std::vector<IID<IPid>>::iterator begin() {return events.begin();}
        std::vector<IID<IPid>>::iterator end() {return events.end();}
        IID<IPid> head() {return events.front();}
        Sequence tail() {
            Sequence tl(events, container);
            tl.pop_front();
            return tl;
        }

        void push_back(IID<IPid> event) {events.push_back(event);}
        void push_front(IID<IPid> event) {events.insert(events.begin(), event);}
        void push_at(sequence_iterator loc, IID<IPid> event) {events.insert(loc, event);}
        void pop_back() {events.pop_back();} 
        void pop_front() {events.erase(events.begin());};
        void erase(IID<IPid> event) {events.erase(events.begin() + index_of(event));}
        sequence_iterator erase(sequence_iterator it) {return events.erase(it);}
        void erase(sequence_iterator begin, sequence_iterator end) {events.erase(begin, end);}
        void clear() {events.clear();}
        
        bool has(IID<IPid> event) {return std::find(events.begin(), events.end(), event) != events.end();}
        int  index_of(IID<IPid> event) {return (std::find(events.begin(), events.end(), event) - events.begin());}
        sequence_iterator find(IID<IPid> event) {return std::find(events.begin(), events.end(), event);}

        Sequence subsequence(sequence_iterator begin, sequence_iterator end);
        void concatenate(Sequence seq) { events.insert(events.end(), seq.events.begin(), seq.events.end()); }
        void concatenate(Sequence seq, sequence_iterator begin) {events.insert(events.end(), begin, seq.events.end());}
        bool hasRWpairs(Sequence &seq);
        
        IID<IPid>& operator[](std::size_t i) {return events[i];}
        const IID<IPid>& operator[](std::size_t i) const {return events[i];}
        bool operator==(Sequence seq) {return (events == seq.events);}
        bool operator!=(Sequence seq) {return (events != seq.events);}
        std::ostream &operator<<(std::ostream &os){return os << to_string();}

        /* if this is prefix of seq */
        bool isprefix(Sequence &seq);
        /* execution subsequence from e1 to e2 including events between e1 and e2 that causally preceed e2 */
        std::pair<IID<IPid>, Sequence> causal_prefix(IID<IPid> e1, IID<IPid> e2, sequence_iterator begin, sequence_iterator end);
        /* program ordered prefix upto end of ev in this */
        
        /* In the sequence attempt to shift events from thread of e1 (including e1)
           to after e2. If cannot shift an event coherently then retun false and keep
           original sequence. */
        bool view_adjust(IID<IPid> e1, IID<IPid> e2);
        /* has events in this and other that occur in reverse order */
        std::pair<bool, std::pair<IID<IPid>, IID<IPid>>> conflicts_with(Sequence &other_seq);
        /* poprefix(e1).(write out of e1, e2).(read out of e1, e2) */
        Sequence backseq(IID<IPid> e1, IID<IPid> e2);

        // [snj]: consistent merge, merges 2 sequences such that all read events maitain their sources
        //          i.e, reads-from relation remain unchanged
        Sequence consistent_merge(Sequence &other_seq);

        std::string to_string();

    };
    Sequence empty_sequence;

    typedef std::vector<Event>::iterator event_sequence_iterator;

    class EventSequence {
    public:
        std::vector<Event> events;
        ViewEqTraceBuilder* container;

        EventSequence() {}
        EventSequence(ViewEqTraceBuilder* c) {container = c;}
        void set_container_reference(ViewEqTraceBuilder* c) {container = c;}

        bool empty() {return (size() == 0);}
        int size() {return events.size();}
        void clear() {events.clear();}
        
        event_sequence_iterator begin() {return events.begin();}
        event_sequence_iterator end() {return events.end();}
        Event last() {if (empty()) return Event(); return events.back();}
        Event head() {return events.front();}
        EventSequence tail() {
            EventSequence tl = *this;
            tl.erase(tl.begin());
            return tl;
        }
       
        bool has(Event event) {return std::find(events.begin(), events.end(), event) != events.end();}
        event_sequence_iterator find(Event event) {return std::find(events.begin(), events.end(), event);}
        event_sequence_iterator find_iid(IID<IPid> iid);
        int  index_of(Event event) {return (std::find(events.begin(), events.end(), event) - events.begin());}

        void push_back(Event event) {events.push_back(event);}
        void pop_front() {events.erase(events.begin());};
        
        void erase(Event event) {events.erase(events.begin() + index_of(event));}
        void erase(IID<IPid> event_id) {erase(find_iid(event_id));}
        event_sequence_iterator erase(event_sequence_iterator it) {return events.erase(it);}

        Sequence to_iid_sequence();
        std::unordered_map<IID<IPid>, int> read_value_map();

        /* this == seq with resolvable conflicts */
        bool VA_equal(EventSequence& seq);
        /* In the sequence attempt to shift events from thread of e1 (including e1)
           to after e2. If cannot shift an event coherently then retun false and keep
           original sequence. */
        bool view_adjust(Event e1, Event e2);
        /* has events in this and other that occur in reverse order */
        std::pair<bool, std::pair<Event, Event>> conflicts_with(EventSequence &other_seq);

        bool operator==(EventSequence seq);
        bool operator!=(EventSequence seq);

        std::string to_string();
    };


    class Thread
    {
    public:
        // [snj]: TODO is spawn event needed?
        Thread(const CPid &cpid, int spawn_event) : cpid(cpid), spawn_event(spawn_event), available(true), awaiting_load_store(false){};
        Thread(const CPid &cpid) : cpid(cpid), available(true), awaiting_load_store(false){};
        CPid cpid;
        int spawn_event;
        bool available;
        std::vector<Event> events;
        bool awaiting_load_store;

        void push_back(Event event) {events.push_back(event);}
        void pop_back() {events.pop_back();}
        int  size() {return events.size();}

        Event& operator[](std::size_t i) {return events[i];}
        const Event& operator[](std::size_t i) const {return events[i];}
    };

    class Lead {
    public:
        Sequence constraint;           // sequence from previous trace to be maintained
        Sequence start;                // new sequence to be explore to get the intended (read,value) pair
        SOPFormula forbidden;          // (read,value) pairs that must not be explored
        EventSequence merged_sequence; // consistent_merge(start, constraint) sequence to explore while maintaining constraint
        bool is_done = false;          // whether the lead has been explored

        ViewEqTraceBuilder* container;
        
        Lead() {}
        Lead(const Lead &l, SOPFormula f) {
            constraint = l.constraint; start = l.start; forbidden = f; 
            merged_sequence = l.merged_sequence;
            container = l.container;
        }
        Lead(EventSequence c, Sequence s, SOPFormula f, std::unordered_map<std::pair<unsigned, unsigned>, int, HashFn>& mem) {
            constraint = c.to_iid_sequence(); start = s; forbidden = f;
            Sequence merged_sequence_id = start.consistent_merge(constraint);
            merged_sequence = merged_sequence_id.to_event_sequence(c, mem);
            merged_sequence.set_container_reference(s.container);
            container = s.container;
        }
        Lead(Sequence s, SOPFormula f, std::unordered_map<std::pair<unsigned, unsigned>, int, HashFn>& mem) {
            constraint.set_container_reference(s.container);
            start = s; forbidden = f;
            merged_sequence = s.to_event_sequence(mem);
            merged_sequence.set_container_reference(s.container);
            container = s.container;
        }
        Lead(EventSequence s, SOPFormula f) {
            constraint.set_container_reference(s.container);
            start = s.to_iid_sequence(); forbidden = f;
            merged_sequence = s;
            merged_sequence.set_container_reference(s.container);
            container = s.container;
        }

        /* this is prefix of l with view-adjustment */
        bool VA_isprefix(Lead& l);
        /* same reads and same values of reads */
        bool VA_equivalent(Lead& l);
        /* suffix of a view-adjusted prefix */
        Sequence VA_suffix(Lead& l);
        
        bool operator==(Lead l);
        std::ostream &operator<<(std::ostream &os){return os << to_string();}

        std::string to_string();
    };

    /* [snj]: dummy event */
    IID<IPid> dummy_id;

    /* [rmnt]: The CPids of threads in the current execution. */
    CPidSystem CPS;

    class State
    {
    public:
        // index of sequence explored corresponding to current state
        int sequence_prefix;                 // index in execution sequence corresponding to this state
        std::vector<Lead> leads;             // leads(configurations) to be explored from this state
        SOPFormula forbidden;                // (read,value pairs that must not be seen after this state)
        int alpha = -1;                      // current lead being explored
        
        int lead_head_execution_prefix = -1; // idx in execution sequence where alpha lead starts (-1 if not a part of a lead)
        bool executing_alpha_lead = false;   // state is a part of alpha

        bool performed_fwd_analysis = false; // forward-analysis was performed at this state
        IID<IPid> fwd_read;                  // forward-analysis was performed for this read
        std::unordered_map<int, std::vector<Lead>> ei_leads; // state -> leads of ei's for this fwd_read

        // shared memory values at this state of execution ( object -> value map )
        std::unordered_map<std::pair<unsigned, unsigned>, int, HashFn> mem; 

        State() {}
        State(int prefix_idx) : sequence_prefix(prefix_idx) {};
        // State(int i, Lead a, SOPFormula f) {sequence_prefix = i; alpha = a; forbidden = f; alpha_sequence = alpha.start + alpha.constraint;};
        
        bool has_unexplored_leads();
        std::vector<Lead> unexplored_leads();
        Lead next_unexplored_lead();

        bool alpha_empty() {return (alpha == -1);}
        EventSequence alpha_sequence() {if (alpha == -1) {llvm::outs() << "MUST NOT BE HERE\n"; return EventSequence();} return leads[alpha].merged_sequence;}
        std::string print_leads();
    };

    /* stream for debug printing on stdout */
    llvm::raw_ostream &out = llvm::outs();

    /* [snj]: check and report redundant explorations  */
    bool check_optimality;

    /* [snj]: list of read_value maps representing execution sequences explored as
       pair(hash_value, read -> value map)
    */
    std::vector<std::unordered_map<IID<IPid>, int>> explored_sequences_summary;

    /* [snj]: list of redundant explorations
        earliest redundant trace -> later redundant traces
    */
    std::unordered_map<int, std::vector<int>> redundant;

    /* The index into prefix corresponding to the last event that was
    * scheduled. Has the value -1 when no events have been scheduled.
    */
    // [snj]: index in execution sequence same as size of sequence-1
    int prefix_idx;

    // [rmnt]: TODO: Do we need sym_idx? It seems to play an important role in record_symbolic as well as whenever we are replaying
    /* [rmnt]: Using their comment for now
    * The index of the currently expected symbolic event, as an index into
    * curev().sym. Equal to curev().sym.size() (or 0 when prefix_idx == -1) when
    * not replaying.
    */
    unsigned sym_idx;

    /* The number of events that were or are going to be replayed in the
    * current computation.
    */
    int replay_point;

    /* The latest value passed to this->metadata(). */
    const llvm::MDNode *last_md;

    /* [snj]: program threads (has vector of events) */
    std::vector<Thread> threads;

    /* [snj]: executions states of current trace */
    std::vector<State> states;

    /* [snj]: Thread id of the thr(current event) */
    int current_thread;

    /* [snj]: current event */
    Event current_event;

    /* [snj]: current state (for read and write - no state for other events) */
    int current_state;

    /* [snj]: execution sequence index where current alpha starts */
    int alpha_head;

    /* [snj]: set of enabled events ie next events of each thread */
    // list of (thread id, next event) pairs
    std::vector<IID<IPid>> Enabled;

    /* [snj]: a EW read that has been assigned a start for a value v from 
       its execution suffixes
       read -> set of values   
    */
   // [snj]: TODO remove this and add the covered value to the forbidden of all at state of the read 
    std::unordered_map<IID<IPid>, std::vector<int>>  covered_read_values;

    /* [snj]: a set of leads that generated from different extensions
       of a EW read for a value v.
       state -> read -> value -> list of leads to get the rv pair (read,value)
    */
    std::unordered_map<int, std::unordered_map<IID<IPid>, std::unordered_map<int, std::vector<Lead>>>> EW_leads;

    /* [snj]: information of spawns occured in current execution sequence
       thread_id(t) -> thread_ids that spawned from t -> event index of the corresponding spawn event
    */
    std::unordered_map<IPid, std::unordered_map<IPid, int>> spawn_summary;

    /* [snj]: information of joins occured in current execution sequence
       thread_id(t) -> thread_ids that joined in t -> event index of the corresponding join event
    */
    std::unordered_map<IPid, std::unordered_map<IPid, int>> join_summary;

    /* object base -> object offset -> value
        maps object to current value in memory
    */
    std::unordered_map<std::pair<unsigned, unsigned>, int, HashFn> mem;

    /* object base -> object offset -> event id
        maps object to event that performed the latest write on the object
    */
   std::unordered_map<std::pair<unsigned, unsigned>, IID<IPid>, HashFn> last_write;
    /* object base -> object offset -> Visible (vpo)
        maps object to its visible-partial-order
    */
    std::unordered_map<std::pair<unsigned, unsigned>, Visible, HashFn> visible;

    /* [snj]: state corresponding to execution sequence prefix */
    std::vector<int> prefix_state;

    /* [snj]: sequence of event ids representing the current trace prefix */
    Sequence execution_sequence;

    /* [snj]: sequence of events to explore from current state */
    EventSequence to_explore;

    /* [snj]: forbidden values for the current trace */
    SOPFormula forbidden;
};

#endif
