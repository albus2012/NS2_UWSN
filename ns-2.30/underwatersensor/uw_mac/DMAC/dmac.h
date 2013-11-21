#ifndef ns_dmac_h
#define ns_dmac_h



#include "packet.h"
#include <random.h>
#include <timer-handler.h>
#include "../underwatermac.h"
#include "../underwaterchannel.h"
#include "../underwaterpropagation.h"
#include <mac.h>

#include <map>
#include <deque>
#include <set>
#include <algorithm>
using namespace std;




typedef double Time;

#define DMAC_CALLBACK_DELAY 0.001
#define INFINITE_PERIOD   10000000.0


enum DmacPacketType
{
  DP_DATA,
  DP_END,
  DP_ACKDATA,
};

struct NodeInfo
{
  Time delay;
  Time cycle;
  Time base;
  Time listenInterval;
};



struct hdr_dmac
{
  int ptype;     //packet type
  int pk_num;    // sequence number
  int data_num;
  bool ack;     // if set true, the receiver should send the ack back immediately
  int sender_addr;  //original sender' address
  Time t_make;           // Timestamp when pkt is generated.
  int receiver_addr;
  Time base;
  Time interval;
  Time t_send;
  Time cycle_period;
  //Time& cycle_period() { return cycle_period; }
  inline static size_t size() {return 8*sizeof(struct hdr_dmac);}
  static int offset_;
  inline static int& offset() { return offset_; }
  inline static hdr_dmac* access(const Packet*  p)
  {
    return (hdr_dmac*) p->access(offset_);
  }
};

class DMac;
struct ScheTime;

class DMac_WakeTimer: public TimerHandler {
public:

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
  bool isPending()
  {
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


struct ScheTime {
  ScheTime* next_;
  Time    SendTime_;
  nsaddr_t  node_id_;  //with this field, we can determine that which node will send packet.
  DMac_WakeTimer  timer_;

  ScheTime(Time SendTime, nsaddr_t node_id, DMac* mac)
    :   next_(NULL), SendTime_(SendTime),
      node_id_(node_id), timer_(mac, this) {
  }

  ~ScheTime() {
    if( timer_.isPending())

      timer_.cancel();
  }

  void start(Time Delay) {
    if( Delay >= 0.0 )
      timer_.resched(Delay);
  }

};



/*The SendTime in SYNC should be translated to absolute time
 *and then insert into ScheQueue
 */
class ScheQueue{
private:
  ScheTime* Head_;
  DMac* mac_;
public:
  ScheQueue(DMac* mac): mac_(mac) {
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
  void clearExpired(Time CurTime);
  void print(Time GuardTime, Time MaxTxTime, bool IsMe, nsaddr_t index);
};

class DMac_CallbackHandler: public Handler{
public:
	DMac_CallbackHandler(DMac* mac) {
		mac_ = mac;
	}
	void handle(Event*);
private:
	DMac* mac_;
};

class DMac_StatusHandler: public Handler{
 public:
	DMac_StatusHandler(DMac* mac){
		 mac_ = mac;
	 }
	void handle(Event*);
 private:
  	DMac* mac_;
};


class DMac_SleepTimer: public TimerHandler {
public:
	DMac_SleepTimer(DMac* mac): TimerHandler() {
		mac_ = mac;
	}
	bool IsPending() {
		if( status() == TIMER_PENDING )
			return true;
		else
			return false;
	}
protected:
	DMac* mac_;
	virtual void expire(Event* e);
};



class DMac_StartTimer: public TimerHandler {
 public:
	DMac_StartTimer(DMac* mac): TimerHandler() {
		mac_ = mac;
	}
 protected:
  virtual void expire(Event* e);

 protected:
	DMac* mac_;

};






class DMac: public UnderwaterMac
{

 public:
	DMac();

	virtual  void RecvProcess(Packet*);
	virtual  void TxProcess(Packet*);
	virtual  int  command(int argc,const char* const* argv);

 protected:

	void	CallbackProcess(Event* e);
	void	StatusProcess(Event* e);



	DMac_CallbackHandler callback_handler;
	Event callback_event;
	Event status_event;
	DMac_StatusHandler  status_handler;


	DMac_WakeTimer wake_timer;	//wake this node after NextCyclePeriod;
	DMac_SleepTimer		sleep_timer;
	DMac_StartTimer		start_timer_;

	Packet* makeSYNCPkt(Time CyclePeriod, nsaddr_t Recver = MAC_BROADCAST); //perhaps CyclePeriod is not required

	void	wakeup(nsaddr_t node_id);  //perhaps I should calculate the energy consumption in these two functions
	void	idle();

	void	setSleepTimer(Time Interval);     //keep awake for To, and then fall sleep
	void	start();	//initilize NexCyclePeriod_ and the sleep timer, sendout first SYNC pkt

	void  sendDataPkt(Packet* p, bool ack);
	void	send_info();
	int  updateNumPktSend() {return NumPktSend_++; }


 private:


	void makeData(Packet* p);
	void canSend();
	void sendData(Packet* p);
	void sendEndFlag();
	Packet* makeEndPkt();

	void handleRecvPkt(Packet* p);
	Packet* makeACK(Packet* p);
	void handleRecvData(Packet* p);
	void canSendACK(Packet* p);
	void setNodeListen(){ nodeFlag = NF_LISTEN;}


	static Time  sendInterval;
	static Time  maxPropTime;
	static Time  baseTime;
	static int   nodeCount;


	ScheQueue	WakeSchQueue_;

	int		CycleCount;   //count the number of cycle.
	int		NumPktSend_;
	int   NumDataSend_;


	typedef int NodeID;
	map<NodeID, NodeInfo> neighbors_;//neighbor nodes info

  //packet queue which this node cache the packet from upper layer
	deque<Packet* >  waitingPackets_;


	enum NodeFlag
	{
	  NF_LISTEN,
	  NF_SEND,
	};

	NodeFlag nodeFlag;
  friend class DMac_CallbackHandler;
  friend class DMac_WakeTimer;
  friend class DMac_SleepTimer;

  friend class DMac_StatusHandler;

  friend class DMac_StartTimer;
  friend class DMac_TxStatusHandler;

};




#endif





















