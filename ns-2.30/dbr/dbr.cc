extern "C" {
#include <stdlib.h>
#include <stdio.h>
}

#include <object.h>
#include <agent.h>
#include <trace.h>
#include <packet.h>
#include <scheduler.h>
#include <random.h>

#include <mac.h>
#include <ll.h>
#include <cmu-trace.h>

#include "dbr.h"

#define	BEACON_RESCHED				\
	beacon_timer_->resched(bint_ + 		\
			Random::uniform(2*bdesync_*bint_) - \
			bdesync_*bint_);

#define DEBUG_AGENT
//#define DEBUG_BEACON
#define DEBUG_PRINT
//#define DEBUG_BROADCASTING

// parameters to control the delay
#define	DBR_MAX_DELAY	0.2	// maximal propagation delay for one hop
#define DBR_MAX_RANGE	100	// maximal transmmition range
#define DBR_MIN_BACKOFF	0.0	// minimal backoff time for the packet

//#define USE_FLOODING_ALG	// test for pure flooding protocol
#define DBR_USE_ROUTEFLAG
#define DBR_MAX_HOPS	3
#define DBR_DEPTH_THRESHOLD 0.0
#define DBR_SCALE	1.0

int hdr_dbr::offset_;

/** 
 * The communication class with TclObject
 */

static class DBRAgentClass : public TclClass {
public:
	DBRAgentClass() : TclClass("Agent/DBR") {}
	TclObject* create(int, const char* const*) {
		return (new DBR_Agent);
	}
} class_DBRAgent;

class DBRHeaderClass : public PacketHeaderClass {
public:
	DBRHeaderClass() : PacketHeaderClass("PacketHeader/DBR",
					sizeof(hdr_dbr)) {
		bind_offset(&hdr_dbr::offset_);
	}
} class_dbrhdr;

void DBR_BeaconHandler::handle(Event *e)
{
	a_->forwardPacket((Packet*)e, 0);
}

void DBR_BeaconTimer::expire(Event *e)
{
	a_->beacon_callback();
}

void DBR_SendingTimer::expire(Event *e)
{
	a_->send_callback();
}

/*
void DBR_DeadNeighbTimer::expire(Event *e)
{
	a->deadneighb_callback(ne);
}
*/

// Insert the item into queue.
// The queue is sorted by the expected sending time
// of the packet.
void MyPacketQueue::insert(QueueItem *q)
{
	QueueItem *tmp;
	deque<QueueItem*>::iterator iter;
	
	// find the insert point
	iter = dq_.begin();
	while (iter != dq_.end())
	{
		tmp = *iter;
		if (tmp->send_time_ > q->send_time_)
		{
			dq_.insert(iter, q);
			return;
		}
		iter++;
	}

	// insert at the end of the queue
	dq_.push_back(q);
}

// Check if packet p in queue needs to be updated.
// If packet is not found, or previous sending time 
// is larger than current one, return TRUE.
// Otherwise return FALSE.
bool
MyPacketQueue::update(Packet *p, double t)
{
	Packet *pkt;
	int curID;
	deque<QueueItem*>::iterator iter;
	hdr_dbr *dbrh;

	// get current packet ID
	dbrh = hdr_dbr::access(p);
	curID = dbrh->packetID();

	// search the queue
	iter = dq_.begin();
	while (iter != dq_.end())
	{
		dbrh = hdr_dbr::access((*iter)->p_);
		if (dbrh->packetID() == curID) 
		{ // entry found
			if ((*iter)->send_time_ > t)
			{
				dq_.erase(iter);
				return TRUE;
			}
			else
				return FALSE;
		}
	}

	// not found
	return TRUE;
}

// Find the item in queue which has the same packet ID
// as p, and remove it.
// If such a item is found, return true, otherwise
// return false.
bool
MyPacketQueue::purge(Packet *p)
{
	Packet *pkt;
	int curID;
	deque<QueueItem*>::iterator iter;
	hdr_dbr *dbrh; 

	// get current packet ID
	dbrh = hdr_dbr::access(p);
	curID = dbrh->packetID();

	// search the queue
	iter = dq_.begin();
	while (iter != dq_.end())
	{
		dbrh = hdr_dbr::access((*iter)->p_);
		if (dbrh->packetID() == curID)
		{
			dq_.erase(iter);
			return TRUE;
		}
		iter++;
	}

	return FALSE;
}

// Dump all the items in queue for debug
void MyPacketQueue::dump()
{
	deque<QueueItem*>::iterator iter;
	hdr_dbr *dbrh; 
	int i = 0;

	iter = dq_.begin();
	while (iter != dq_.end())
	{
		dbrh = hdr_dbr::access((*iter)->p_);
		fprintf(stderr, "Queue[%d]: packetID %d, send time %f\n",
				i, dbrh->packetID(), (*iter)->send_time_);
		iter++;
		i++;
	}
}

