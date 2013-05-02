#include "uw_drouting_rtable.h"

uw_drouting_rtable::uw_drouting_rtable() { }


void uw_drouting_rtable::print(nsaddr_t id) {
  printf("%f node %d Routing Table:\n", NOW, id);
  for (rtable_t::iterator it = rt_.begin(); it != rt_.end(); it++) {
  printf("%d\t%d\t%d\t%d\n",
  id,
  (*it).first,
  (*it).second.first,
	it->second.second  );
 }
}


 int uw_drouting_rtable::ifchg (){

      return chg;

}

void uw_drouting_rtable::clear() {
   rt_.clear();
}

void uw_drouting_rtable::rm_entry(nsaddr_t dest) {
  rt_.erase(dest);
 }

void uw_drouting_rtable::add_entry(nsaddr_t dest, DN next) {
  rt_[dest] = next;
 }

//nsaddr_t uw_drouting_rtable::lookup(nsaddr_t dest) {
nsaddr_t uw_drouting_rtable::lookup(nsaddr_t dest) {
   rtable_t::iterator it = rt_.find(dest);
   if (it == rt_.end())
	  return (nsaddr_t)IP_BROADCAST;
   else
	  return (*it).second.first;//add by jun
}


 u_int32_t uw_drouting_rtable::size() {
  return rt_.size();
 }
 
 
void uw_drouting_rtable::update(rtable_t* newrt, nsaddr_t Source_N) {    //add by jun
  
   DN tp;
   nsaddr_t tmp;
   chg=0;

   	
   	  if (lookup(Source_N) == (nsaddr_t)IP_BROADCAST){
   	    tp.first= Source_N;	    
	    tp.second=1;	    
	    add_entry(Source_N, tp);
	    chg=1;
	  }

		  for( rtable_t:: iterator it=newrt->begin(); it !=newrt->end(); it++)  {       
  
				if( it->first == node_id() ) 
				  continue;
				
				if (lookup((*it).first) != (nsaddr_t)IP_BROADCAST){

					  tmp = lookup((*it).first);             	     
	    
					  if (rt_[it->first].second > (*it).second.second +1 ){	      
						rm_entry((*it).first);	      
						tp.first=Source_N;
						tp.second=(*it).second.second+1;
						add_entry((*it).first, tp);  
						chg=1;
	    
					  }  	    
    
				  }  
				  else{
					tp.first= Source_N;
					tp.second=(*it).second.second + 1;
					add_entry((*it).first, tp);
					chg=1;
	    
				  }      
	    	    
  
			}

    
}


