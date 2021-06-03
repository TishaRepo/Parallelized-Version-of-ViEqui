#include <config.h>
#ifndef __VIEW_EQ_TRACE_BUILDER_H__
#define __VIEW_EQ_TRACE_BUILDER_H__

#include <unordered_map>

#include "TSOPSOTraceBuilder.h"
#include "SymEv.h"
#include "SOPFormula.h"
#include "Visible.h"

typedef llvm::SmallVector<SymEv, 1> sym_ty;

class ViewEqTraceBuilder : public TSOPSOTraceBuilder
{
public:
    int round; //[snj]: TODO temporary
    ViewEqTraceBuilder(const Configuration &conf = Configuration::default_conf);
    virtual ~ViewEqTraceBuilder() override;

    virtual bool schedule(int *proc, int *type, int *alt, bool *doexecute) override;
    virtual void refuse_schedule() override;
    virtual void metadata(const llvm::MDNode *md) override;
    virtual void mark_available(int proc, int aux = -1) override;
    virtual void mark_unavailable(int proc, int aux = -1) override;
            bool is_enabled(int thread_id);
    virtual NODISCARD bool full_memory_conflict() override;
    virtual NODISCARD bool join(int tgt_proc) override;
    virtual NODISCARD bool spawn() override;

    virtual bool reset() override;
    virtual IID<CPid> get_iid() const override;
    virtual void cancel_replay() override;
    virtual bool is_replaying() const override;
    virtual Trace *get_trace() const override;
    virtual void debug_print() const override;

    //[nau]: added virtual function definitions for the sake of compiling
    //[snj]: added some more to the list
    virtual NODISCARD bool store(const SymData &ml) override;
    virtual NODISCARD bool atomic_store(const SymData &ml) override;
    virtual NODISCARD bool compare_exchange(const SymData &sd, const SymData::block_type expected, bool success) override;
    virtual NODISCARD bool load(const SymAddrSize &ml) override;
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
    typedef int IPid;

    class Sequence;

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
        unsigned object;

        Event() {}
        Event(SymEv sym) {symEvent.push_back(sym);}
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
        IID<IPid> get_iid() const {return iid;}
        int get_id() {return iid.get_index();}
        IPid get_pid() {return iid.get_pid();}


        std::vector<Event> unexploredInfluencers(Sequence &seq);
        std::vector<Event> exploredInfluencers(Sequence &seq);
        std::vector<Event> exploredWitnesses(Sequence &seq);
        Sequence prefix(Sequence &seq);
        std::vector<Event> poPrefix(Sequence &seq);

        std::string to_string() const;
        std::string type_to_string() const;
        inline std::ostream &operator<<(std::ostream &os){return os << to_string();}
        inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os){return os << to_string();}

        bool operator==(Event event);
        bool operator!=(Event event);
        // std::string   operator<<() {to_string();}
    };

    std::unordered_map<unsigned,Visible> visible;//vpo for each object, can be accessed by referencing index 'e.object'

    class Thread;

    class Sequence
    {
    private:
        // [snj]: required by Sequence::cmerge function
        std::tuple<Sequence, Sequence, Sequence> join(Sequence &primary, Sequence &other, IID<IPid> delim, Sequence &joined);
        // [snj]: projects tuple projectsions on the thress sequqnces respectively
        void project(std::tuple<Sequence, Sequence, Sequence> &triple, Sequence &seq1, Sequence &seq2, Sequence &seq3);

    public:
        std::vector<IID<IPid>> events;
        std::vector<Thread> *threads;

        Sequence(){}
        Sequence(std::vector<Thread>* t){threads = t;}
        Sequence(std::vector<IID<IPid>> &seq, std::vector<Thread>* t){events = seq; threads = t;}

        bool empty() {return (size() == 0);}
        std::size_t size() const {return events.size();}
        IID<IPid> last() {return events.back();}
        std::vector<IID<IPid>>::iterator begin() {return events.begin();}
        std::vector<IID<IPid>>::iterator end() {return events.end();}
        IID<IPid> head() {return events.front();}
        Sequence tail() {
            std::vector<IID<IPid>> tl(events.begin()+1, events.end());
            Sequence stl(tl,threads);
            return stl;
        }

        void push_back(IID<IPid> event) {events.push_back(event);}
        void pop_back() {events.pop_back();}
        void pop_front() {events.erase(events.begin());};
        void clear() {events.clear();}
        bool has(IID<IPid> event) {return std::find(events.begin(), events.end(), event) != events.end();}

        void concatenate(Sequence seq) { events.insert(events.end(), seq.events.begin(), seq.events.end()); }
        bool hasRWpairs(Sequence &seq);
        bool conflits_with(Sequence &seq);

        IID<IPid>& operator[](std::size_t i) {return events[i];}
        const IID<IPid>& operator[](std::size_t i) const {return events[i];}

        bool isPrefix(Sequence &seq);
        Sequence prefix(IID<IPid> ev);
        Sequence suffix(IID<IPid> ev);
        Sequence suffix(Sequence &seq);
        Sequence poPrefix(IID<IPid> ev);
        Sequence commonPrefix(Sequence &seq);
        bool conflicting(Sequence &other_seq);
        Sequence backseq(IID<IPid> e1, IID<IPid> e2);
        // [snj]: consistent merge, merges 2 sequences such that all read events maitain their sources
        //          i.e, reads-from relation remain unchanged
        Sequence cmerge(Sequence &other_seq);

    };
    typedef std::vector<IID<IPid>>::iterator sequence_iterator;


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

        Event& operator[](std::size_t i) {return events[i];}
        const Event& operator[](std::size_t i) const {return events[i];}
    };

    // [snj]: sequence of event ids
    Sequence execution_sequence;

    // [snj]: dummy event
    Event no_load_store;

    std::vector<Thread>
        threads;
    /* [rmnt]: The CPids of threads in the current execution. */
    CPidSystem CPS;

    class State
    {
        // index of sequence explored corresponding to current state
        int sequence_prefix;
        // std::unordered_set leads;
        // std::unordered_set done;

    public:
        bool hasUnexploredEvents();
        bool hasUnexploredRWpair();
        std::pair<Event,Event> unexploredRWpair();
    };

    bool schedule(int *proc);

    IPid ipid(int proc, int aux) const
    {
        return proc;
    };

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

    /* Are we currently replaying the events given in prefix from the
    * previous execution? Or are we executing new events by arbitrary
    * scheduling?
    */
    bool replay;

    /* The number of events that were or are going to be replayed in the
    * current computation.
    */
    int replay_point;

    /* The latest value passed to this->metadata(). */
    const llvm::MDNode *last_md;

    /* [snj]: Thread id of the thr(current event) */
    int current_thread;

    /* [snj]: current event */
    Event current_event;

    /* [snj]: set of enabled events ie next events of each thread */
    // list of (thread id, next event) pairs
    std::vector<IID<IPid>> Enabled;

    // [snj]: memory map object to last stored value
    std::unordered_map<unsigned, int> mem;
    std::unordered_map<unsigned, IID<IPid>> last_write;
    std::unordered_set<IID<IPid>> unexploredInfluencers(Event er, SOPFormula<IID<IPid>>& f);
    std::unordered_set<IID<IPid>> exploredInfluencers(Event er, SOPFormula<IID<IPid>> &f);
    std::unordered_set<IID<IPid>> exploredWitnesses(Event ew, SOPFormula<IID<IPid>> &f);

};

#endif
