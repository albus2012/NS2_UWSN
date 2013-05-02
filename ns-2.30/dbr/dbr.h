/** 
 *  Depth based routing alogrithm header file.
 *  Author: Hai Yan, University of Connecticut CSE, March, 2007
 */

#ifndef	_DBR_H_
#define	_DBR_H_

#include <deque>

#include <stdarg.h>

#include <object.h>
#include <agent.h>
#include <trace.h>
#include <packet.h>
#include <mac.h>
#include <mobilenode.h>
#include <classifier-port.h>

#include "pkt_cache.h"

#define	DBR_PORT		0xFF

#define	DBR_BEACON_DESYNC	0.1		// desynchronizing form for alive beacons
#define	DBR_BEACON_INT		10		// interval between beacons
#define	JITTER			1		// jitter for broadcasting

class DBR_Agent;
class MyPacketQueue;
class NeighbEnt;
class NeighbTable;
class hdr_dbr;

#if	0
struct DBRPacket {
	int dest;
	int src;
	Packet *pkt;	// inner NS packet

	DBRPacket() : pkt(NULL) {}
	DBRPacket(Packet *p, hdr_dbr *dbrh) : 
			pkt(p) {}
};
#endif

class DBR_AgentTimer : public TimerHandler {
public:
	DBR_AgentTimer(DBR_Agent *a) { a_ = a; }
	virtual void expire(Event *e) = 0;

protected:
	DBR_Agent *a_;
};

class DBR_BeaconHandler : public Handler {
public:
	DBR_BeaconHandler(DBR_Agent *a) { a_ = a; }
	virtual void handle(Event *e);

private:
	DBR_Agent *a_;
};

class DBR_BeaconTimer : public DBR_AgentTimer {
public:
	DBR_BeaconTimer(DBR_Agent *a) : DBR_AgentTimer(a) {}
	virtual void expire(Event *e); 
};

class DBR_SendingTimer : public DBR_AgentTimer {
public:
	DBR_SendingTimer(DBR_Agent *a) : DBR_AgentTimer(a) {}
	virtual void expire(Event *e); 
};

class QueueItem {
public:
	QueueItem() : p_(0), send_time_(0) {}
	QueueItem(Packet *p, double t) : p_(p), send_time_(t) {}

	Packet *p_;		// pointer to the packet
	double send_time_;	// time to send the packet 
};

class MyPacketQueue {
public:
	MyPacketQueue() : dq_() {}
	~MyPacketQueue() { dq_.clear(); }

	bool empty() { return dq_.empty(); }
	int size() { return dq_.size(); }
	void dump();

	void pop() { dq_.pop_front(); }; 
	QueueItem* front() { return dq_.front(); };
	void insert(QueueItem* q);
	bool update(Packet *p, double t);
	bool purge(Packet *p);

private:
	deque<QueueItem*> dq_;
};

class NeighbEnt {
public:
	NeighbEnt(DBR_Agent* ina) : 
		x(0.0), y(0.0), z(0.0), routeFlag(0) {}	
	// the agent is used for timer object

	double x, y, z;		// location of neighbor, actually we only need depth info
	nsaddr_t net_id;    // IP of neighbor 
	int routeFlag;		// indicates that a routing path exists

	// user timer 
	//DBR_DeadNeighbTimer dnt;	// timer for expiration of neighbor

};

class NeighbTable {
public:
	NeighbTable(DBR_Agent *a); 
	~NeighbTable();

	void dump(void);
	void ent_delete(const NeighbEnt *e);        	// delete an neighbor
	NeighbEnt *ent_add(const NeighbEnt *e);     	// add an neighbor
	NeighbEnt *ent_find_shadowest(MobileNode *mn);  // find the neighbor with minimal depth
	void updateRouteFlag(nsaddr_t, int); 

private:
	int num_ents;       // number of entries in use
	int max_ents;       // capacity of the table
	DBR_Agent *a_;       // agent owns the table
	NeighbEnt **tab;    // neighbor table
};

#define	DBRH_DATA_GREEDY	0
#define	DBRH_DATA_RECOVER	1
#define	DBRH_BEACON		2

class hdr_dbr {
public:
	static int offset_;		// offset of this header
	static int& offset() { return offset_; }
	static hdr_dbr* access(const Packet *p) {
		return (hdr_dbr*)p->access(offset_); 
	}

	double x, y, z;	

	int& packetID() { return packetID_; }
	
	int& valid() { return valid_; }
	int& mode()  { return mode_; }
	int& nhops() { return nhops_; }
	nsaddr_t& prev_hop() { return prev_hop_; }
	nsaddr_t& owner() { return owner_; }

	double& depth() { return depth_; }

	// get the header size 
	int size() 
	{ 	int sz;
		sz = 4 * sizeof(int); 
		sz += 3 * sizeof(double);
		return sz;
	}

private:
	int packetID_;		// unique id for packet

	int valid_;		// is this header in the packet?
	int mode_;		// routing mode: greedy | recovery
	int nhops_;		// max # of hops to broadcast
				// in recovery mode
	nsaddr_t prev_hop_;	// the sender
	nsaddr_t owner_;	// from whom the sender got it

	double depth_;		// the depth of last hop
};

class DBR_Agent : public Tap, public Agent {
	friend class DBR_BeaconHandler;
	friend class DBR_BeaconTimer;
	friend class DBR_SendingTimer;

public:
	DBR_Agent();
	~DBR_Agent();

	virtual int command(int argc, const char*const* argv);
	virtual void recv(Packet *, Handler *);
	virtual void recv2(Packet *, Handler *);

	virtual void tap(const Packet *p);

	void deadneighb_callback(NeighbEnt *ne); 
	void beacon_callback(void);
	void send_callback(void);
	
	//DBR_BeaconHandler bhdl;

protected:
	Trace *traceagent;	// Trace agent 
	PortClassifier *dmux;

	int off_mac_;
	int off_ll_;
	int off_ip_;
	int off_dbr_;		// offset of DBR packet header in pkt
		
	double bint_;		// beacon interval
	double bdesync_;	// desynchronizing term

	double latest_;		// latest time to send the packet

	NsObject *ll;				// our link layer output
	//NsObject *port_dmux;
	MobileNode *mn_;			// MobileNode associated with the agent
	NeighbTable *ntab_;			// neighbor entry table
	DBR_BeaconTimer *beacon_timer_;		// beacon timer
	DBR_SendingTimer *send_timer_;		// sending timer

	PktCache *pc_;				// packet cache for broadcasting;
	MyPacketQueue pq_;			// packet queue
	int pkt_cnt_;				// counter for packets have been sent

	inline void send_to_dmux(Packet *p, Handler *h) {
		dmux->recv(p, h);
	}

	void forwardPacket(Packet *, int = 0);
	void init(void);
	void trace(char* fmt, ...);
	Packet* makeBeacon(void);
	void sendBeacon(void);
	void beaconIn(Packet *);

	void handlePktForward(Packet *p);
	
};

#endif	/* _DBR_H_ */
