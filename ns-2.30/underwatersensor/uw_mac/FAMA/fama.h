#ifndef __FAMA_H__
#define __FAMA_H__

#include <packet.h>
#include <random.h>

#include <timer-handler.h>

#include <mac.h>
#include "../underwatermac.h"
#include "../underwaterchannel.h"
#include "../underwaterpropagation.h"


#include <queue>
#include <vector>
#include <set>
using namespace std;

typedef double Time;
#define CALLBACK_DELAY 0.001

class FAMA;

struct hdr_FAMA{
	nsaddr_t SA;
	nsaddr_t DA;

	enum PacketType {
		RTS,	//the previous forwarder thinks this is DATA-ACK
		CTS,
		FAMA_DATA,
		ND		//neighbor discovery. need know neighbors, so it can be used as next hop.
	} packet_type;

	static int offset_;
	inline static int& offset() {  return offset_; }

	inline static hdr_FAMA* access(const Packet*  p) {
		return (hdr_FAMA*) p->access(offset_);
	}

};

class FAMA_ND_Timer: public TimerHandler{
public:
	FAMA_ND_Timer(FAMA* mac): TimerHandler() {
		mac_ = mac;
		nd_times_ = 4;
	}
protected:
	FAMA* mac_;
	int	 nd_times_;
	virtual void expire(Event* e);
};


class FAMA_BackoffTimer: public TimerHandler{
public:
	FAMA_BackoffTimer(FAMA* mac): TimerHandler() {
		mac_ = mac;
	}

protected:
	FAMA* mac_;
	virtual void expire(Event* e);
};


class FAMA_StatusHandler: public Handler{
public:
	FAMA_StatusHandler(FAMA* mac): Handler() {
		mac_ = mac;
	}

protected:
	FAMA* mac_;
	virtual void handle(Event* e);
};



class FAMA_WaitCTSTimer: public TimerHandler{
public:
	FAMA_WaitCTSTimer(FAMA* mac): TimerHandler() {
		mac_ = mac;
	}
protected:
	FAMA* mac_;
	virtual void expire(Event* e);
};

class FAMA_DataSendTimer: public TimerHandler{
public:
	FAMA_DataSendTimer(FAMA* mac): TimerHandler() {
		mac_ = mac;
	}
	Packet* pkt_;
protected:
	FAMA* mac_;
	virtual void expire(Event* e);
};


class FAMA_DataBackoffTimer: public TimerHandler{
public:
	FAMA_DataBackoffTimer(FAMA* mac): TimerHandler() {
		mac_ = mac;
	}

protected:
	FAMA* mac_;
	virtual void expire(Event* e);
};



class FAMA_RemoteTimer: public TimerHandler{
public:
	FAMA_RemoteTimer(FAMA* mac): TimerHandler() {
		mac_ = mac;
		ExpiredTime = -1;
	}
	
protected:
	FAMA* mac_;
	Time	ExpiredTime;
	virtual void expire(Event* e);
	friend class FAMA;
};


class FAMA_CallBackHandler: public Handler{
public:
	FAMA_CallBackHandler(FAMA* mac): Handler() {
		mac_ = mac;
	}
protected:
	FAMA* mac_;
	virtual void handle(Event* e);
};

class FAMA: public UnderwaterMac{
public:
	FAMA();
	int  command(int argc, const char*const* argv);
	void TxProcess(Packet* pkt);
	void RecvProcess(Packet* pkt);


protected:

	enum {
		PASSIVE,
		BACKOFF,
		WAIT_CTS,
		WAIT_DATA_FINISH,
		WAIT_DATA,
		REMOTE   /*I don't know what it it means. but 
				 node can only receiving packet in this status*/
	}FAMA_Status;

	Time NDPeriod;
	int  MaxBurst;	//the maximum number of packet burst. default is 1
	Time DataPktInterval;  //0.0001??

	Time EstimateError;		//Error for timeout estimation
	int	 DataSize;
	int  ControlSize;
	int	 neighbor_id; //use this value to pick the next hop one by one

	FAMA_BackoffTimer		backoff_timer;
	FAMA_StatusHandler		status_handler;
	FAMA_ND_Timer			NDTimer;   //periodically send out Neighbor discovery packet for 4 times.
	FAMA_WaitCTSTimer		WaitCTSTimer;
	FAMA_DataBackoffTimer	DataBackoffTimer;
	FAMA_RemoteTimer		RemoteTimer;
	FAMA_CallBackHandler	CallBack_Handler;


	Time MaxPropDelay;
	Time RTSTxTime;
	Time CTSTxTime;



	queue<Packet*>	PktQ_;
	vector<nsaddr_t> NeighborList_;
	set<FAMA_DataSendTimer*> DataSendTimerSet;

	Event	status_event;
	Event	callback_event;

	packet_t UpperLayerPktType;


	Packet* makeND(); //broadcast
	Packet* makeRTS(nsaddr_t Recver);
	Packet* makeCTS(nsaddr_t RTS_Sender);

	void	processND(Packet* pkt);
	void	processRTS(Packet* pkt);

	void	sendRTS(Time DeltaTime);

	void	sendPkt(Packet* pkt);
	void	sendDataPkt();

	void	processDataSendTimer(FAMA_DataSendTimer* DataSendTimer);
	void	processDataBackoffTimer();
	void	processRemoteTimer();


	void	StatusProcess();
	void	backoffProcess();
	void	CallbackProcess(Event* e);


	Time	getTxTimebyPktSize(int PktSize);
	int		getPktSizebyTxTime(Time TxTime);
	bool	CarrierDected();
	void	doBackoff();
	void	doRemote(Time DeltaTime);



	friend class FAMA_BackoffTimer;
	friend class FAMA_ND_Timer;
	friend class FAMA_StatusHandler;
	friend class FAMA_WaitCTSTimer;
	friend class FAMA_DataSendTimer;
	friend class FAMA_DataBackoffTimer;
	friend class FAMA_RemoteTimer;
	friend class FAMA_CallBackHandler;

};

#endif


