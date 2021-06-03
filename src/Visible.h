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
        Visible(unsigned o_id, IID<IPid> init_event);
        std::vector<std::vector<IID<IPid>>> mpo;
        std::vector<std::vector<unsigned>> visible_start;
        unsigned object_id;
        std::vector<bool> init_visible; //init event visible to thread[i+1] if init_visible[i] is true
        IID<MPid> low_mid;  //low index according to mpo
        IID<IPid> low_iid;  //last observed write event - initialised to init
        
        void add_thread();  
        void execute_read(unsigned er_proc_id, IID<IPid> new_low);
        void add_enabled_write( IID<IPid> ew);
        bool check_init_visible(int pid);
};
