#include <config.h>
#ifndef __VIEW_EQ_TRACE_BUILDER_H__
#define __VIEW_EQ_TRACE_BUILDER_H__

#include "TSOPSOTraceBuilder.h"
#include "SymEv.h"

typedef llvm::SmallVector<SymEv, 1> sym_ty;

class ViewEqTraceBuilder : public TSOPSOTraceBuilder
{
public:
    ViewEqTraceBuilder(const Configuration &conf = Configuration::default_conf);
    virtual ~ViewEqTraceBuilder() override;

    virtual bool schedule(int *proc) override;
    virtual void refuse_schedule() override;
    virtual void mark_available(int proc, int aux = -1) override;
    virtual void mark_unavailable(int proc, int aux = -1) override;
    // virtual void cancel_replay() override;
    // virtual bool is_replaying() const override;
    // virtual void metadata(const llvm::MDNode *md) override;
    // virtual bool sleepset_is_empty() const override;
    // virtual bool check_for_cycles() override;
    virtual Trace *get_trace() const override;
    virtual bool reset() override;
    virtual IID<CPid> get_iid() const override;

    virtual void debug_print() const override;

    virtual NODISCARD bool spawn() override;
    virtual NODISCARD bool store(const SymData &ml) override;
    virtual NODISCARD bool atomic_store(const SymData &ml) override;
    virtual NODISCARD bool compare_exchange
    (const SymData &sd, const SymData::block_type expected, bool success) override;
    virtual NODISCARD bool load(const SymAddrSize &ml) override;
    virtual NODISCARD bool full_memory_conflict() override;
    virtual NODISCARD bool fence() override;
    virtual NODISCARD bool join(int tgt_proc) override;
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
    virtual int cond_destroy(const SymAddrSize &ml) override;
    virtual NODISCARD bool register_alternatives(int alt_count) override;
    virtual long double estimate_trace_count() const override;

protected:
    typedef int IPid;

    class Object
    {
        std::string var_name;
    };

    class Sequence
    {
        std::vector<ViewEqTraceBuilder::    Event> events;

        Sequence &merge(Sequence &other_seq);
    };

    class Event
    {
        Object object;

    public:
        Event(const IID<IPid> &iid, sym_ty sym = {}) : symEvent(std::move(sym)), iid(iid){};
        int getValue(Sequence &seq);
        IID<IPid> iid;
        sym_ty symEvent;
        std::vector<Event> unexploredInfluencers(Sequence &seq);
        std::vector<Event> exploredInfluencers(Sequence &seq);
        std::vector<Event> exploredWitnesses(Sequence &seq);
        Sequence prefix(Sequence &seq);
        std::vector<Event> poPrefix(Sequence &seq);
    };

    class Thread
    {
    public:
        Thread(const CPid &cpid, int spawn_event) : cpid(id), available(true), spawn_event(spawn_event){};
        CPid cpid;
        int spawn_event;
        bool available;
        std::vector<unsigned> event_indices;
    };

    int prefix_idx;

    std::vector<Thread>
        threads;

    bool schedule(int *proc, int *aux, int *alt, bool *dryrun);

    IPid ipid(int proc, int aux) const
    {
        assert(-1 <= aux && aux <= 0);
        assert(proc * 2 + 1 < int(threads.size()));
        return aux ? proc * 2 : proc * 2 + 1;
    };
};

#endif