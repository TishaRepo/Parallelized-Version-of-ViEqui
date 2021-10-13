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


class ViewEqTraceBuilder : public TSOPSOTraceBuilder
{
protected:
    class Event;
    class Thread;
    class Sequence;
    class Lead;
    class State;

public:
    typedef int IPid;
    // typedef std::pair<unsigned, unsigned> Object; // [snj]: TODO use this

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

    // current value in memory for an object
    int current_value(std::pair<unsigned, unsigned> obj);

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
    // check if the causal dependence all events in the lead start remains the same for all extensions
    bool is_independent_EW_lead(Sequence& start);
    // perform union of state leads with pending EW_leads
    void add_EW_leads(int state);

    // forward and backward analysis for builfing view-starts
    void forward_analysis(Event event, SOPFormula& forbidden);
    void backward_analysis_read(Event event, SOPFormula& forbidden, std::unordered_map<int, std::vector<Lead>>& L);
    void backward_analysis_write(Event event, SOPFormula& forbidden, std::unordered_map<int, std::vector<Lead>>& L);
    void backward_analysis(Event event, SOPFormula& forbidden);

    // combining new leads with existing leads of a state
    void consistent_union(int state, Lead& l);
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
    // [snj]: TODO check whether we can do without the above listed functions

    /* [rmnt]: I have just copied their comment for now. TODO Write our own
    * Records a symbolic representation of the current event.
    *
    * During replay, events are checked against the replay log. If a mismatch is
    * detected, a nondeterminism error is reported and the function returns
    * false.
    *
    * Otherwise returns true.
    */
    // [snj]: record symbolic event for event
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
        
        Event() {}
        Event(SymEv sym) {symEvent.push_back(sym); md = 0;}
        Event(const IID<IPid> &iid, sym_ty sym = {}) : iid(iid), symEvent(std::move(sym)), md(0){};
        void make_spawn();
        void make_join();
        void make_read();
        void make_write();
        SymEv sym_event() const {return symEvent[0];}

        bool is_spawn() {return type == SPAWN;}
        /* is read of shared object */
        bool is_read() {return (symEvent.size()==1 && sym_event().addr().addr.block.is_global() && type == READ);}
        /* is write of shared object */
        bool is_write() {return (symEvent.size()==1 && sym_event().addr().addr.block.is_global() && type == WRITE);}
        bool is_global() {return (symEvent.size()==1 && sym_event().addr().addr.block.is_global());}
        bool same_object(Event e);
        bool RWpair(Event e);

        IID<IPid> get_iid() const {return iid;}
        int get_id() {return iid.get_index();}
        IPid get_pid() {return iid.get_pid();}
        int get_id() const {return iid.get_index();}
        IPid get_pid() const {return iid.get_pid();}


        std::vector<Event> unexploredInfluencers(Sequence &seq);
        std::vector<Event> exploredInfluencers(Sequence &seq);
        std::vector<Event> exploredWitnesses(Sequence &seq);
        Sequence prefix(Sequence &seq);

