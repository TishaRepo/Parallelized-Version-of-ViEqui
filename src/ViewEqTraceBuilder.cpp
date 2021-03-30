#include "ViewEqTraceBuilder.h"

ViewEqTraceBuilder::ViewEqTraceBuilder(const Configuration &conf) : TSOPSOTraceBuilder(conf)
{
    threads.push_back(Thread(CPid(), -1));
    prefix_idx = -1;
}
/*
    [rmnt] : This function is meant to replace the schedule function used in Interpreter::run in Execution.cpp. It searches for an available real thread (we are storing thread numbers at even indices since some backend functions rely on that). Once it finds one we push its index (prefix_idx is a global index keeping track of the number of events we have executed) to the respective thread's event indices. We also create its event object. Now here we have some work left. What they do is insert this object into prefix. We have to figure out where the symbolic event member of the Event object is being filled up (its empty on initialisation) as that will get us to how they find the type of event. Also, we have to implement certain functions like mark_available, mark_unavailable, reset etc which are required for this to replace the interface of the current schedule function. There are also other functions like metadata() and fence() that we are not sure whether our implementation would need.
    */
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