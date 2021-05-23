#include <config.h>
#ifndef __VIEW_EQ_TRACE_BUILDER_H__
#define __VIEW_EQ_TRACE_BUILDER_H__

#include "TSOPSOTraceBuilder.h"
#include "SymEv.h"

typedef llvm::SmallVector<SymEv, 1> sym_ty;

class ViewEqTraceBuilder : public TSOPSOTraceBuilder
{
public:
    int round;
    ViewEqTraceBuilder(const Configuration &conf = Configuration::default_conf);
    virtual ~ViewEqTraceBuilder() override;

    virtual bool schedule(int *proc, int *aux, int *alt, bool *dryrun) override;
    virtual void refuse_schedule() override;
    virtual void metadata(const llvm::MDNode *md) override;
    virtual void mark_available(int proc, int aux = -1) override;
    virtual void mark_unavailable(int proc, int aux = -1) override;

    //[snj]: split store and load functions to pre and post
    NODISCARD bool store_pre(const SymData &ml);
    NODISCARD bool store_post(const SymData &ml);
    NODISCARD bool load_pre(const SymAddrSize &ml);
    NODISCARD bool load_post(const SymAddrSize &ml);
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
    bool NODISCARD record_symbolic(SymEv event);

protected:
    typedef int IPid;

    class Sequence;

    class Event
    {
    public:
        IID<IPid> iid;
        sym_ty symEvent;
        const llvm::MDNode *md;
        // [snj]: value signifies the value read by a read event or written by a write event in the current
        // execution sequence, all value are initialized to 0 by default
        int value;

        Event() {}
        Event(const IID<IPid> &iid, sym_ty sym = {}) : iid(iid), symEvent(std::move(sym)), md(0) { value = 0; };
        SymEv sym_event() const { return symEvent[0]; }

        bool is_read() { return (sym_event().kind == SymEv::LOAD); }
        bool is_write() { return (sym_event().kind == SymEv::STORE); }
        bool same_object(Event e);

        std::vector<Event> unexploredInfluencers(Sequence &seq);
        std::vector<Event> exploredInfluencers(Sequence &seq);
        std::vector<Event> exploredWitnesses(Sequence &seq);
        Sequence prefix(Sequence &seq);
        std::vector<Event> poPrefix(Sequence &seq);

        std::string to_string() const;
        inline std::ostream &operator<<(std::ostream &os) { return os << to_string(); }
        inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os) { return os << to_string(); }

