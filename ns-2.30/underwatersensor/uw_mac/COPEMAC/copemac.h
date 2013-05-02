#ifndef __OTMAN_H__
#define __OTMAN_H__

#include <packet.h>
#include <random.h>

#include <timer-handler.h>

#include <mac.h>
#include "../underwatermac.h"
#include "../underwaterchannel.h"
#include "../underwaterpropagation.h"



#include <map>
#include <set>
#include <vector>
#include <queue>
using namespace std;


#define OTMAN_CALLBACK_DELAY 0.001
#define OTMAN_BACKOFF_TIME	 0.01
#define	OTMAN_REV_DELAY		0.5
#define BACKOFF_DELAY_ERROR		1
#define	MAX_INTERVAL		1
#define SEND_DELAY			0.001
#define MAXIMUMCOUNTER		6

typedef double Time;

enum PacketType{
	OTMAN_ND,
	OTMAN_ND_REPLY,
	MULTI_REV,
	MULTI_REV_ACK,
	MULTI_DATA_ACK
};
//neighbor discovery packet header

//struct hdr_nd{
//	Time send_time_;
//	static inline int size() { 
//		return 8; 
//	}
//};
//
//
//struct hdr_nd_reply{
//	Time nd_send_time_;
//	Time delay_at_receiver;
//	static inline int size() {
//		return sizeof(Time)+sizeof(Time); 
//	}
//};
//
//
//struct hdr_ctrl{
//	int size_;
//	inline int size() { return size_; }	
//};


struct hdr_otman{
	PacketType packet_type;
	inline int size() {
		return 1; //return size_;
	};
	/*union{
		hdr_nd ndh;
		hdr_nd_reply nd_reply_h;
		hdr_ctrl  ctrl_h;
	}hdr;*/
	int size_;

	static int offset_;
  	inline static int& offset() { return offset_; }
  	inline static hdr_otman* access(const Packet*  p) {
		return (hdr_otman*) p->access(offset_);
	}
};


//-----------------------------------------------
//save the incoming packets.
//-----------------------------------------------


struct PktElem{
	Packet* pkt_;
	PktElem* next_;
	inline PktElem(): pkt_(NULL), next_(NULL) {
	}
	inline PktElem(Packet* pkt): pkt_(pkt), next_(NULL) {
	}
};

struct PktList{
	PktElem* head;
	PktElem* tail;
	inline PktList(): head(NULL), tail(NULL){
	};
	inline ~PktList() {
		PktElem* temp = NULL;
		while (head != tail ) {
			temp = head;
			head = head->next_;
			Packet::free(temp->pkt_);
			temp->next_ = NULL;
		}
	}
};



class OTMAN;

//all pending packets are stored in this data structure
class PktWareHouse{
	friend class OTMAN;
private:
	map<nsaddr_t, PktList> Queues;
	int		CachedPktNum_;
	bool	locked;
public:
	PktWareHouse():CachedPktNum_(0), locked(false) {
	}
	inline bool IsEmpty() {
		return !CachedPktNum_;
	}
		 
	//inline void lock() {
	//	locked = true;
	//}

	//inline void unlock() {
	//	locked = false;
	//}
	void	insert2PktQs(Packet* p);
	bool	deletePkt(nsaddr_t Recver, int SeqNum);
};


//----------------------------------------------------
//cache the reservation request
struct RevReq{
	nsaddr_t	requestor;
	int			AcceptedRevID;  //if AcceptedRevID and RejctedRevID is not succesive, both are rejected
	int			RejectedRevID;
	Time		StartTime;
	Time		EndTime;
};

struct DataAck{
	nsaddr_t Sender;
	int		 SeqNum;
};
//---------------------------------------------------
class PktSendTimer: public TimerHandler{
public:
	PktSendTimer(OTMAN* mac, Packet* pkt): TimerHandler() {
		mac_ = mac;
		pkt_ = pkt;
	}
	~PktSendTimer();
protected:
	OTMAN* mac_;
	Packet* pkt_;
	void expire(Event* e);
};

enum RevType {
		PRE_REV,	//the interval is in reserved in REV Request
		AVOIDING,	//overheared from rev-ack. cannot receive but can send at this time
		SENDING,	//the interval is for that this node sends packet
		RECVING	//the interval is for receiving packet from other nodes
};

//Reservation Element
struct RevElem{

	Time		StartTime;
	Time		EndTime;
	nsaddr_t	Reservor;   //reserve for which node
	RevType		rev_type;
	int			RevID;

	
	//node may reserve time for itself to send out packet, this timer is used to send packet
	PktSendTimer* send_timer;
	RevElem*	next;
	RevElem();
	RevElem(int RevID_, Time StartTime_, 
		Time EndTime_, nsaddr_t Reservor_, RevType rev_type_);
	~RevElem();
};