NeighbTable::NeighbTable(DBR_Agent *a)
{
    int i;

    num_ents = 0;
    max_ents = 100;

    // create the default table with size 100
    tab = new NeighbEnt* [100];
    a_ = a;

    for (i = 0; i < 100; i++)
        tab[i] = new NeighbEnt(a);
}

NeighbTable::~NeighbTable()
{
    int i;

    for (i = 0; i < max_ents; i++)
        delete tab[i];

    delete[] tab;
}

static int neighbEntCmp(const void *a, const void *b)
{
	nsaddr_t ia = ((const NeighbEnt*)a)->net_id;
	nsaddr_t ib = ((const NeighbEnt*)b)->net_id;

	if (ia > ib) return 1;
	if (ia < ib) return -1;
	return 0;
}

void NeighbTable::dump(void)
{
	int i;
	
	for (i = 0; i < num_ents; i++)
		fprintf(stderr, "tab[%d]: %d x = %f, y = %f, z = %f\n",
				i, tab[i]->net_id, 
				tab[i]->x, tab[i]->y, tab[i]->z);
}

void
NeighbTable::ent_delete(const NeighbEnt *ne)
{
	int l, r, m;
	int i;
	NeighbEnt *owslot;

	// binary search
	l = 0; r = num_ents - 1;
	while (l <= r)
	{
		m = l + (r - l)/2;
		if (tab[m]->net_id < ne->net_id)
			l = m + 1;
		else if (tab[m]->net_id > ne->net_id)
			r = m - 1;
		else
			// m is the entry to be deleted
			break;
	}

	if (l > r)
		// no found!
		return;

	owslot = tab[m];

	// slide the entries
	i = m + 1;
	while (i < num_ents)
		tab[i - 1] = tab[i++];

	tab[num_ents-1] = owslot;
	num_ents--;
}

#if 0
/** 
 * Add a neighbor entry ne into the table.
 * The table is sorted by node address.
 */

NeighbEnt* 
NeighbTable::ent_add(const NeighbEnt *ne)
{
	int l, r, m;
	int i;
	NeighbEnt *owslot;

	if (num_ents >= max_ents)
	{
		fprintf(stderr, "Neighbor table is full!\n");
		return 0;
	}

	// binary search
	l = 0; r = num_ents - 1;
	while (l <= r)
	{
		m = l + (r - l)/2;
		if (tab[m]->net_id < ne->net_id)
			l = m + 1;
		else if (tab[m]->net_id > ne->net_id)
			r = m - 1;
		else
		{
			// the entry is existing
			// update the info
			tab[m]->x = ne->x;
			tab[m]->y = ne->y;
			tab[m]->z = ne->z;

			return tab[m];
		}	
	}

	// the entry should go to l
	owslot = tab[num_ents];

	// slide the entries after l
	i = num_ents - 1;
	while (i >= l)
		tab[i+1] = tab[i--];

	tab[l] = owslot;
	tab[l]->net_id = ne->net_id;
	tab[l]->x = ne->x;
	tab[l]->y = ne->y;
	tab[l]->z = ne->z;
	num_ents++;

	return tab[l];
}
#else
NeighbEnt*
NeighbTable::ent_add(const NeighbEnt *ne)
{
	NeighbEnt **pte;
	NeighbEnt *pe;
	int i, j;
	int l, r, m;
	
	// find if the neighbor is already existing
	for (i = 0; i < num_ents; i++)
		if (tab[i]->net_id == ne->net_id)
		{
			tab[i]->x = ne->x;
			tab[i]->y = ne->y;
			tab[i]->z = ne->z;
			
			return tab[i];
		}

	/*
	if (pte = (NeighbEnt**)bsearch(ne, tab, num_ents, 
				sizeof(NeighbEnt *), neighbEntCmp))
	{
		(*pte)->net_id = ne->net_id;	// it doesn't hurt to rewrite it!
		(*pte)->x = ne->x;
		(*pte)->y = ne->y;
		(*pte)->z = ne->z;

		return (*pte);
	}
	*/

	// need we increase the size of table
	if (num_ents == max_ents)
	{
		NeighbEnt **tmp = tab;
		max_ents *= 2;			// double the space
		tab = new NeighbEnt* [max_ents];
		bcopy(tmp, tab, num_ents*sizeof(NeighbEnt *));

		for (i = num_ents; i < max_ents; i++)
			tab[i] = new NeighbEnt(a_);

		delete[] tmp;
	}

	// get the insert point
	if (num_ents == 0)
		i = 0;
	else 
	{
		l = 0;
		r = num_ents - 1;

		while (r > l)
		{
			m = l + (r - l) / 2;
			if (ne->net_id < tab[m]->net_id)
				r = m - 1; 
			else
				l = m + 1;
		}
			
		if (r < l)
			i = r + 1;
		else
			if (ne->net_id < tab[r]->net_id)
				i = r;
			else
				i = r + 1;
	}
	
	// assign an unused slot to i
	if (i <= (num_ents - 1))
		pe = tab[num_ents];

	// adjust the entries after insert point i
	j = num_ents - 1;
	while (j >= i)
	{
		tab[j+1] = tab[j];
		j--;
	}

	if (i <= (num_ents - 1))
		tab[i] = pe;
	tab[i]->net_id = ne->net_id;
	tab[i]->x = ne->x;
	tab[i]->y = ne->y;
	tab[i]->z = ne->z;
	num_ents++;

	return tab[i];
}
#endif	// 0

