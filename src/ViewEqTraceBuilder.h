#include <config.h>
#ifndef __VIEW_EQ_TRACE_BUILDER_H__
#define __VIEW_EQ_TRACE_BUILDER_H__

#include "TSOPSOTraceBuilder.h"
#include "SymEv.h"

typedef llvm::SmallVector<SymEv, 1> sym_ty;

class ViewEqTraceBuilder : public TSOPSOTraceBuilder
{
public:
    ViewEqTraceBuilder();
    virtual ~ViewEqTraceBuilder() override;

    virtual bool schedule() override;

protected:
    class Object
    {
        std::string var_name;
    }

    class Sequence
    {
        std::vector<Event> events;

        Sequence &merge(Sequence &other_seq);
    }

    class Event
    {
        int event_id;
        enum event_type
        {
            READ,
            WRITE,
            OTHER // [rmnt] : Include other types of events like those given in Symbolic Events ?
        };
        Object object;
        int thread_id;
        int getValue(Sequence &seq);
        sym_ty symEvent;
        std::vector<Event> unexploredInfluencers(Sequence &seq);
        std::vector<Event> exploredInfluencers(Sequence &seq);
        std::vector<Event> exploredWitnesses(Sequence &seq);
        Sequence prefix(Sequence &seq);
        std::vector<Event> poPrefix(Sequence &seq);
    }
}

#endif