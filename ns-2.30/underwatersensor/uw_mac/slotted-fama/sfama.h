#ifndef __sfama_h__
#define __sfama_h__

#include <packet.h>
#include <random.h>
#include <timer-handler.h>

#include <mac.h>
#include "underwatersensor/uw_mac/underwatermac.h"
#include "underwatersensor/uw_mac/underwaterchannel.h"
#include "underwatersensor/uw_mac/underwaterpropagation.h"

#include "sfama-pkt.h"

#include <queue>
#include <vector>
#include <set>
using namespace std;

typedef double Time;

//#define SFAMA_DEBUG

class SFAMA;



class SFAMA_Wait_Send_Timer: public TimerHandler {
  friend class SFAMA;
public:
	SFAMA_Wait_Send_Timer(SFAMA* mac): TimerHandler() {
		mac_ = mac;
		pkt_ = NULL;
	}

	void stop() {
		if( pkt_ != NULL ) {
			Packet::free(pkt_);
			pkt_ = NULL;
		}
		if( this->status() == TIMER_PENDING ) {
			cancel();
		}
	}

protected:
	SFAMA* mac_;
	Packet* pkt_;
	virtual void expire(Event* e);
};


class SFAMA_Wait_Reply_Timer: public TimerHandler {
public:
	SFAMA_Wait_Reply_Timer(SFAMA* mac): TimerHandler() {
		mac_ = mac;
	}

	void stop() {
		if( this->status() == TIMER_PENDING ) {
			cancel();
		}
	}

protected:
	SFAMA* mac_;
	virtual void expire(Event* e);
};


class SFAMA_Backoff_Timer: public TimerHandler{
public:
	SFAMA_Backoff_Timer(SFAMA* mac): TimerHandler() {
		mac_ = mac;
	}

	void stop() {
		if( this->status() == TIMER_PENDING ) {
			cancel();
		}
		
	}

protected:
	SFAMA* mac_;
	virtual void expire(Event* e);
};



class SFAMA_CallBackHandler: public Handler{
public:
	SFAMA_CallBackHandler(SFAMA* mac): Handler() {
		mac_ = mac;
	}
protected:
	SFAMA* mac_;
	virtual void handle(Event* e);
};

class SFAMA_SlotInitHandler: public Handler {
public:
	SFAMA_SlotInitHandler(SFAMA* mac): Handler() {
		mac_ = mac;
	}
protected:
	SFAMA* mac_;
	virtual void handle(Event* e);
};

class SFAMA_MAC_StatusHandler: public Handler{
public:
	SFAMA_MAC_StatusHandler(SFAMA* mac): Handler() {
		mac_ = mac;
	}
	int& slotnum() { return slotnum_; }
protected:
	SFAMA* mac_;
	int		slotnum_;
	virtual void handle(Event* e);
};


class SFAMA_DataSend_Timer: public TimerHandler{
public:
	SFAMA_DataSend_Timer(SFAMA* mac): TimerHandler() {
		mac_ = mac;
	}
protected:
	SFAMA* mac_;
	virtual void expire(Event* e);
};

enum SFAMA_Status{
    IDLE_WAIT,  /*do nothing but just wait*/
    WAIT_SEND_RTS,
    WAIT_SEND_CTS,
    WAIT_RECV_CTS,
    WAIT_SEND_DATA,
    WAIT_RECV_DATA,
    WAIT_SEND_ACK,
    WAIT_RECV_ACK,
    BACKOFF,
	BACKOFF_FAIR
};

class SFAMA: public UnderwaterMac{
public:
	SFAMA();
	int  command(int argc, const char*const* argv);
	// to process the incomming packet
	virtual  void RecvProcess(Packet*);
	// to process the outgoing packet
	virtual  void TxProcess(Packet*);


	enum SFAMA_Status status_;


	void CallBackProcess(Event* e);
	void StatusProcess(int slotnum);

	void WaitSendTimerProcess(Packet* pkt);
	void BackoffTimerProcess();
	void WaitReplyTimerProcess(bool directcall=false);
	void DataSendTimerProcess();

	void initSlotLen();

private:

	int DataSize;
	int ControlSize;
	int MaxPropDelay;
	//index_ is the mac address of this node
	Time guard_time_;  //need to be binded
	Time slot_len_;

	bool is_in_round;
	bool is_in_backoff;

	int max_backoff_slots_;

	int max_burst_; /*maximum number of packets in the train*/
	int data_sending_interval_;


	SFAMA_CallBackHandler callback_handler;
	SFAMA_MAC_StatusHandler status_handler;
	SFAMA_SlotInitHandler slotinit_handler;

	Event	callback_event;
	Event	status_event;	
	Event	slotinit_event;

	//wait to send pkt at the beginning of next slot
	SFAMA_Wait_Send_Timer wait_send_timer; 
	//wait for the corresponding reply. Timeout if fail to get
	SFAMA_Wait_Reply_Timer wait_reply_timer; 
	//if there is a collision or RTS/CTS not for this node, do backoff
	SFAMA_Backoff_Timer    backoff_timer; 

	SFAMA_DataSend_Timer	datasend_timer;

	queue<Packet*> SendingPktQ_;
	queue<Packet*> CachedPktQ_;
	queue<Packet*> Backup_SendingPktQ_;

	//packet_t UpperLayerPktType;
	
protected:
	Packet* makeRTS(nsaddr_t recver, int slot_num);
	Packet* makeCTS(nsaddr_t rts_sender, int slot_num);
	Packet* fillDATA(Packet* data_pkt);
	Packet* makeACK(nsaddr_t data_sender);

	
	void processRTS(Packet* rts_pkt);
	void processCTS(Packet* cts_pkt);
	void processDATA(Packet* data_pkt);
	void processACK(Packet* ack_pkt);

	void doBackoff(int backoff_slotnum);

	void sendPkt(Packet* pkt);
	void sendDataPkt(Packet* pkt);

	void setStatus(enum SFAMA_Status status);
	enum SFAMA_Status getStatus();

	void stopTimers();
	void releaseSentPkts();

	void prepareSendingDATA();

	Time getPktTrainTxTime();

	void scheduleRTS(nsaddr_t recver, int slot_num);


	//Time getTxtimeByPktSize(int pkt_size);
	Time getTime2ComingSlot(Time t);


	int randBackoffSlots();
	
	
#ifdef SFAMA_DEBUG
	void printQ(queue<Packet*>& my_q);
	void printAllQ();
#endif
	




	friend class SFAMA_CallBackHandler;
	friend class SFAMA_Backoff_Timer;
	friend class SFAMA_MAC_StatusHandler;
	friend class SFAMA_Wait_Send_Timer;
	friend class SFAMA_Wait_Reply_Timer;
	friend class SFAMA_DataSend_Timer;

};

#endif
