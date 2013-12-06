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
#include <vector>
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

class DMac_WakeTimer: public TimerHandler {
public:

  DMac_WakeTimer(DMac* mac)
    :TimerHandler(),mac_ (mac)
  {}

  DMac_WakeTimer()
   :TimerHandler(),mac_(NULL)
  {}

  DMac_WakeTimer(const DMac_WakeTimer & rhs)
   :TimerHandler(), mac_(rhs.mac_)
  {}

  bool isPending()
  {
    if( status() == TIMER_PENDING )
      return true;
    else
      return false;
  }

protected:
  DMac* mac_;
  virtual void expire(Event* e);
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

class Topology
{
public:
  struct Point
  {
    int id;
    int x;
    int y;
    int z;
  };
  Topology(int n);
  void addNode(int id, int x, int y, int z);

  double getDelay(int i, int j);
  int getNextNode(int i);
  vector<int> getPath();

private:
  void setDelay();
  vector<int> path;
  vector<Point> points;
  vector<vector<double> > delay;
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


	DMac_WakeTimer wake_timer;
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

	int sendout(Packet* p);
	void makeData(Packet* p);
	void canSend();
	void sendData(Packet* p);
	void sendEndFlag();
	Packet* makeEndPkt();

	void handleRecvPkt(Packet* p);
	Packet* makeACK(Packet* p);
	void handleRecvData(Packet* p);
	void canSendACK(Packet* p);

	void addCycleCount() { CycleCount++;}
	int  getCycleCount() { return CycleCount;}

	enum NodeFlag
	{
	  NF_LISTEN,
	  NF_SEND,
	  NF_NODATA,
	  NF_WAIT,
	}nodeFlag;


	void setNodeWaitFlag(){ nodeFlag = NF_WAIT;}
	void setNodeListenFlag(){ nodeFlag = NF_LISTEN;}
	void setNodeSendFlag(){ nodeFlag = NF_SEND;}
	void setNodeNoDataFlag(){ nodeFlag = NF_NODATA;}
	NodeFlag getNodeFlag(){ return nodeFlag;}
	bool isNodeWaitFlag() { return nodeFlag == NF_WAIT;}
	bool isNodeListenFlag() { return nodeFlag == NF_LISTEN;}
	bool isNodeSendFlag() { return nodeFlag == NF_SEND;}
	bool isNodeNoDataFlag() { return nodeFlag == NF_NODATA;}

	static Time  sendInterval;
	static Time  maxPropTime;
	static Time  baseTime;
	static int   nodeCount;
	static Topology topo;

//	ScheQueue	WakeSchQueue_;

	int		CycleCount;   //count the number of cycle.
	int		NumPktSend_;
	int   NumDataSend_;

	typedef int NodeID;
	map<NodeID, NodeInfo> neighbors_;//neighbor nodes info

  //packet queue which this node cache the packet from upper layer
	deque<Packet* >  waitingPackets_;


	struct NodeTime
	{
	  Time startSend;
	  Time endSend;
	}nodeTime;

	Time getRoundTime();
  Time getInitStartTime();
  Time getInitEndTime();
	void initNodeTime();

	void updateNodeTime();

	bool isSendEndFlag();
  Time now();

  friend class DMac_CallbackHandler;
  friend class DMac_WakeTimer;
  friend class DMac_SleepTimer;

  friend class DMac_StatusHandler;

  friend class DMac_StartTimer;
  friend class DMac_TxStatusHandler;

};


#endif





