/*
 * update the neighbor entry's routeFlag field with va
 */

void NeighbTable::updateRouteFlag(nsaddr_t addr, int val)
{
	int i;

	for (i = 0; i < num_ents; i++)
	{
		if (tab[i]->net_id == addr)
		{
			tab[i]->routeFlag = val;
			return;
		}
	}
}

#ifdef	DBR_USE_ROUTEFLAG
NeighbEnt *
NeighbTable::ent_find_shadowest(MobileNode *mn)
{
	NeighbEnt *ne = 0;
	int i;
	double x, y, z, t;

	mn->getLoc(&x, &y, &z);
	t = z;

	#ifdef DEBUG_AGENT
	fprintf(stderr, "[%d]: %d neighbors.\n", mn->address(), num_ents);
	#endif

	for (i = 0; i < num_ents; i++)
	{
		#ifdef	DEBUG_PRINT
		fprintf(stderr, "%d[%d] x: %f, y: %f, z: %f\n", 
			mn->address(), tab[i]->net_id, tab[i]->x, tab[i]->y, tab[i]->z);
		#endif


		if (tab[i]->routeFlag == 1)
		{
			ne = tab[i];
			return ne;
		}

		if (tab[i]->z > t)
		{
		    t = tab[i]->z;
		    ne = tab[i];
		}
	}
	return ne;
}
#else
NeighbEnt *
NeighbTable::ent_find_shadowest(MobileNode *mn)
{
    NeighbEnt *ne = 0;
    int i;
    double x, y, z, t;
 
    mn->getLoc(&x, &y, &z);
    t = z;

    for (i = 0; i < num_ents; i++)
    {
	#ifdef	DEBUG_PRINT
	fprintf(stderr, "%d[%d] x: %f, y: %f, z: %f\n", 
		mn->address(), tab[i]->net_id, tab[i]->x, tab[i]->y, tab[i]->z);
	#endif

        if (tab[i]->z > t)
        {
            t = tab[i]->z;
            ne = tab[i];
        }
    }

    return ne;
}
#endif	// DBR_USE_ROUTEFLAG


DBR_Agent::DBR_Agent() : Agent(PT_DBR),
	bint_(DBR_BEACON_INT), bdesync_(DBR_BEACON_DESYNC), mn_(0), pkt_cnt_(0)
{
	ntab_ = new NeighbTable(this);

	// create packet cache
	pc_ = new PktCache();
}

DBR_Agent::~DBR_Agent()  
{
	delete ntab_;

	// packet cache
	delete pc_;
}

// construct the beacon packet
Packet* 
DBR_Agent::makeBeacon(void)
{
	Packet *p = allocpkt();
	assert(p != NULL);
	
	hdr_ip *iph = hdr_ip::access(p);
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_dbr *dbrh = hdr_dbr::access(p);

	// setup header
	cmh->next_hop_ = IP_BROADCAST;
	cmh->addr_type_ = AF_INET;
	cmh->ptype_ = PT_DBR;
	cmh->direction() = hdr_cmn::DOWN;
	cmh->size() = dbrh->size() + IP_HDR_LEN;

	//iph->daddr() = IP_BROADCAST << Address::instance().nodeshift();
	iph->saddr() = mn_->address();
	iph->daddr() = IP_BROADCAST;
	iph->dport() = DBR_PORT;

	// fill in the location info 
	mn_->getLoc(&(dbrh->x), &(dbrh->y), &(dbrh->z));
	dbrh->mode() = DBRH_BEACON;
	dbrh->nhops() = 1;

	return p;
}

// send beacon pkt only once at the beginning
void DBR_Agent::sendBeacon(void)
{
	Packet *p = makeBeacon();
	hdr_cmn *cmh = hdr_cmn::access(p);

	if (p) 
	{
		assert(!HDR_CMN(p)->xmit_failure_);
		if (cmh->direction() == hdr_cmn::UP)
			cmh->direction() = hdr_cmn::DOWN;
		Scheduler::instance().schedule(ll, p, 0);
	}
	else
	{
		fprintf(stderr, "ERROR: Can't make new beacon!\n");
		abort();
	}
}