        std::string to_string() const;
        std::string type_to_string() const;
        inline std::ostream &operator<<(std::ostream &os){return os << to_string();}
        inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os){return os << to_string();}

        bool operator==(Event event);
        bool operator!=(Event event);
        // std::string   operator<<() {to_string();}
    };

    typedef std::vector<IID<IPid>>::iterator sequence_iterator;
    
    class Sequence
    {
    public:
        std::vector<IID<IPid>> events;
        std::vector<Thread> *threads;

        Sequence(){assert(false);}
        Sequence(std::vector<Thread>* t){threads = t;}
        Sequence(std::vector<IID<IPid>> &seq, std::vector<Thread>* t){events = seq; threads = t;}
        Sequence(IID<IPid>& e, std::vector<Thread>* t) {events.push_back(e); threads = t;}
        Sequence(std::unordered_set<IID<IPid>> a, std::vector<Thread>* t) {events.insert(events.begin(), a.begin(), a.end()); threads = t;}
        
        void update_threads(std::vector<Thread>* t) {threads = t;}
        std::vector<Event> to_event_sequence();

        bool empty() {return (size() == 0);}
        std::size_t size() const {return events.size();}
        IID<IPid> last() {if (empty()) return IID<IPid>(-1,-1); return events.back();}
        std::vector<IID<IPid>>::iterator begin() {return events.begin();}
        std::vector<IID<IPid>>::iterator end() {return events.end();}
        IID<IPid> head() {return events.front();}
        Sequence tail() {
            Sequence tl(events, threads);
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
        int  value_of(IID<IPid> event);
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
        /* program ordered prefix upto end of ev in this */
        std::pair<std::pair<bool, bool>, Sequence> po_prefix(IID<IPid> e1, IID<IPid> e2, sequence_iterator begin, sequence_iterator end);
        /* program ordered prefix upto end of ev in this */
        Sequence po_prefix_master(IID<IPid> e1, IID<IPid> e2, sequence_iterator begin, sequence_iterator end);
        
        /* return read -> value map of reads in sequence 
           (maps a read to 0(init value) if a write for the read is not in the sequence) */
        std::unordered_map<IID<IPid>, int> read_value_map();
        /* return read -> value and write -> value maps of events in sequence 
           (maps a read to 0(init value) if a write for the read is not in the sequence) 
           Also returns the object -> value map representing the final values of objects 
           after the sequence, modified from within the sequence */
        void event_value_map(std::unordered_map<IID<IPid>, int>& read_value_map, 
                             std::unordered_map<IID<IPid>, int>& write_value_map,
                             std::unordered_map<unsigned, std::unordered_map<unsigned, int>>& post_memory);
        /* In the sequence attempt to shift events from thread of e1 (including e1)
           to after e2. If cannot shift an event coherently then retun false and keep
           original sequence. */
        bool view_adjust(IID<IPid> e1, IID<IPid> e2);
        /* has events in this and other that occur in reverse order */
        bool conflicts_with(Sequence &other_seq);
        std::pair<bool, std::pair<IID<IPid>, IID<IPid>>> conflicts_with(Sequence &other_seq, bool returnRWpair);
        /* poprefix(e1).(write out of e1, e2).(read out of e1, e2) */
        Sequence backseq(IID<IPid> e1, IID<IPid> e2);

        std::string to_string();

    };
    Sequence empty_sequence;


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
    private:
        // [snj]: required by consistent_merge function
        std::tuple<Sequence, Sequence, Sequence> join(Sequence &primary, Sequence &other, IID<IPid> delim, Sequence &joined);
        // [snj]: projects tuple projectsions on the thress sequqnces respectively
        void project(std::tuple<Sequence, Sequence, Sequence> &triple, Sequence &seq1, Sequence &seq2, Sequence &seq3);

    public:
        Sequence constraint;              // sequence from previous trace to be maintained
        Sequence start;                   // new sequence to be explore to get the intended (read,value) pair
        SOPFormula forbidden;             // (read,value) pairs that must not be explored
        Sequence merged_sequence;         // consistent_merge(start, constraint) sequence to explore while maintaining constraint
        bool is_done = false;             // whether the lead has been explored

        // (read -> value read) map on read events of merged sequence
        std::unordered_map<IID<IPid>, int> read_value_map; 
        // (write -> value written) map on write events of merged sequence
        std::unordered_map<IID<IPid>, int> write_value_map; 
        // (object -> value) memory map after lead
        std::unordered_map<unsigned, std::unordered_map<unsigned, int>> post_lead_memory_map;
        
        Lead() {}
        Lead(Sequence c, Sequence s, SOPFormula f) {
            constraint = c; start = s; forbidden = f;
            merged_sequence = consistent_merge(s, c);
            merged_sequence.event_value_map(read_value_map, write_value_map, post_lead_memory_map);

            llvm::outs() << "read map:\n";
            for (auto it = read_value_map.begin(); it != read_value_map.end(); it++) 
                llvm::outs() << it->first << " --> " << it->second << "\n";
            llvm::outs() << "write map:\n";
            for (auto it = write_value_map.begin(); it != write_value_map.end(); it++) 
                llvm::outs() << it->first << " --> " << it->second << "\n";
        }
        Lead(Sequence s, SOPFormula f) {
            start = s; forbidden = f;
            merged_sequence = s;
            merged_sequence.event_value_map(read_value_map, write_value_map, post_lead_memory_map);

            llvm::outs() << "read map:\n";
            for (auto it = read_value_map.begin(); it != read_value_map.end(); it++) 
                llvm::outs() << it->first << " --> " << it->second << "\n";
            llvm::outs() << "write map:\n";
            for (auto it = write_value_map.begin(); it != write_value_map.end(); it++) 
                llvm::outs() << it->first << " --> " << it->second << "\n";
        }
        Lead(Sequence s, SOPFormula f, std::unordered_map<IID<IPid>, int> rv_map, 
                                        std::unordered_map<IID<IPid>, int> wv_map,
                                        std::unordered_map<unsigned, std::unordered_map<unsigned, int>> memory_map) {
            start = s; forbidden = f;
            merged_sequence = s;
            read_value_map = rv_map;
            write_value_map = wv_map;
            post_lead_memory_map = memory_map;
        }

        /* this is prefix of l with view-adjustment */
        bool VA_isprefix(Lead& l);
        /* same reads and same values of reads */
        bool VA_equivalent(Lead& l);
        /* suffix of a view-adjusted prefix */
        Sequence VA_suffix(Lead& l);
        
        bool operator==(Lead l);
        std::ostream &operator<<(std::ostream &os){return os << to_string();}
        
        // [snj]: consistent merge, merges 2 sequences such that all read events maitain their sources
        //          i.e, reads-from relation remain unchanged
        Sequence consistent_merge(Sequence &primary_seq, Sequence &other_seq);
        std::string to_string();
    };

    /* [snj]: dummy event */
    Event no_load_store;
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

        State() {}
        State(int prefix_idx) : sequence_prefix(prefix_idx) {};
        // State(int i, Lead a, SOPFormula f) {sequence_prefix = i; alpha = a; forbidden = f; alpha_sequence = alpha.start + alpha.constraint;};
        
        bool has_unexplored_leads();
        std::vector<Lead> unexplored_leads();
        Lead next_unexplored_lead();

        bool alpha_empty() {return (alpha == -1);}
        Sequence alpha_sequence() {if (alpha == -1) {llvm::outs() << "MUST NOT BE HERE\n"; return Sequence();} return leads[alpha].merged_sequence;}
        std::string print_leads();
    };

    /* stream for debug printing on stdout */
    llvm::raw_ostream &out = llvm::outs();

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
    std::unordered_map<IID<IPid>, std::vector<int>>  covered_read_values;

    /* [snj]: a set of leads that generated from different extensions
       of a EW read for a value v.
       state -> read -> value -> list of leads to get the rv pair (read,value)
    */
    std::unordered_map<int, std::unordered_map<IID<IPid>, std::unordered_map<int, std::vector<Lead>>>> EW_leads;

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
    Sequence to_explore;

    /* [snj]: forbidden values for the current trace */
    SOPFormula forbidden;

    /* [snj]: initial values of objects 
       object -> value
    */
    // std::unordered_map<Object, int> INIT_VALUES; // [snj]: TODO use this
};

#endif
