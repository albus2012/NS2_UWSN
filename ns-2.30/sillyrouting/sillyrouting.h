#ifndef __SILLYROUTING_H__
#define __SILLYROUTING_H__

#include <agent.h>
#include <packet.h>
#include <trace.h>
#include <random.h>
#include <classifier-port.h>
#include <address.h>

#include "config.h"
#include "scheduler.h"
#include "ip.h"
#include "queue.h"
#include "connector.h"
#include "cmu-trace.h"
#include "delay.h"
#include "arp.h"
#include "ll.h"
#include "mac.h"
#include "mobilenode.h"
#include "underwatersensor/uw_common/underwatersensornode.h"
#include "strings.h"


#ifndef BROAD_JITTER
#define BROAD_JITTER (Random::uniform()*0.25)
#endif

#ifndef ROUTER_PORT
#define ROUTER_PORT 255
#endif


class SillyRouting: public Agent{
private:
	nsaddr_t	myaddr_;
protected:
	PortClassifier* dmux_;	
	Trace*			tracetarget;
	MobileNode*     node_;
	UnderwaterSensorNode* node;
	NsObject*		ll_;
public:
	SillyRouting();
	int command(int, const char* const*);
	void recv(Packet*, Handler*);
};

#endif 


