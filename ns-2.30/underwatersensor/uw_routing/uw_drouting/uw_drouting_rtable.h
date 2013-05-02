#ifndef __uw_drouting_rtable_h__
#define __uw_drouting_rtable_h__

#include <trace.h>
#include <map>
#include <ip.h>

struct DN {
nsaddr_t first;
nsaddr_t second;
};


//typedef std::map<nsaddr_t, int> DN;//should be defined
class uw_drouting;
typedef std::map<nsaddr_t, DN> rtable_t;//should be defined

class uw_drouting_rtable {

friend class uw_drouting;
 rtable_t rt_;
 int chg;
 nsaddr_t node_id_;

 public:

 uw_drouting_rtable();
 
 nsaddr_t& node_id() {
   return node_id_;
  }
 
 void print(nsaddr_t id);
 
 void clear();
 
 void rm_entry(nsaddr_t);
 
 //void add_entry(nsaddr_t, nsaddr_t); 
 
 void add_entry(nsaddr_t, DN);//add by jun
 
 void update(rtable_t*, nsaddr_t); // add by jun

 //nsaddr_t lookup(nsaddr_t);
 nsaddr_t lookup(nsaddr_t);

 u_int32_t size();

 int ifchg ();
 };

 #endif