class RevQueues{
private:
	RevElem*	Head_;
	OTMAN*		mac_;
public:
	RevQueues(OTMAN* mac);
	~RevQueues();
	/*
	 * If [startTime, EndTime] overlaps with some 
	 * existing reservation time interval,
	 * push will fails and false is returned. 
	 * Otherwise, insert successfully and return true.
	 * If force is true, it will not check availability
	 */
	bool push(int RevID, Time StartTime, Time EndTime, nsaddr_t Reservor, 
		RevType rev_type, Packet* pkt=NULL);
	void	clearExpired(Time ExpireTime);
	bool    checkAvailability(Time StartTime, Time EndTime, RevType rev_type);
	void	deleteRev(int RevID);
	void	updateStatus(int RevID, RevType new_type);
	void	printRevQueue();
	Time	getValidStartTime(Time Interval, Time SinceTime=NOW);  //absolute time
};

//---------------------------------------------------



//handlers
class OTMAN_CallbackHandler: public Handler{
public:
	OTMAN_CallbackHandler(OTMAN* mac) {
		mac_ = mac;
	}
	void handle(Event*);
private:
	OTMAN* mac_;
};


class OTMAN_StatusHandler: public Handler{
public:
	OTMAN_StatusHandler(OTMAN* mac) {
		mac_ = mac;
	}
	void handle(Event*);
private:
	OTMAN* mac_;
};


class OTMAN_SendDelayHandler: public Handler{
public:
	OTMAN_SendDelayHandler(OTMAN* mac) {
		mac_ = mac;
	}
	void handle(Event*);
private:
	OTMAN* mac_;
};

class OTMAN_BackoffHandler: public Handler{
public:
	OTMAN_BackoffHandler(OTMAN* mac) {
		mac_ = mac;
		counter = 0;
	}
	void clear();
	void handle(Event*);
private:
	OTMAN* mac_;
	Packet* pkt_;
	int counter;
	friend class OTMAN;
};


class OTMAN_NDProcess_Initor: public TimerHandler{
public:
	OTMAN_NDProcess_Initor(OTMAN* mac): TimerHandler() {
		mac_ = mac;
		MaxTimes_ = 3;
	}

protected:
	OTMAN* mac_;
	int MaxTimes_;    //the maximum times of delay measurement
	virtual void expire(Event* e);
};


class OTMAN_NDReply_Initor: public TimerHandler{
	public:
	OTMAN_NDReply_Initor(OTMAN* mac): TimerHandler() {
		mac_ = mac;
	}

protected:
	OTMAN* mac_;
	virtual void expire(Event* e);
};


class OTMAN_DataSendTimer: public TimerHandler {
public:
	OTMAN_DataSendTimer(OTMAN* mac): TimerHandler() {
		mac_ = mac;
	}
protected:
	OTMAN* mac_;  //Data pulse
	virtual void expire(Event* e);
};


class OTMAN_RevAckAccumTimer: public TimerHandler {
public:
	OTMAN_RevAckAccumTimer(OTMAN* mac): TimerHandler() {
		mac_ = mac;
	}
	
protected:
	OTMAN* mac_;
	virtual void expire(Event* e);
};


class OTMAN_DataAckAccumTimer: public TimerHandler {
public:
	OTMAN_DataAckAccumTimer(OTMAN* mac): TimerHandler() {
		mac_ = mac;
	}
	
protected:
	OTMAN* mac_;
	virtual void expire(Event* e);
};

class OTMAN_InitiatorTimer: public TimerHandler {
public:
	OTMAN_InitiatorTimer(OTMAN* mac): TimerHandler() {
		mac_ = mac;
	}
protected:
	OTMAN* mac_;
	virtual void expire(Event* e);
};

struct NDRecord{
	Time nd_sendtime;
	Time nd_recvtime;
};

class CtrlPktTimer: public TimerHandler{
public:
	CtrlPktTimer(OTMAN* mac):TimerHandler() {
		mac_ = mac;
	}
	OTMAN* mac_;
	Packet* pkt_;
protected:
	virtual void expire(Event* e);
};


class ctrlpktqueue{
public:
	ctrlpktqueue(OTMAN* mac) {
		mac_ = mac;
	}
	void insert(Packet* ctrl_p, Time delay);
	void clearExpiredElem();
	
protected:
	OTMAN* mac_;
	queue<CtrlPktTimer*>  CtrlQ_;
};

class AckWaitTimer: public TimerHandler{
public:
	AckWaitTimer(): TimerHandler() {
		mac_ = NULL;
	}
	Packet* pkt_;
	OTMAN* mac_;
protected:	
	virtual void expire(Event* e);

};

class PrintDistanceTimer: public TimerHandler{
public:
	PrintDistanceTimer(OTMAN* mac): TimerHandler() {
		mac_ = mac;
	}
protected:
	virtual void expire(Event* e);
	OTMAN* mac_;
};

