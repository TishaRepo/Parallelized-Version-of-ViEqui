#include <vector>
#include <unordered_set>
#include <iostream>
#include <llvm/Support/raw_ostream.h>

#include "IID.h"
typedef int IPid;
typedef int MPid;

class Visible{

    public:
        Visible(){}
        Visible(IID<IPid> init_event, int init_val); //init event indexed as 0
        std::vector<std::vector<std::pair<IID<IPid>, int>>> mpo; //matrix containing write event iids and values
        std::vector<std::vector<unsigned>> visible_start;
        std::vector<bool> init_visible; //init event visible to thread[i+1] if init_visible[i] is true
        IID<MPid> low_mid;  //low index according to mpo
        IID<IPid> low_iid;  //last observed write event - initialised to init
        bool first_read_after_join;

        void add_thread();
        void execute_read(unsigned er_proc_id, IID<IPid> new_low, int val);
        void add_enabled_write( IID<IPid> ew, int value);
        bool check_init_visible(int pid);
};
