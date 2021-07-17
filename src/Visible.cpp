#include "Visible.h"

Visible::Visible(IID<IPid> init_event, int init_val): low_iid(init_event){
    assert(init_event.get_pid() == 0);
    first_read_after_join = true;
    std::vector<std::pair<IID<IPid>, int>> init{std::make_pair(init_event, init_val)}; //init_event passed in constructor since that's  when an object is first seen
    mpo.push_back(init);
    IID<MPid> m(0,0);
    low_mid = m;
}

void Visible::add_thread(){
    assert(mpo.size() > 0);

    std::vector<std::pair<IID<IPid>, int>> mpo_branch;
    mpo.push_back(mpo_branch);

    for(int i = 0; i < visible_start.size(); i++){
        visible_start[i].push_back(0);  //0 stands for init event
    }

    std::vector<unsigned> new_thread(visible_start.size() + 1, 0);
    visible_start.push_back(new_thread);
    init_visible.push_back(true);
}

void Visible::add_enabled_write(IID<IPid> ew, int val){
    assert(ew.get_pid() > 0 );
    //add event in mpo
    mpo[ew.get_pid()].push_back({ew, val});
    // strike out writes before ew in its own thread po
    //visible_start is not defined for thread 0, so indices are off by 1 to pid
    visible_start[ew.get_pid() - 1][ew.get_pid() - 1] = mpo[ew.get_pid()].size();

}

void Visible::execute_read(unsigned er_proc_id,IID<IPid> new_low, int val){
    if(er_proc_id == 0){
        if(first_read_after_join){
            first_read_after_join = false;
            return;
        }
        else return;
    }

    if( low_iid == new_low ){ //if last executed write is same as last observed write
        if(visible_start[er_proc_id - 1][low_mid.get_pid() - 1] < low_mid.get_index() )
            visible_start[er_proc_id - 1][low_mid.get_pid() - 1] = low_mid.get_index();
        return;
    }


    assert(new_low.get_pid() > 0);

    //check if there exists another write of the same value as ew
    bool sameValExists = false;
    for(int i = 0; i < visible_start[er_proc_id - 1].size(); i++){
        if(i == new_low.get_pid() - 1) continue;

        for(int j = visible_start[er_proc_id - 1][i]; j < mpo[i + 1].size(); j++){
            if(mpo[i + 1][j].second == val) {
                sameValExists = true;
                break;
            }
        }
    }

    //strike out old low in the read event's thread
    if(low_mid.get_pid() == 0) init_visible[er_proc_id - 1] = false; //for init event
    else if(visible_start[er_proc_id - 1][low_mid.get_pid() - 1] < low_mid.get_index() + 1)
        visible_start[er_proc_id - 1][low_mid.get_pid() - 1] = low_mid.get_index() + 1;//for other than init event

    low_iid = new_low;
    
    //find new low in the mpo, can use binary search since IIDs would be sorted in mpo
    auto it = lower_bound(mpo[new_low.get_pid()].begin(),mpo[new_low.get_pid()].end(), std::make_pair(new_low,0), 
    [](std::pair<IID<IPid>, int> p1, std::pair<IID<IPid>, int> p2){
        return p1.first < p2.first;
    });
    IID<MPid> m(new_low.get_pid(), it - mpo[new_low.get_pid()].begin() + 1);
    low_mid = m;

    //if same value event exists from other thread strike init only
    if(sameValExists) init_visible[er_proc_id - 1] = false;
    //else strike out all events in order before new low 
    else if(!sameValExists && visible_start[er_proc_id - 1][low_mid.get_pid() - 1] < low_mid.get_index() )
        visible_start[er_proc_id - 1][low_mid.get_pid() - 1] = low_mid.get_index();

}

bool Visible::check_init_visible(int pid){
    assert(pid > 0);
    if( pid == 0 ){
        for(int i = 1; i < mpo.size(); i++)
            if(mpo[i].size() != 0) return false;
        return true;
    } 
    if(pid >= init_visible.size() + 1) {llvm::outs()<<pid<<"  "<<init_visible.size()<<"sizenotok\n"; assert(false);}
    if(! init_visible[pid - 1]) {return false;}
    
    //if a thread has moved on from 0, it means init is striked out
    for(int i = 0; init_visible[pid - 1] && i < visible_start[pid - 1].size(); i++){
      if(visible_start[pid - 1][i] != 0) init_visible[pid - 1] = false;
    }

    return init_visible[pid - 1];
}
