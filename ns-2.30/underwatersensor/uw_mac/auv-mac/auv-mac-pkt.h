#ifndef _AUV_MAC_PKT_H__
#define _AUV_MAC_PKT_H__

#include <packet.h>
#include <random.h>
#include <timer-handler.h>
//#include <iostream>
//using namespace std;
typedef double Time;
namespace AUV
{

//PT_AUV_SYNC
//Hello Packet share the stucture of SYNC

struct hdr_SYNC{
	Time cycle_period_;
	Time& cycle_period() {
		return cycle_period_;
	}

	inline static size_t size() {
		return 8*sizeof(Time);
	}

	static int offset_;
	inline static int& offset()
	{
		return offset_;
	}
	inline static hdr_SYNC* access(const Packet* p) 
	{
		//cout << "hdr_SYNC*)p->access(offset_) offset:"<< offset_<< endl;
		return (hdr_SYNC*)p->access(offset_);
	}
};

//PT_AUV_ML
struct hdr_MissingList{
	uint16_t	node_num_;
	nsaddr_t*	addr_list_;

	inline size_t size() {
		return 8*(sizeof(uint16_t) + node_num_*sizeof(nsaddr_t));
	}

	hdr_MissingList(): node_num_(0), addr_list_(NULL) {
	}

	~hdr_MissingList() {
		delete []addr_list_;
	}

	static int offset_;
	inline static int& offset() {
		return offset_;
	}

	inline static hdr_MissingList* access(const Packet* p) {
		return (hdr_MissingList*)p->access(offset_);
	}
};
//HELLO packet is same as hdr_SYNC

//MISSING LIST is implemented in the data area of packet


class AUV_MAC;
struct ScheduleTime;

class AUV_MAC_WakeTimer: public TimerHandler {
public:
	AUV_MAC_WakeTimer(AUV_MAC* mac, ScheduleTime* ScheT);

protected:
	AUV_MAC* mac_;
	ScheduleTime* ScheT_;
	virtual void expire(Event* e);
};

/*
 * Data structure for neighbors' schedule
 */


struct ScheduleTime {
	ScheduleTime* next_;
	Time		SendTime_;
	nsaddr_t	node_id_;  //with this field, we can determine that which node will send packet.
	AUV_MAC_WakeTimer	timer_;  //necessary
	//Packet*  pkt_;    //the packet this node should send out

	ScheduleTime(Time SendTime, nsaddr_t node_id, AUV_MAC* mac):
			next_(NULL), SendTime_(SendTime), node_id_(node_id), timer_(mac, this) {
	}

	~ScheduleTime() {
		if( timer_.status() == TimerHandler::TIMER_PENDING )
			timer_.cancel();
	}

	void start(Time Delay) {
		if( Delay >= 0.0 )
			timer_.resched(Delay);
	}

};



/*The SendTime in SYNC should be translated to absolute time 
 *and then insert into ScheduleQueue
 */
class ScheduleQueue{
private:
	ScheduleTime* Head_;
	AUV_MAC* mac_;
public:
	ScheduleQueue(AUV_MAC* mac): mac_(mac) {
		Head_ = new ScheduleTime(0.0, 0, NULL);
	}

	~ScheduleQueue() {
		ScheduleTime* tmp;
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

	ScheduleTime* top();//NULL is returned if the queue is empty
	void pop();
	bool checkGuardTime(Time SendTime, Time GuardTime, Time MaxTxTime); //the efficiency is too low, I prefer to use the function below
	Time getAvailableSendTime(Time StartTime, Time OriginalSchedule, Time GuardTime, Time MaxTxTime);
	void clearExpired(Time CurTime);
	void print(Time GuardTime, Time MaxTxTime, bool IsMe, nsaddr_t index);
};


#endif

}