// fetch the packet from the sending queue
// and broadcast.
void DBR_Agent::send_callback(void)
{
	QueueItem *q;
	hdr_dbr *dbrh;

	// we're done if there is no packet in queue
	if (pq_.empty())
		return;

	// send the first packet out
	q = pq_.front();
	pq_.pop();
	Scheduler::instance().schedule(ll, q->p_, 0);

	// put the packet into cache
	dbrh = hdr_dbr::access(q->p_);
	pc_->addPacket(dbrh->packetID());

	// reschedule the timer if there are
	// other packets in the queue
	if (!pq_.empty())
	{
		q = pq_.front();
		latest_ = q->send_time_;
		send_timer_->resched(latest_ - NOW);
	}
}

void DBR_Agent::beacon_callback(void)
{
	return;

	Packet *p = makeBeacon();

	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);

	if (p) 
	{
		assert(!HDR_CMN(p)->xmit_failure_);
		if (cmh->direction() == hdr_cmn::UP)
			cmh->direction() = hdr_cmn::DOWN;

		//fprintf(stderr, "%d is broadcasting beacon pkt src: %d, dst: %d\n",
		//		mn_->address(), iph->saddr(), iph->daddr());

		Scheduler::instance().schedule(ll, p, Random::uniform()*JITTER);
	}
	else
	{
		fprintf(stderr, "ERROR: Can't make new beacon!\n");
		exit(-1);
	}

	//BEACON_RESCHED
}

void DBR_Agent::deadneighb_callback(NeighbEnt *ne)
{
	ntab_->ent_delete(ne);
}

void DBR_Agent::forwardPacket(Packet *p, int flag)
{
	hdr_ip *iph = hdr_ip::access(p);
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_dbr *dbrh = hdr_dbr::access(p);
	NeighbEnt *ne;

	double delay = 0.0;

	#ifdef	DEBUG_AGENT
	fprintf(stderr, "node %d is forwarding to %d\n", 
				mn_->address(), Address::instance().get_nodeaddr(iph->daddr()));
	#endif

	// common settings for forwarding
	cmh->direction() = hdr_cmn::DOWN;
	cmh->addr_type_ = AF_INET;
	cmh->ptype_ = PT_DBR;
	cmh->size() = dbrh->size() + IP_HDR_LEN;

	// make decision on next hop based on packet mode
	switch (dbrh->mode()) 
	{
	case DBRH_DATA_GREEDY: 
		ne = ntab_->ent_find_shadowest(mn_);
		if (ne)
		{
			cmh->next_hop() = ne->net_id;
			#ifdef	DEBUG_AGENT
			fprintf(stderr, "[%d] -> %d\n", mn_->address(), ne->net_id);
			#endif
		}
		else
		{
			#ifdef	DEBUG_PRINT
			fprintf(stderr, "[%d]: put pkt into recovery mode!\n", mn_->address());
			#endif
		
			// put packet into recovery mode
			cmh->next_hop() = IP_BROADCAST;
			dbrh->mode() = DBRH_DATA_RECOVER;
			dbrh->prev_hop() = mn_->address();
			dbrh->owner() = mn_->address();
			dbrh->nhops() = DBR_MAX_HOPS;		// set the range of broadcasting by hops
		}
		break;
	case DBRH_DATA_RECOVER:
		#ifdef	DEBUG_PRINT
		fprintf(stderr, "[%d]: forward in recovery alogrithm!\n", mn_->address());
		#endif

		if (pc_->accessPacket(dbrh->packetID()))
		{
			// each node only broadcasts once 
			#ifdef DEBUG_PRINT
			fprintf(stderr, "[%d]: packet is already in cache!\n", mn_->address());
			#endif

			return;
		}
		else
			pc_->addPacket(dbrh->packetID());

		// can we find other greedy node?
		ne = ntab_->ent_find_shadowest(mn_);
		if (ne == NULL)
		{
			if (dbrh->nhops() <= 0)
			{
				#ifdef	DEBUG_BROADCASTING
				fprintf(stderr, "[%d] drops pkt! (nhops < 0)\n", mn_->address());
				#endif
					
				drop(p, DROP_RTR_TTL);
				return;
			}
			else
			{
				// broadcasting
				cmh->next_hop() = IP_BROADCAST;
				dbrh->mode() = DBRH_DATA_RECOVER;
				dbrh->owner() = dbrh->prev_hop();
				dbrh->prev_hop() = mn_->address();
				dbrh->nhops()--;
			
				// set broadcasting delay
				delay = Random::uniform() * JITTER;
			}
		}
		else if (ne->net_id != dbrh->prev_hop())
		{
			// new route is found, put pkt back to greedy alg
			cmh->next_hop() = ne->net_id;
			dbrh->mode() = DBRH_DATA_GREEDY;
			#ifdef	DEBUG_AGENT
			fprintf(stderr, "back to greedy: [%d] -> %d\n", mn_->address(), ne->net_id);
			#endif
		}
		else
		{
			#ifdef	DEBUG_PRINT
			fprintf(stderr, "[%d]:dbrh->nhops = %d\n", mn_->address(),
					dbrh->nhops());
			#endif

			if (dbrh->nhops() <= 0)
			{
				#ifdef	DEBUG_BROADCASTING
				fprintf(stderr, "[%d] drops pkt! (nhops < 0)\n", mn_->address());
				#endif

				drop(p, DROP_RTR_TTL);
				return;
			}
			else
			{
				#ifdef	DEBUG_BROADCASTING
				fprintf(stderr, "[%d] is broadcasting!\n", mn_->address());
				#endif

				// broadcasting
				cmh->next_hop() = IP_BROADCAST;
				dbrh->mode() = DBRH_DATA_RECOVER;
				dbrh->owner() = dbrh->prev_hop();
				dbrh->prev_hop() = mn_->address();
				dbrh->nhops()--;

				// set broadcasting delay
				delay = Random::uniform() * JITTER;
			}
		}
		break;
	default:
		fprintf(stderr, "Wrong! The packet can't be forwarded!\n");
		abort();
		break;
	}

	// schedule the sending
	assert(!HDR_CMN(p)->xmit_failure_);
	Scheduler::instance().schedule(ll, p, delay);
}