        bool operator==(Event event);
        bool operator!=(Event event);
    };

    class Sequence
    {
    private:
        // [snj]: required by Sequence::cmerge function
        std::tuple<Sequence, Sequence, Sequence> join(Sequence &primary, Sequence &other, Event delim, Sequence &joined);
        // [snj]: projects tuple projectsions on the thress sequqnces respectively
        void project(std::tuple<Sequence, Sequence, Sequence> &triple, Sequence &seq1, Sequence &seq2, Sequence &seq3);

    public:
        std::vector<Event> events;

        Sequence() {}
        Sequence(std::vector<Event> &seq) { events = seq; }

        bool empty() { return (size() == 0); }
        std::size_t size() const { return events.size(); }
        Event last() { return events.back(); }
        std::vector<Event>::iterator begin() { return events.begin(); }
        std::vector<Event>::iterator end() { return events.end(); }
        Event head() { return events.front(); }
        Sequence tail()
        {
            std::vector<Event> tl(events.begin() + 1, events.end());
            Sequence stl(tl);
            return stl;
        }

        void push_back(Event event) { events.push_back(event); }
        void pop_back() { events.pop_back(); }
        void pop_front() { events.erase(events.begin()); };
        void clear() { events.clear(); }
        bool has(Event event) { return std::find(events.begin(), events.end(), event) != events.end(); }

        void concatenate(Sequence seq) { events.insert(events.end(), seq.events.begin(), seq.events.end()); }
        bool hasRWpairs(Sequence &seq);

        Event &operator[](std::size_t i) { return events[i]; }
        const Event &operator[](std::size_t i) const { return events[i]; }

        // [snj]: consistent merge, merges 2 sequences such that all read events maitain their sources
        //          i.e, reads-from relation remain unchanged
        Sequence cmerge(Sequence &other_seq);
    };
    typedef std::vector<Event>::iterator sequence_iterator;

    class Thread
    {
    public:
        Thread(const CPid &cpid, int spawn_event) : cpid(cpid), spawn_event(spawn_event), available(true){};
        CPid cpid;
        int spawn_event;
        bool available;
        std::vector<unsigned> event_indices;
    };

    // [rmnt]: Keeping a vector containing all the events which have been executed (and also the ongoing one).
    // Meant to emulate the prefix without needing any WakeupTree functionality.
    Sequence execution_sequence;

    std::vector<Thread>
        threads;
    /* [rmnt]: The CPids of threads in the current execution. */
    CPidSystem CPS;

    /*[rmnt]: Taken their ByteInfo object and representation of Memory as a mapping from SymAddr to ByteInfo as it is. 
    * TODO: Do we need all the fields inside ByteInfo (last_update for example)?
    * UPDATE: Commented them out for now, will see if we need them later 
    */
    /* A ByteInfo object contains information about one byte in
   * memory. In particular, it recalls which events have recently
   * accessed that byte.
   */
    class ByteInfo
    {
    public:
        ByteInfo() : {}; //last_update(-1), last_update_ml({SymMBlock::Global(0), 0}, 1){};
        /* An index into prefix, to the latest update that accessed this
     * byte. last_update == -1 if there has been no update to this
     * byte.
     */
        int last_update; //[snj]: TODO might need, check
        /* The complete memory location (possibly multiple bytes) that was
     * accessed by the last update. Undefined if there has been no
     * update to this byte.
     */
        // SymAddrSize last_update_ml;
        /* Set of events that updated this byte since it was last read.
     *
     * Either contains last_update or is empty.
     */
        // [snj]: don't need: VecSet<int> unordered_updates;
        /* last_read[tid] is the index in prefix of the latest (visible)
     * read of thread tid to this memory location, or -1 if thread tid
     * has not read this memory location.
     *
     * The indexing counts real threads only, so e.g. last_read[1] is
     * the last read of the second real thread.
     *
     * last_read_t is simply a wrapper around a vector, which expands
     * the vector as necessary to accomodate accesses through
     * operator[].
     */

        // [snj]: don't need: struct last_read_t
        // {
        //     std::vector<int> v;
        //     int operator[](int i) const { return (i < int(v.size()) ? v[i] : -1); };
        //     int &operator[](int i)
        //     {
        //         if (int(v.size()) <= i)
        //         {
        //             v.resize(i + 1, -1);
        //         }
        //         return v[i];
        //     };
        //     std::vector<int>::iterator begin() { return v.begin(); };
        //     std::vector<int>::const_iterator begin() const { return v.begin(); };
        //     std::vector<int>::iterator end() { return v.end(); };
        //     std::vector<int>::const_iterator end() const { return v.end(); };
        // } last_read;
    };
    std::map<SymAddr, ByteInfo> mem;

    bool schedule(int *proc);

    IPid ipid(int proc, int aux) const
    {
        return proc;
    };

    Event &curev()
    {
        assert(0 <= prefix_idx);
        assert(prefix_idx < int(prefix.size()));
        return execution_sequence[prefix_idx]; //at(prefix_idx);
    };

    const Event &curev() const
    {
        assert(0 <= prefix_idx);
        assert(prefix_idx < int(prefix.size()));
        return execution_sequence[prefix_idx];
    };

    /* The index into prefix corresponding to the last event that was
    * scheduled. Has the value -1 when no events have been scheduled.
    */
    // [snj]: index in execution sequence aka index size of sequence
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
};

#endif
