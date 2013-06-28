#ifndef _DMAC_BASE_H__
#define _DMAC_BASE_H__

#include <packet.h>
#include <random.h>
#include <timer-handler.h>
//#include <iostream>
//using namespace std;
typedef double Time;



class DMac;
struct ScheTime;

class DMac_WakeTimer: public TimerHandler {
public:
  //DMac_WakeTimer(DMac* mac, ScheTime* ScheT);

  DMac_WakeTimer(DMac* mac,ScheTime* ScheT)
    :TimerHandler()
  {
    mac_ = mac;
    ScheT_ = ScheT;
  }
  DMac_WakeTimer()
   :TimerHandler()
  {
    mac_ = NULL;
    ScheT_ = NULL;
  }
  DMac_WakeTimer(const DMac_WakeTimer & rhs)
   :TimerHandler(), mac_(rhs.mac_),ScheT_(rhs.ScheT_)
  {
    mac_ = rhs.mac_;
    ScheT_ = rhs.ScheT_;
  }

  bool isPending() {
    if( status() == TIMER_PENDING )
      return true;
    else
      return false;
  }
protected:
  DMac* mac_;
  ScheTime* ScheT_;
  virtual void expire(Event* e);
};

/*
 * Data structure for neighbors' schedule
 */

class ScheTime {
public:
  ScheTime* next_;
  Time    SendTime_;
  nsaddr_t  node_id_;  //with this field, we can determine that which node will send packet.
  DMac_WakeTimer  timer_;  //necessary
  //Packet*  pkt_;    //the packet this node should send out

  ScheTime(Time SendTime, nsaddr_t node_id, DMac* mac)
    :   next_(NULL), SendTime_(SendTime),
        node_id_(node_id), timer_(mac, this)
  {}

  ~ScheTime() {
    if( timer_.isPending() )

      timer_.cancel();
  }

  void start(Time Delay) {
    if( Delay >= 0.0 )
      timer_.resched(Delay);
  }

};



/*The SendTime in SYNC should be translated to absolute time
 *and then insert into ScheduleQueue
 *//*The SendTime in SYNC should be translated to absolute time
 *and then insert into ScheQueue
 */
class ScheQueue{
private:
  ScheTime* Head_;
  DMac* mac_;
public:
  ScheQueue(DMac* mac)
    : mac_(mac)
  {
    Head_ = new ScheTime(0.0, 0, NULL);
  }


  ~ScheQueue() {
    ScheTime* tmp;
    while( Head_ != NULL ) {
      tmp = Head_;
      Head_ = Head_->next_;
      delete tmp;
    }
  }

public:

  //first parameter is the time when sending next packet,
  //the last one is the time interval between current time and sending time
  void push(Time SendTime, nsaddr_t node_id, Time Interval);

  ScheTime* top();//NULL is returned if the queue is empty
  void pop();
  bool checkGuardTime(Time SendTime, Time GuardTime, Time MaxTxTime); //the efficiency is too low, I prefer to use the function below
  Time getAvailableSendTime(Time StartTime, Time OriginalSchedule, Time GuardTime, Time MaxTxTime);
  void clearExpired(Time CurTime);
  void print(Time GuardTime, Time MaxTxTime, bool IsMe, nsaddr_t index);
};


#endif