// get the info from beacon pkt
void DBR_Agent::beaconIn(Packet *p)
{
	hdr_ip *iph = hdr_ip::access(p);
	hdr_dbr *dbrh = hdr_dbr::access(p);

	nsaddr_t src = Address::instance().get_nodeaddr(iph->saddr());

	// create NeighbEnt
	NeighbEnt *ne;
	ne = new NeighbEnt(this);

	#ifdef DEBUG_BEACON
	fprintf(stderr, "%d got beacon from %d\n",
				mn_->address(), src);
	#endif

	ne->x = dbrh->x;
	ne->y = dbrh->y;
	ne->z = dbrh->z;
	ne->net_id = src;
	
	ntab_->ent_add(ne);

	delete ne;

	// we consumed the packet, free it!
	Packet::free(p);
}

#if 1
void DBR_Agent::recv(Packet *p, Handler *)
{
	hdr_ip *iph = hdr_ip::access(p);
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_dbr *dbrh = hdr_dbr::access(p);

	double x, y, z;

	nsaddr_t src = Address::instance().get_nodeaddr(iph->saddr());
	nsaddr_t dst = Address::instance().get_nodeaddr(iph->daddr());

	if (mn_ == NULL)
	{
		fprintf(stderr, "ERROR: Pointer to node is null!\n");
		abort();
	}

	mn_->getLoc(&x, &y, &z);

	if (dbrh->mode() == DBRH_BEACON)
	{// although we eliminate the beacon pkt
	 // but we still reserve the handler for 
	 // furture control logic

		#ifdef DEBUG_PRINT
		fprintf(stderr, "[%d]: beacon pkt is received.\n", mn_->address());
		#endif

		// self is not one of the neighbors
		if (src != mn_->address())
			beaconIn(p); 
		return;
	}
	
	if ((src == mn_->address()) &&
	    (cmh->num_forwards() == 0))
	{// packet I'm originating

		cmh->direction() = hdr_cmn::DOWN;
		cmh->addr_type_ = AF_INET;
		cmh->ptype_ = PT_DBR;
		cmh->size() = dbrh->size() + IP_HDR_LEN;
		cmh->next_hop() = IP_BROADCAST;
		iph->ttl_ = 128;

		// setup DBR header
		dbrh->mode() = DBRH_DATA_GREEDY;
		//dbrh->packetID() = (int)mn_->address();
		dbrh->packetID() = pkt_cnt_++;
		dbrh->depth() = z;		// save the depth info

		// broadcasting the pkt
		assert(!HDR_CMN(p)->xmit_failure_);
		Scheduler::instance().schedule(ll, p, 0);

		return;
	}

	if ((src == mn_->address()) &&
	    (dbrh->mode() == DBRH_DATA_GREEDY))
	{// Wow, it seems some one is broadcasting the pkt for us,
	 // so we need to dismiss the timer for the pkt

		#ifdef	DEBUG_PRINT
		fprintf(stderr, "[%d] got the pkt I've sent\n", mn_->address());
		#endif

		drop(p, DROP_RTR_ROUTE_LOOP);
		return;
	}

	if (dst == mn_->address())
	{// packet is for me

		#ifdef DEBUG_PRINT
		fprintf(stderr, "[%d] packet is delivered!\n", mn_->address());
		fflush(stderr);
		#endif

		// we may need to send it to upper layer agent
		send_to_dmux(p, 0);

		return;
	}

	// packet I'm forwarding
	handlePktForward(p);
}

