#ifndef __UWALOHA_H__
#define __UWALOHA_H__

#include <packet.h>
#include <random.h>
#include <math.h>

#include <timer-handler.h>

#include <mac.h>
#include "../underwatermac.h"
#include "../underwaterchannel.h"
#include "../underwaterpropagation.h"


#include <queue>
#include <map>

using namespace std;



typedef double Time;
#define CALLBACK_DELAY 0.001	//the interval between two consecutive sendings
#define MAXIMUMCOUNTER 4
#define Broadcast -1
class UWALOHA;
enum PacketType {
  ALOHA_DATA,
  ALOHA_ACK
};

struct hdr_UWALOHA{
  int ptype;
	nsaddr_t SA;
	nsaddr_t DA;


	static int offset_;
	inline static int& offset() {  return offset_; }
	
	inline static int size() {
		return sizeof(nsaddr_t)*2 + 1; /*for packet_type*/;
	}

	inline static hdr_UWALOHA* access(const Packet*  p) {
		return (hdr_UWALOHA*) p->access(offset_);
	}

};

class UWALOHA_DataSendTimer: public TimerHandler{
public:
	UWALOHA_DataSendTimer(UWALOHA* mac): TimerHandler() {
		mac_ = mac;
	}
	//void resched(double delay);
	Packet* pkt_;

protected:
	UWALOHA* mac_;
	virtual void expire(Event *e);
};

class UWALOHA_WaitACKTimer: public TimerHandler{
public:
	UWALOHA_WaitACKTimer(UWALOHA* mac): TimerHandler() {
		mac_ = mac;
		ack_times_ = 30;
	}
	//void resched(double delay);

protected:
	UWALOHA* mac_;
	int	 ack_times_;
	virtual void expire(Event* e);
};


class UWALOHA_BackoffTimer: public TimerHandler{
public:
	UWALOHA_BackoffTimer(UWALOHA* mac): TimerHandler() {
		mac_ = mac;
	}
	//void resched(double delay);

protected:
	UWALOHA* mac_;
	virtual void expire(Event* e);
};


class UWALOHA_ACK_RetryTimer: public TimerHandler{
public:
	UWALOHA_ACK_RetryTimer(UWALOHA* mac, Packet* pkt=NULL) {
		mac_ = mac;
		pkt_ = pkt;
		id_ = id_generator++;
	}
	
	Packet*& pkt() {
		return pkt_;
	}
	
	long id() {
		return id_;
	}
	
protected:
	UWALOHA* mac_;
	Packet* pkt_;
	long	id_;
	static long id_generator;
	virtual void expire(Event* e);
};


class UWALOHA_StatusHandler: public Handler{
public:
	UWALOHA_StatusHandler(UWALOHA* mac): Handler() {
		mac_ = mac;
	}
	bool&	is_ack() {
		return is_ack_;
	}
protected:
	UWALOHA* mac_;
	bool	is_ack_;
	virtual void handle(Event* e);
};


class UWALOHA_CallBackHandler: public Handler{
public:
	UWALOHA_CallBackHandler(UWALOHA* mac): Handler() {
		mac_ = mac;
	}
protected:
	UWALOHA* mac_;
	virtual void handle(Event* e);
};


class UWALOHA: public UnderwaterMac{
public:
	UWALOHA();
	int  command(int argc, const char*const* argv);
	void TxProcess(Packet* pkt);
	void RecvProcess(Packet* pkt);
	int bo_counter;
	
	void	processRetryTimer(UWALOHA_ACK_RetryTimer* timer);
protected:

	enum UWALOHA_STATUS{
		UWALOHA_PASSIVE,
		UWALOHA_BACKOFF,
		UWALOHA_SEND_DATA,
		UWALOHA_WAIT_ACK,
	}UWALOHA_Status;

	double	Persistent;
	int persize;
	int		ACKOn;
	Time	Min_Backoff;
	Time 	Max_Backoff;
	Time	WaitACKTime;
	Time	MAXACKRetryInterval;
	
	bool	blocked;

	int	seq_n;
	
	UWALOHA_BackoffTimer		BackoffTimer;
	UWALOHA_WaitACKTimer		WaitACKTimer;

	UWALOHA_CallBackHandler		CallBack_handler;
	UWALOHA_StatusHandler		status_handler;

	Time MaxPropDelay;
	Time DataTxTime;
	Time ACKTxTime;


	queue<Packet*>	PktQ_;
	map<long, UWALOHA_ACK_RetryTimer*> RetryTimerMap_;   //map timer id to the corresponding pointer

	Event	status_event;
	Event 	Forward_event;
	Event	callback_event;

	Packet* makeACK(nsaddr_t RTS_Sender);

	void	replyACK(Packet* pkt);

	void	sendACK(Time DeltaTime);

	void	sendPkt(Packet* pkt);//why?
	void	sendDataPkt();
//	void 	DropPacket(Packet*);
	void	doBackoff();
//	void	processDataSendTimer(Event *e);
	void	processWaitACKTimer(Event *e);
	void	processPassive();
//	void	processBackoffTimer();

	
	void	retryACK(Packet* ack);

	void	StatusProcess(bool is_ack);
	void	BackoffProcess();
	void	CallbackProcess(Event* e);

	bool	CarrierDected();
//	void	doBackoff(Event *e);


	friend class UWALOHA_BackoffTimer;
	friend class UWALOHA_WaitACKTimer;
	friend class UWALOHA_CallBackHandler;
	friend class UWALOHA_StatusHandler;
	friend class UWALOHA_ForwardHandler;

};



#endif


