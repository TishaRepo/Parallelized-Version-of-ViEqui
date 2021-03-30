#include "ViewEqTraceBuilder.h"

ViewEqTraceBuilder::ViewEqTraceBuilder(const Configuration &conf) : TSOPSOTraceBuilder(conf)
{
    threads.push_back(Thread(CPid(), -1));
    prefix_idx = -1;
}

bool ViewEqTraceBuilder::schedule(int *proc, int *aux, int *alt, bool *dryrun)
{
    *dryrun = false;
    *alt = 0;
    const unsigned size = threads.size();
    unsigned i;

    for (i = 0; i < size; i += 2)
    {
        if (conf.max_search_depth < 0 || threads[i].event_indices.size() < conf.max_search_depth)
        {
            threads[i].event_indices.push_back(++prefix_idx);
            Event event = Event(IID<IPid>(IPid(i), threads[i].event_indices.size()));
            *proc = i / 2;
            *aux = -1;
            return true;
        }
    }

    return false;
}