#ifdef USE_FLOODING_ALG
void DBR_Agent::handlePktForward(Packet *p)
{
	hdr_ip *iph = hdr_ip::access(p);
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_dbr *dbrh = hdr_dbr::access(p);

	if (--iph->ttl_ == 0)
	{
		drop(p, DROP_RTR_TTL);
		return;
	}

	// Is this pkt recieved before?
	// each node only broadcasts same pkt once 
	if (pc_->accessPacket(dbrh->packetID()))
	{
		drop(p, DROP_RTR_TTL);
		return;
	}
	else
		pc_->addPacket(dbrh->packetID());

	// common settings for forwarding
	cmh->num_forwards()++;
	cmh->direction() = hdr_cmn::DOWN;
	cmh->addr_type_ = AF_INET;
	cmh->ptype_ = PT_DBR;
	cmh->size() = dbrh->size() + IP_HDR_LEN;
	cmh->next_hop() = IP_BROADCAST;
	
	// finally broadcasting it!
	assert(!HDR_CMN(p)->xmit_failure_);
	Scheduler::instance().schedule(ll, p, Random::uniform()*JITTER);
}
#else
// Forward the packet according to its mode
// There are two modes right now: GREEDY and RECOVERY
// The node will broadcast all RECOVERY pakets, but
// it will drop the GREEDY pakets from upper level.
void DBR_Agent::handlePktForward(Packet *p)
{
	hdr_ip *iph = hdr_ip::access(p);
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_dbr *dbrh = hdr_dbr::access(p);

	double delta;
	double delay = .0;

	double x, y, z;

	if (--iph->ttl_ == 0)
	{
		drop(p, DROP_RTR_TTL);
		return;
	}

	/*
	// dump the queue
	fprintf(stderr, "-------- Node id: %d --------\n", mn_->address());
	fprintf(stderr, " curID: %d\n", dbrh->packetID());
	pq_.dump();
	*/

#if 0
	// search sending queue for p
	if (pq_.purge(p))
	{
		drop(p, DROP_RTR_TTL);
		return;
	}
#endif
	
	// common settings for forwarding
	cmh->num_forwards()++;
	cmh->direction() = hdr_cmn::DOWN;
	cmh->addr_type_ = AF_INET;
	cmh->ptype_ = PT_DBR;
	cmh->size() = dbrh->size() + IP_HDR_LEN;
	cmh->next_hop() = IP_BROADCAST;

	switch (dbrh->mode())
	{
	case DBRH_DATA_GREEDY:
		mn_->getLoc(&x, &y, &z);
	
		// compare the depth
		delta = z - dbrh->depth();

		// only forward the packet from lower level
		if (delta < DBR_DEPTH_THRESHOLD)
		{
			pq_.purge(p);
			drop(p, DROP_RTR_TTL);
			return;
		}

		#ifdef DEBUG_NONE
		fprintf(stderr, "[%d]: z = %.3f, depth = %.3f, delta = %.3f\n", 
			mn_->address(), z, dbrh->depth(), delta);
		#endif

		// update current depth
		dbrh->depth() = z;

		// compute the delay
		//delay = DBR_DEPTH_THRESHOLD / delta * DBR_SCALE;
		delta = 1.0 - delta / DBR_MAX_RANGE;
		delay = DBR_MIN_BACKOFF + 4.0 * delta * DBR_MAX_DELAY;

		// set time out for the packet
		
		break;
	case DBRH_DATA_RECOVER:
		if (dbrh->nhops() <= 0)
		{
			#ifdef	DEBUG_PRINT
			fprintf(stderr, "[%d] drops pkt! (nhops < 0)\n", mn_->address());
			#endif
					
			drop(p, DROP_RTR_TTL);
			return;
		}
		dbrh->nhops()--;
		break;
	default:
		fprintf(stderr, "Unknown data type!]n");
		return;
	}

	// make up the DBR header
	dbrh->owner() = dbrh->prev_hop();
	dbrh->prev_hop() = mn_->address();

	#ifdef DEBUG_NONE
	fprintf(stderr, "[%d]: delay %f before broacasting!\n", 
			mn_->address(), delay);
	#endif

	if (pc_ == NULL) {
		fprintf(stderr, "packet cache pointer is null!\n");
		exit(-1);
	}

	// Is this pkt recieved before?
	// each node only broadcasts the same pkt once 
	if (pc_->accessPacket(dbrh->packetID()))
	{
		drop(p, DROP_RTR_TTL);
		return;
	}
	//else
	//	pc_->addPacket(dbrh->packetID());

	// put the packet into sending queue
	double expected_send_time = NOW + delay;
	QueueItem *q = new QueueItem(p, expected_send_time);

	/*
	pq_.insert(q);
	QueueItem *qf;
	qf = pq_.front();
	send_timer_->resched(qf->send_time_ - NOW);
	*/

	if (pq_.empty())
	{ 
		pq_.insert(q);
		latest_ = expected_send_time;
		send_timer_->resched(delay);
	}
	else
	{
		if (pq_.update(p, expected_send_time))
		{
			pq_.insert(q);

			// update the sending timer
			if (expected_send_time < latest_)
			{
				latest_ = expected_send_time;
				send_timer_->resched(delay);
			}
		}
	}
}
#endif	// end of USE_FLOODING_ALG
#else
void DBR_Agent::recv(Packet *p, Handler *)
{
	hdr_ip *iph = hdr_ip::access(p);
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_dbr *dbrh = hdr_dbr::access(p);
	
	nsaddr_t src = Address::instance().get_nodeaddr(iph->saddr());
	nsaddr_t dst = Address::instance().get_nodeaddr(iph->daddr());

	#ifdef	DEBUG_PRINT
	//fprintf(stderr, "%d receives pkt from %d to %d\n", 
	//	mn_->address(), src, dst);
	#endif

	if (mn_ == NULL)
	{
		fprintf(stderr, "ERROR: Pointer to node is null!\n");
		abort();
	}

	if (dbrh->mode() == DBRH_BEACON)
	{// beacon packet

		// self is not one of the neighbors
		if (src != mn_->address())
			beaconIn(p); 
		return;
	}
	else if ((src == mn_->address()) &&
		(cmh->num_forwards() == 0))
	{// packet I'm originating

		#ifdef DEBUG_PRINT
		fprintf(stderr, "%d generates data packet.\n", src);
		#endif
		
		cmh->size() += IP_HDR_LEN + 8;
		cmh->direction() = hdr_cmn::DOWN;
		iph->ttl_ = 128;
		dbrh->mode() = DBRH_DATA_GREEDY;
		dbrh->packetID() = (int)mn_->address();
	}
	else if ((src == mn_->address()) &&
		(dbrh->mode() == DBRH_DATA_GREEDY))
	{// duplicate packet, discard

		#ifdef	DEBUG_PRINT
		fprintf(stderr, "got the pkt I've sent\n");
		#endif

		drop(p, DROP_RTR_ROUTE_LOOP);
		return;
	}
	else if (dst == mn_->address())
	{// packet is for me

		#ifdef DEBUG_BROADCASTING
		fprintf(stderr, "Packet is delivered!\n");
		fflush(stderr);
		#endif

		// we may need to send it to upper layer agent
		send_to_dmux(p, 0);

		return;
	}
/*------------------------------------------------
	else if (dst == IP_BROADCAST) 
	{
		if (dbrh->mode() == DBRH_BEACON)
		{
			// self is not one of the neighbors
			if (src != mn_->address())
				beaconIn(p); 
			return;
		}
	}
------------------------------------------------*/
	else
	{// packet I'm forwarding

		if (--iph->ttl_ == 0)
		{
			drop(p, DROP_RTR_TTL);
			return;
		}

		if((dbrh->mode() == DBRH_DATA_RECOVER) &&
			(dbrh->owner() == mn_->address()))
		{
			//fprintf(stderr, "got the pkt I've sent\n");
			drop(p, DROP_RTR_ROUTE_LOOP);
			// it seems this neighbor couldn't find a greedy node
			//ntab_->updateRouteFlag(dbrh->prev_hop(), 0);
			return;
		}
	}

	#ifdef	DEBUG_PRINT
	fprintf(stderr, "owner %d, prev-hop: %d, cur: %d\n",
		dbrh->owner(), dbrh->prev_hop(), mn_->address());
	#endif

	// it's time to forward the pkt now	
	forwardPacket(p);
}
#endif

