
#include "ConfSpace.hpp"

namespace Slic3r {



ConfSpace::ConfSpace()  {
}

std::pair<ConfSpace::idx_type,bool> 
ConfSpace::insert (const Point& val) {
    idx_type idx;
    if((idx=find(val))>=0)
        return std::make_pair(idx,false);
    idx=vertices.insert(vertices.end(), Vertex(val))-vertices.begin();
    index.insert(index_type::value_type(val, idx));
    return std::make_pair(idx,true);
}
       
ConfSpace::idx_type 
ConfSpace::find(const Point& val) {
    index_type::const_iterator i;
    i=index.find(val);
    if(i==index.end()) return -1;    
    return i->second;
}

}
