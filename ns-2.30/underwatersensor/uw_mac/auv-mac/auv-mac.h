#ifndef __AUV_MAC_H__
#define __AUV_MAC_H__


#include "auv-mac-pkt.h"
#include <timer-handler.h>
#include <random.h>
#include <mac.h>
#include "../underwatermac.h"
#include "../underwaterchannel.h"
#include "../underwaterpropagation.h"
#include <set>
#include <queue>
#include <algorithm>
using namespace std;
namespace AUV
{

#define AUV_MAC_CALLBACK_DELAY 0.001
#define INFINITE_PERIOD   10000000.0
#define PRE_WAKE_TIME     1

class AUV_MAC;

class AUV_MAC_CallbackHandler: public Handler{
public:
	AUV_MAC_CallbackHandler(AUV_MAC* mac) {
		mac_ = mac;
	}
	void handle(Event*);
private:
	AUV_MAC* mac_;
};


class AUV_MAC_StatusHandler: public Handler{
 public:
	 AUV_MAC_StatusHandler(AUV_MAC* mac){
		 mac_ = mac;
	 }
	void handle(Event*);
 private:
  	AUV_MAC* mac_;
};


class AUV_MAC_SleepTimer: public TimerHandler {
public:
	AUV_MAC_SleepTimer(AUV_MAC* mac): TimerHandler() {
		mac_ = mac;
	}
	bool IsPending() {
		if( status() == TIMER_PENDING )
			return true;
		else
			return false;
	}
protected:
	AUV_MAC* mac_;
	virtual void expire(Event* e);
};


class AUV_MAC_PktSendTimer: public TimerHandler {
public:
	AUV_MAC_PktSendTimer(AUV_MAC* mac): TimerHandler() {
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
	AUV_MAC* mac_;
	Time	tx_time_;
	//Packet* p_;
	virtual void expire(Event* e);
};


class AUV_MAC_StartTimer: public TimerHandler {
public:
	AUV_MAC_StartTimer(AUV_MAC* mac): TimerHandler() {
		mac_ = mac;
	}

protected:
	AUV_MAC* mac_;
	virtual void expire(Event* e);
};


class AUV_MAC: public UnderwaterMac{
	friend class AUV_MAC_CallbackHandler;
	friend class AUV_MAC_WakeTimer;
	friend class AUV_MAC_SleepTimer;
	friend class AUV_MAC_StatusHandler;
	friend class AUV_MAC_PktSendTimer;
	friend class AUV_MAC_StartTimer;
	friend class AUV_MAC_TxStatusHandler;
	//friend AUV_MAC_SendPktTimer;

public:
	AUV_MAC();

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
	void	TxPktProcess(Event* e, AUV_MAC_PktSendTimer* pkt_send_timer);


	AUV_MAC_CallbackHandler callback_handler;
	Event callback_event;
//	AUV_MAC_PktSendTimer	pkt_send_timer;
	Event status_event;
	AUV_MAC_StatusHandler  status_handler;
	

	//AUV_MAC_WakeTimer wake_timer;	//wake this node after NextCyclePeriod;
	AUV_MAC_SleepTimer		sleep_timer;
	AUV_MAC_StartTimer		start_timer_;

	Packet* makeSYNCPkt(Time CyclePeriod, nsaddr_t Recver = MAC_BROADCAST); //perhaps CyclePeriod is not required
	Packet* fillMissingList(Packet* p);
	Packet* fillSYNCHdr(Packet* p, Time CyclePeriod);

	void	wakeup(nsaddr_t node_id);  //perhaps I should calculate the energy consumption in these two functions
	void	sleep();
	void	sendoutPkt(Time NextCyclePeriod);
	//bool	setWakeupTimer(); //if the node still need to keep wake, return false.
	void	setSleepTimer(Time Interval);     //keep awake for To, and then fall sleep
	void	start();	//initilize NexCyclePeriod_ and the sleep timer, sendout first SYNC pkt
	Time	genNxCyclePeriod();   //I want to use normal distribution
	void	processMissingList(unsigned char* ML, nsaddr_t src);
	void	SYNCSchedule(bool initial = false);

	void	send_info();


private:
	set<nsaddr_t> CL_;				//contact list
	set<nsaddr_t> neighbors_;		//neighbor list.
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

	ScheduleQueue	WakeSchQueue_;
	/* 
	 *packet queue which this node cache the packet from upper layer 
	 */
	queue<Packet*>  PacketQueue_;
	int		CycleCounter_;   //count the number of cycle.
	int		NumPktSend_;
	uint		next_hop_num;
	set<AUV_MAC_PktSendTimer *> PktSendTimerSet_;
};




#endif

}