void DBR_Agent::recv2(Packet *p, Handler *)
{
	hdr_ip *iph = hdr_ip::access(p);
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_dbr *dbrh = hdr_dbr::access(p);
	
	nsaddr_t src = Address::instance().get_nodeaddr(iph->saddr());
	nsaddr_t dst = Address::instance().get_nodeaddr(iph->daddr());

	#ifdef	DEBUG_PRINT
	//fprintf(stderr, "%d receives pkt from %d to %d\n", 
	//	mn_->address(), src, dst);
	#endif

	if (mn_ == NULL)
	{
		fprintf(stderr, "ERROR: Pointer to node is null!\n");
		abort();
	}

	if (dbrh->mode() == DBRH_BEACON)
	{// beacon packet

		// self is not one of the neighbors
		if (src != mn_->address())
			beaconIn(p); 
		return;
	}
	else if ((src == mn_->address()) &&
		(cmh->num_forwards() == 0))
	{// packet I'm originating

		#ifdef DEBUG_PRINT
		fprintf(stderr, "%d generates data packet.\n", src);
		#endif
		
		cmh->size() += IP_HDR_LEN + 8;
		cmh->direction() = hdr_cmn::DOWN;
		iph->ttl_ = 128;
		dbrh->mode() = DBRH_DATA_GREEDY;
		dbrh->packetID() = (int)mn_->address();
	}
	else if ((src == mn_->address()) &&
		(dbrh->mode() == DBRH_DATA_GREEDY))
	{// duplicate packet, discard

		#ifdef	DEBUG_PRINT
		fprintf(stderr, "got the pkt I've sent\n");
		#endif

		drop(p, DROP_RTR_ROUTE_LOOP);
		return;
	}
	else if (dst == mn_->address())
	{// packet is for me

		#ifdef DEBUG_BROADCASTING
		fprintf(stderr, "Packet is delivered!\n");
		fflush(stderr);
		#endif

		// we may need to send it to upper layer agent
		send_to_dmux(p, 0);

		return;
	}
/*------------------------------------------------
	else if (dst == IP_BROADCAST) 
	{
		if (dbrh->mode() == DBRH_BEACON)
		{
			// self is not one of the neighbors
			if (src != mn_->address())
				beaconIn(p); 
			return;
		}
	}
------------------------------------------------*/
	else
	{// packet I'm forwarding

		if (--iph->ttl_ == 0)
		{
			drop(p, DROP_RTR_TTL);
			return;
		}

		if((dbrh->mode() == DBRH_DATA_RECOVER) &&
			(dbrh->owner() == mn_->address()))
		{
			//fprintf(stderr, "got the pkt I've sent\n");
			drop(p, DROP_RTR_ROUTE_LOOP);
			// it seems this neighbor couldn't find a greedy node
			//ntab_->updateRouteFlag(dbrh->prev_hop(), 0);
			return;
		}
	}

	#ifdef	DEBUG_PRINT
	fprintf(stderr, "owner %d, prev-hop: %d, cur: %d\n",
		dbrh->owner(), dbrh->prev_hop(), mn_->address());
	#endif

	// it's time to forward the pkt now	
	forwardPacket(p);
}

