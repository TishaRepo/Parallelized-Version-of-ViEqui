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

    virtual bool schedule() override;

protected:
    typedef int IPid;

    class Object
    {
        std::string var_name;
    };

    class Sequence
    {
        std::vector<Event> events;

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
}

#endif