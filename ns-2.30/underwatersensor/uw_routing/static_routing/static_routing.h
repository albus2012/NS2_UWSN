#ifndef STATIC_ROUTING
#define STATIC_ROUTING

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <float.h>
#include <stdlib.h>

#include <tcl.h>

#include "agent.h"
#include "tclcl.h"
#include "config.h"
#include "packet.h"
#include "trace.h"
#include "random.h"
#include "classifier.h"
#include "underwatersensor/uw_common/underwatersensornode.h"
#include "arp.h"
#include "mac.h"
#include "ll.h"


#include <map>
using namespace std;

/*header length of Static routing*/
#define SR_HDR_LEN (3*sizeof(nsaddr_t)+sizeof(int))

class StaticRouting: public Agent {
public:
	StaticRouting();
	int command(int argc, const char*const* argv);
	void recv(Packet*, Handler*);
protected:
	nsaddr_t my_addr;

	UnderwaterSensorNode *node;
	Trace *tracetarget;       // Trace Target
	NsObject *ll;  
	NsObject *port_dmux;
	
	bool has_set_routefile;
	bool has_set_node;
	char route_file[50];

	void readRouteTable(char* filename);


private:
	map<nsaddr_t, nsaddr_t> rtable_;
	
};




#endif