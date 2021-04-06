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

    virtual bool schedule(int *proc, int *aux, int *alt, bool *dryrun) override;
    virtual void refuse_schedule() override;
    virtual void metadata(const llvm::MDNode *md) override;
    virtual void mark_available(int proc, int aux = -1) override;
    virtual void mark_unavailable(int proc, int aux = -1) override;

protected:
    typedef int IPid;

    class Object
    {
        std::string var_name;
    };

    class Sequence;
    class Event
    {
        Object object;

    public:
        Event(const IID<IPid> &iid, sym_ty sym = {}) : iid(iid), symEvent(std::move(sym)), md(0){};
        int getValue(Sequence &seq);
        IID<IPid> iid;
        sym_ty symEvent;
        const llvm::MDNode *md;
        std::vector<Event> unexploredInfluencers(Sequence &seq);
        std::vector<Event> exploredInfluencers(Sequence &seq);
        std::vector<Event> exploredWitnesses(Sequence &seq);
        Sequence prefix(Sequence &seq);
        std::vector<Event> poPrefix(Sequence &seq);
    };

    class Sequence
    {
        std::vector<Event> events;

        Sequence &merge(Sequence &other_seq);
    };

    class Thread
    {
    public:
        Thread(const CPid &cpid, int spawn_event) : cpid(cpid), spawn_event(spawn_event), available(true){};
        CPid cpid;
        int spawn_event;
        bool available;
        std::vector<unsigned> event_indices;
    };

    // [rmnt]: Keeping a vector containing all the events which have been executed (and also the ongoing one). Meant to emulate the prefix without needing any WakeupTree functionality.
    std::vector<Event> prefix;

    int prefix_idx;

    // [rmnt]: TODO: Do we need sym_idx? It seems to play an important role in record_symbolic as well as whenever we are replaying

    std::vector<Thread>
        threads;

    bool schedule(int *proc);

    IPid ipid(int proc, int aux) const
    {
        assert(-1 <= aux && aux <= 0);
        assert(proc * 2 + 1 < int(threads.size()));
        return aux ? proc * 2 : proc * 2 + 1;
    };

    Event &curev()
    {
        assert(0 <= prefix_idx);
        assert(prefix_idx < int(prefix.size()));
        return prefix[prefix_idx];
    };
};

#endif