int DBR_Agent::command(int argc, const char * const *argv)
{
	if (argc == 2)
	{
		if (strcmp(argv[1], "start-dbr") == 0)
		{
			init();
			return TCL_OK;
		}
		
		if (strcmp(argv[1], "test") == 0)
		{
			sendBeacon();	// only send beacon once
			return TCL_OK;
		}
	}
	else if (argc == 3)
	{
		TclObject *obj;
			
		if (strcmp(argv[1], "port-dmux") == 0) {
			dmux = (PortClassifier *)TclObject::lookup(argv[2]);
			if (dmux == 0) {
				fprintf(stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1],
					argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		}

		if (strcasecmp(argv[1], "tracetarget") == 0)
		{
			if ((obj = TclObject::lookup(argv[2])) == 0)
			{
				fprintf(stderr, "%s: %s lookup of %s failed\n", 
					__FILE__, argv[1], argv[2]);
				return TCL_ERROR;
			}
			traceagent = (Trace *)obj;
			return TCL_OK;
		}

		if (strcmp(argv[1], "node") == 0)
		{
			if ((obj = TclObject::lookup(argv[2])) == 0)
			{
				fprintf(stderr, "%s: %s lookup of %s failed\n", 
					__FILE__, argv[1], argv[2]);
				return TCL_ERROR;
			}
			mn_ = (MobileNode *)obj;
			return TCL_OK;
		}

		if (strcasecmp(argv[1], "add-ll") == 0)
		{
			if ((obj = TclObject::lookup(argv[2])) == 0)
			{
				fprintf(stderr, "DBRAgent: %s lookup of %s failed\n",
						argv[1], argv[2]);
				return TCL_ERROR;
			}
			ll = (NsObject*)obj;
			return TCL_OK;
		}

	}
	return (Agent::command(argc, argv));
}

void DBR_Agent::trace(char* fmt, ...)
{
	va_list ap;

	if (!traceagent) return;

	va_start(ap, fmt);
	vsprintf(traceagent->pt_->buffer(), fmt, ap);
	traceagent->pt_->dump();
	va_end(ap);
}

void DBR_Agent::init(void)
{
	// setup the timer
	beacon_timer_ = new DBR_BeaconTimer(this);
	beacon_timer_->sched(Random::uniform(bint_));

	send_timer_ = new DBR_SendingTimer(this);
	//send_timer_->sched(Random::uniform(bint_));
}

void DBR_Agent::tap(const Packet *p)
{
	// add function later
}

//////////////////////////////////////////////////////////////////
void dumpDBRHdr(Packet *p)
// dump the header info for debug
{
	hdr_dbr *dbrh = hdr_dbr::access(p);

}
