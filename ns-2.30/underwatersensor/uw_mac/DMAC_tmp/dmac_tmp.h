#ifndef ns_dmac_h
#define ns_dmac_h



#include "packet.h"
#include "dmac_base.h"
//#include "uwbuffer.h"
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
#define PRE_WAKE_TIME    1


enum DmacPacketType
{
  DP_DATA,
  P_REV,
  P_ACKREV,
  P_ACKDATA,
  DP_SYNC
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
  int block_num; // the block num, in real world, one bit is enough
  int sender_addr;  //original sender' address
  Time t_make;           // Timestamp when pkt is generated.
  int receiver_addr;
  Time duration;
  Time interval;
  Time t_send;
  Time ts;
  Time cycle_period;
  //Time& cycle_period() { return cycle_period; }
  inline static size_t size() {return 8*sizeof(Time);}
  static int offset_;
  inline static int& offset() { return offset_; }
  inline static hdr_dmac* access(const Packet*  p)
  {
    return (hdr_dmac*) p->access(offset_);
  }
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


class DMac_PktSendTimer: public TimerHandler {
public:
	DMac_PktSendTimer(DMac* mac): TimerHandler() {
		mac_ = mac;
	}

	Time& tx_time() {
		return tx_time_;
	}

	//Packet*& pkt() {
	//	return p_;
	//}
public:
	Packet* p_;
protected:
	DMac* mac_;
	Time	tx_time_;
	//Packet* p_;
	virtual void expire(Event* e);
};


class DMac_StartTimer: public TimerHandler {
public:
	DMac_StartTimer(DMac* mac): TimerHandler() {
		mac_ = mac;
	}

protected:
	DMac* mac_;
	virtual void expire(Event* e);
};






class DMac: public UnderwaterMac
{

 public:
	DMac();

	virtual  void RecvProcess(Packet*);
	/*
	 * AUV MAC assumes that node knows when it will send out the next packet.
	 * To simulate such pre-knowledge, we should not send out the outgoing packet in TxProcess(),
	 * but just queue it and then send out it according to the Schedule via sendoutPkt().
	 */
	virtual  void TxProcess(Packet*);
	virtual  int  command(int argc,const char* const* argv);

 protected:
	void	sendFrame(Packet* p, bool IsMacPkt, Time delay = 0.0);
	void	CallbackProcess(Event* e);
	void	StatusProcess(Event* e);
	void	TxPktProcess(Event* e, DMac_PktSendTimer* pkt_send_timer);


	DMac_CallbackHandler callback_handler;
	Event callback_event;
//	DMac_PktSendTimer	pkt_send_timer;
	Event status_event;
	DMac_StatusHandler  status_handler;


	DMac_WakeTimer wake_timer;	//wake this node after NextCyclePeriod;
	DMac_SleepTimer		sleep_timer;
	DMac_StartTimer		start_timer_;

	Packet* makeSYNCPkt(Time CyclePeriod, nsaddr_t Recver = MAC_BROADCAST); //perhaps CyclePeriod is not required
	//Packet* fillMissingList(Packet* p);
	//Packet* fillSYNCHdr(Packet* p, Time CyclePeriod);

	void	wakeup(nsaddr_t node_id);  //perhaps I should calculate the energy consumption in these two functions
	void	sleep();
	//void	sendoutPkt(Time NextCyclePeriod);
	//bool	setWakeupTimer(); //if the node still need to keep wake, return false.
	void	setSleepTimer(Time Interval);     //keep awake for To, and then fall sleep
	void	start();	//initilize NexCyclePeriod_ and the sleep timer, sendout first SYNC pkt
	Time	genNxCyclePeriod();   //I want to use normal distribution
	//void	processMissingList(unsigned char* ML, nsaddr_t src);
	void	SYNCSchedule(bool initial = false);

	void	send_info();


 private:
	//set<nsaddr_t> CL_;				//contact list

	/*the difference between CL_ and neighbors_ is the Missing list*/
	Time  NextCyclePeriod_;		//next sending cycle
	Time  AvgCyclePeriod_;
	Time  StdCyclePeriod_;

	/*  The length of initial cycle period, whenever I send current
	 *  packet, I should first decide when I will send out next one.
	 */
	static Time  InitialCyclePeriod_;
	static Time  ListenPeriod_;		//the length of listening to the channel after transmission.
	static Time  MaxTxTime_;
	static Time  MaxPropTime_;		//GuardTime_;		//2* static Time  MaxPropTime_;
	static Time  hello_tx_len;
	static Time  WakePeriod_;

	ScheQueue	WakeSchQue_;
	/*
	 *packet queue which this node cache the packet from upper layer
	 */

	int		CycleCounter_;   //count the number of cycle.
	int		NumPktSend_;
	uint		next_hop_num;

	set<DMac_PktSendTimer *> PktSendTimerSet_;

	typedef int NodeID;
	map<NodeID, NodeInfo> neighbors_;//neighbor nodes info
	map<NodeID, deque<Packet* > > waitingPackets_;
  friend class DMac_CallbackHandler;
  friend class DMac_WakeTimer;
  friend class DMac_SleepTimer;
  friend class DMac_StatusHandler;
  friend class DMac_PktSendTimer;
  friend class DMac_StartTimer;
  friend class DMac_TxStatusHandler;
  //friend DMac_SendPktTimer;
};




#endif





















