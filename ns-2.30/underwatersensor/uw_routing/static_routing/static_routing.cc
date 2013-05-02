#include "static_routing.h"
#include "cmu-trace.h"

static class StaticRoutingClass : public TclClass {
public:
	StaticRoutingClass() : TclClass("Agent/StaticRouting") {}
	TclObject* create(int argc, const char*const* argv) {
		return(new StaticRouting());
	}
} class_staticrouting;


StaticRouting::StaticRouting():Agent(PT_UW_SROUTE)
{
	node = NULL;
	has_set_routefile = false;
	has_set_node = false;
}


int StaticRouting::command(int argc, const char *const *argv)
{
	Tcl& tcl =  Tcl::instance();

	if ( argc == 2 ) {
		if( strcasecmp(argv[1], "start" ) == 0 ) {
			return TCL_OK;
		}
	} else if (argc == 3) {
		if ( strcasecmp( argv[1], "addr") == 0 ) {
			my_addr = Address::instance().str2addr(argv[2]);
				
			has_set_node = true; 

			if( has_set_routefile ) {
				readRouteTable(route_file);
			}
			return TCL_OK;
		}

		if( strcasecmp(argv[1], "set-routefile")==0 ){
			//load routing table
			has_set_routefile = true;

			strcpy(route_file, argv[2]);

			if( has_set_node ) {
				readRouteTable(route_file);
			}
			return TCL_OK;

		}else if (strcasecmp(argv[1], "on-node")==0) {
			//   printf ("inside on node\n");
			node = (UnderwaterSensorNode *)tcl.lookup(argv[2]);

			return TCL_OK;
		} else if ( strcasecmp(argv[1], "node") == 0 ) {
			node = (UnderwaterSensorNode *)tcl.lookup(argv[2]);
			return TCL_OK;
		} else if (strcasecmp(argv[1], "add-ll") == 0) {

			TclObject *obj;

			if ( (obj = TclObject::lookup(argv[2])) == 0) {
				fprintf(stderr, "lookup of %s failed\n", argv[2]);
				return TCL_ERROR;
			}
			ll = (NsObject *) obj;

			return TCL_OK;
		}else if(strcasecmp (argv[1], "tracetarget") == 0) {
			TclObject *obj;
			if ((obj = TclObject::lookup (argv[2])) == 0) {
				fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1],
					argv[2]);
				return TCL_ERROR;
			}

			tracetarget = (Trace *) obj;
			return TCL_OK;
		}else if(strcasecmp(argv[1], "port-dmux") == 0) {
			// printf("vectorbasedforward:port demux is called \n");
			TclObject *obj;

			if ( (obj = TclObject::lookup(argv[2])) == 0) {
				fprintf(stderr, "lookup of %s failed\n", argv[2]);
				return TCL_ERROR;
			}
			port_dmux = (NsObject *) obj;
			return TCL_OK;
		}

	} 

	return Agent::command(argc, argv);
}


void StaticRouting::readRouteTable(char *filename)
{
	FILE* stream = fopen(filename, "r");
	nsaddr_t current_node, dst_node, nxt_hop; 

	if( stream == NULL ) {
		printf("ERROR: Cannot find routing table file!\nEXIT...\n");
		exit(0);
	}

	while( !feof(stream) ) {
		fscanf(stream, "%d:%d:%d", &current_node, &dst_node, &nxt_hop);

		if( my_addr == current_node ) {
			rtable_[dst_node] = nxt_hop;
		}
	}

	fclose(stream);
}



void StaticRouting::recv(Packet *p, Handler *h)
{
	struct hdr_cmn* ch = HDR_CMN(p);
	struct hdr_ip* ih = HDR_IP(p);

	ch->uw_flag() = true;

	if( ih->saddr() == my_addr ) {
		if( ch->num_forwards() > 0 ) {
			//there is a loop, and then drop it
			drop(p, DROP_RTR_ROUTE_LOOP);
			return;
		}
		else if (ch->num_forwards() == 0 ) {
			ch->size() +=  SR_HDR_LEN;
		}
	} else if( ch->next_hop() != my_addr ) {
		drop(p, DROP_MAC_DUPLICATE);
		return;
	}
	
	//increase the number of forwards
	ch->num_forwards() += 1;

	if( ch->direction() == hdr_cmn::UP && 
		( (u_int32_t)ih->daddr() == IP_BROADCAST || ih->daddr() == my_addr ) ) {
		port_dmux->recv(p, (Handler*)NULL);
		return;
	
	} else {
		ch->direction() = hdr_cmn::DOWN;
		ch->addr_type() = NS_AF_INET;
		if ( (u_int32_t)ih->daddr() == IP_BROADCAST ) {
			ch->next_hop() = IP_BROADCAST;
		}
		else {
			if( rtable_.count(ih->daddr()) != 0 )
				ch->next_hop() = rtable_[ih->daddr()];
			else {
				//fail to find the route, drop it
				drop(p, DROP_RTR_NO_ROUTE);
				return;
			}
		}

		Scheduler::instance().schedule(target_, p, 0.0);
	
	}

}
