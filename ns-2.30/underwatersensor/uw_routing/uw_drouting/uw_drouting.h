#ifndef __uw_drouting_h__
#define __uw_drouting_h__

#include "uw_drouting_pkt.h"
#include "uw_drouting_rtable.h"
#include <agent.h>
#include <packet.h>
#include <trace.h>
#include <timer-handler.h>
#include <random.h>
#include <classifier-port.h>

 #define CURRENT_TIME Scheduler::instance().clock()
 #define JITTER (Random::uniform()*0.5)

 class uw_drouting; // forward declaration

 /* Timers */

 class uw_drouting_PktTimer : public TimerHandler {
 public:
 uw_drouting_PktTimer(uw_drouting* agent, double update_interval) : TimerHandler() {
 agent_ = agent;
 }
 double& update_interval() {
   return update_interval_;
}
 protected:
 uw_drouting* agent_;
 double		update_interval_;
 virtual void expire(Event* e);
 };

 /* Agent */

 class uw_drouting : public Agent {

 /* Friends */
 friend class uw_drouting_PktTimer;

 /* Private members */
 nsaddr_t ra_addr_;
// uw_drouting_state state_;//?????????define state
 //uw_drouting_rtable rtable_;//????????define table 
 uw_drouting_rtable  rtable_;//add by jun
 
 int accessible_var_;
 u_int8_t seq_num_;

 protected:

 PortClassifier* dmux_; // For passing packets up to agents.
 Trace* logtarget_; // For logging.
 uw_drouting_PktTimer pkt_timer_; // Timer for sending packets.

 inline nsaddr_t& ra_addr() { return ra_addr_; }
// inline uw_drouting_state& state() { return state_; }
 inline int& accessible_var() { return accessible_var_; };

 void forward_data(Packet*);
 void recv_uw_drouting_pkt(Packet*);
 void send_uw_drouting_pkt();
 void reset_uw_drouting_pkt_timer();
 double broadcast_jitter(double range);

 public:

 uw_drouting(nsaddr_t);
 int command(int, const char*const*);
 void recv(Packet*, Handler*);
 int coun;

 };

 #endif
