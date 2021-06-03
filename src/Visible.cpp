#include "Visible.h"

Visible::Visible(unsigned o_id, IID<IPid> init_event): object_id(o_id), low_iid(init_event){
    assert(init_event.get_pid() == 0);
    std::vector<IID<IPid>> init{init_event}; //init_event passed in constructor since that's  when an object is first seen
    mpo.push_back(init);
    IID<MPid> m(0,0);
    low_mid = m;
}

void Visible::add_thread(){
    assert(mpo.size() > 0);

    std::vector<IID<IPid>> mpo_branch;
    mpo.push_back(mpo_branch);

    for(int i = 0; i < visible_start.size(); i++){
        visible_start[i].push_back(0);  //0 stands for init event
    }

    std::vector<unsigned> new_thread(visible_start.size() + 1, 0);
    visible_start.push_back(new_thread);
    init_visible.push_back(true);
}

void Visible::add_enabled_write(IID<IPid> ew){
    assert(ew.get_pid() > 0 );
    //add event in mpo
    mpo[ew.get_pid()].push_back(ew);

    //visible_start is not defined for thread 0, so indices are off by 1 to pid
    visible_start[ew.get_pid() - 1][ew.get_pid() - 1] = mpo[ew.get_pid()].size();

    //strike out low in the thread ew got enabled from
    if(low_mid.get_pid() == 0) init_visible[ew.get_pid() - 1] = false; //if init event simply change visible flag

    else if(visible_start[ew.get_pid() - 1][low_mid.get_pid() - 1] < low_mid.get_index() + 1 )
        visible_start[ew.get_pid() - 1][low_mid.get_pid() - 1] = low_mid.get_index() + 1;//else set index to event after low
}

void Visible::execute_read(unsigned er_proc_id,IID<IPid> new_low){
    assert(er_proc_id > 0);
    if( low_iid == new_low ) //if last executed write is same as last observed write
        return;

    assert(new_low.get_pid() > 0);

    //strike out old low in the read event's thread
    if(low_mid.get_pid() == 0) init_visible[er_proc_id - 1] = false; //for init event
    else if(visible_start[er_proc_id - 1][low_mid.get_pid() - 1] < low_mid.get_index() + 1)
        visible_start[er_proc_id - 1][low_mid.get_pid() - 1] = low_mid.get_index() + 1;//for other than init event

    low_iid = new_low;
    std::vector<IID<IPid>>::iterator it;
    //find new low in the mpo, can use binary search since IIDs would be sorted in mpo
    it = lower_bound(mpo[new_low.get_pid()].begin(),mpo[new_low.get_pid()].end(), new_low);
    IID<MPid> m(new_low.get_pid(), it - mpo[new_low.get_pid()].begin() + 1);
    low_mid = m;

    //strike out all events in order before new low
    if(visible_start[er_proc_id - 1][low_mid.get_pid() - 1] < low_mid.get_index() )
        visible_start[er_proc_id - 1][low_mid.get_pid() - 1] = low_mid.get_index();

}

bool Visible::check_init_visible(int pid){
    assert(pid > 0);
    if(pid - 1 >= init_visible.size()) llvm::outs()<<init_visible.size()<<"sizenotok\n";
    if(! init_visible[pid - 1]) {return false;}

    //if a thread has moved on from 0, it means init is striked out
    for(int i = 0; init_visible[pid - 1] && i < visible_start[pid - 1].size(); i++){
      if(visible_start[pid - 1][i] != 0) init_visible[pid - 1] = false;
    }


    return init_visible[pid - 1];
}