class OTMAN: public UnderwaterMac{
public:
	OTMAN();
	int  command(int argc, const char*const* argv);
	// to process the incomming packet
	virtual  void RecvProcess(Packet*);
	// to process the outgoing packet
	virtual  void TxProcess(Packet*);
	


protected:
	void	PreSendPkt(Packet* pkt, Time delay=0.0001);  //send out the packet after delay
	void	SendPkt(Packet* pkt);
	void	start();
	//int	round2Slot(Time time);  //round the time to the slot sequence num since now
	//Time	Slot2Time(int SlotNum, Time BaseTime = NOW);
	//Time	round2RecverSlotBegin(Time time, nsaddr_t recver);

	void	startHandShake();
	Time	map2OwnTime(Time SenderTime, nsaddr_t Sender); //map time SenderTime on sender to the time on this node

	void	RecordDataPkt(Packet* pkt);
	//process control packets.
	void	processND(Packet* pkt);
	void	processNDReply(Packet* pkt);
	void	processMultiRev(Packet* pkt);
	void	processMultiRevAck(Packet* pkt);
	void	processDataAck(Packet* pkt);

	//make control packets.
	Packet* makeND();  //ND packet include the neighbors which this node already knows.
	Packet* makeNDReply(); 
	Packet* makeMultiRev();
	Packet* makeMultiRevAck();
	Packet* makeDataAck();

	//for test
	void	printDelayTable();
	void	printResult();


	void	clearAckWaitingList();
	void	insertAckWaitingList(Packet* p, Time delay);

	//hanlder callback functions & Events
	void	CallbackProcess(Event* e);
	void	StatusProcess(Event *e);
	Event	callback_event;
	Event	status_event;
	Event	backoff_event;
	

	//handlers and timers
	OTMAN_CallbackHandler	callback_handler;
	OTMAN_StatusHandler		status_handler;
	OTMAN_SendDelayHandler	senddelay_handler;
	OTMAN_BackoffHandler	backoff_handler;
	OTMAN_NDProcess_Initor	nd_process_initor;  //it's a timer
	OTMAN_NDReply_Initor	ndreply_initor;
	OTMAN_DataSendTimer		DataSendTimer;
	OTMAN_RevAckAccumTimer	RevAckAccumTimer;
	OTMAN_DataAckAccumTimer DataAckAccumTimer;
	OTMAN_InitiatorTimer	InitiatorTimer;
	PrintDistanceTimer		PrintTimer;

private:
	/*Time	PktInterval_;*/	//the interval between sending two packet
	Time	NDInterval_;	//the interval between two successive ND process
	Time	DataAccuPeriod;	//the period of data pulse 
	Time	DataTxStartTime_;
	Time	RevAckAccumTime_;
	Time	DataAckAccumTime_;
	/*length of time slot should be bigger than maximum transmissioin time of packet*/
	//Time	TimeSlotLen_;
	PktWareHouse	PktWH_;	
	
	map<nsaddr_t, Time>	prop_delays;  //the propagation delay to neighbors
	map<nsaddr_t, Time>	nd_receive_time;   //the time when receives ND packet
	map<nsaddr_t, Time>	nd_depart_neighbor_time;    //the time when neighbor send ND to this node
	vector<RevReq*>	 PendingRevs;  //all pending revs are stored here. Schedule it before sending ack
	vector<DataAck*> PendingDataAcks;
	map<nsaddr_t, int>	SucDataNum;   //result is here

	RevQueues	RevQ_;
	uint	next_hop;
	uint	neighbor_id;
	//Time MajorBackupInterval; //int		MaxSlotRange_;  //the interval between majorInterval and backup
	
	//the begin time of MajorInterval will be chosen among the following two values
	//they are the offset based on current time
	Time		MajorIntervalLB;  //lower bound of major interval//RecvSlotLowerRange; 
	
	Time		DataStartTime;
	static int		RevID;
	Time	GuardTime;
	Time	NDWin;
	Time	NDReplyWin;
	map<nsaddr_t, NDRecord> PendingND;

	map<int, AckWaitTimer>	AckWaitingList; //stores the packet is (prepared to) sent out but not receive the ack yet.
	
	Time	AckTimeOut;
	ctrlpktqueue    CtrlPktQ_;
	int		PktSize;

	int		isParallel;
	
	friend class RevQueues;
	friend class OTMAN_CallbackHandler;
	friend class OTMAN_StatusHandler;
	friend class OTMAN_NDProcess_Initor;
	friend class OTMAN_NDReply_Initor;
	friend class OTMAN_DataSendTimer;
	friend class OTMAN_RevAckAccumTimer;
	friend class PktSendTimer;
	friend class OTMAN_DataAckAccumTimer;
	friend class OTMAN_SendDelayHandler;
	friend class OTMAN_BackoffHandler;
	friend class OTMAN_InitiatorTimer;
	friend class AckWaitTimer;
	friend class CtrlPktTimer;
	friend class ctrlpktqueue;
	friend class PrintDistanceTimer;

};

#endif
