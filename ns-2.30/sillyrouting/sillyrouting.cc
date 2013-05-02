#include "sillyrouting.h"



static class SillyRoutingClass: public TclClass
{
public:
	SillyRoutingClass():TclClass("Agent/SillyRouting"){}
	TclObject *create(int argc, const char* const* argv)
	{
		return (new SillyRouting());
	}
}class_sillyrouting;



SillyRouting::SillyRouting():Agent(PT_UWVB)
{
}


int SillyRouting::command(int argc, const char* const*argv)
{
		if( argc== 2 )
		{
			if( strcasecmp(argv[1], "start") == 0 )
			{
				return TCL_OK;
			}
	
		}
		else if( argc==3 )
		{
			if (strcasecmp (argv[1], "addr") == 0) {
	 			int temp;
	 			temp = Address::instance().str2addr(argv[2]);
	 			myaddr_ = temp;
	 			return TCL_OK;
      		}

			TclObject *obj;
			if ((obj = TclObject::lookup (argv[2])) == 0)
				return TCL_ERROR;
	
			if (strcasecmp (argv[1], "tracetarget") == 0)
			{
				tracetarget = (Trace *) obj;
				return TCL_OK;
			}
			
			if (strcasecmp(argv[1], "add-ll") == 0) {
				ll_ = (NsObject *) obj;
				return TCL_OK;
			}
			
			if (strcasecmp(argv[1], "on-node")==0) {
				//   printf ("inside on node\n");
				node = (UnderwaterSensorNode *)obj;
				return TCL_OK;
			}
			
			if (strcasecmp (argv[1], "node") == 0)
			{
				node_ = (MobileNode*) obj;
				return TCL_OK;
			}
			
			if (strcasecmp (argv[1], "port-dmux") == 0)
			{
				dmux_ = (PortClassifier*) obj;
				return TCL_OK;
			}
		}
	
		return Agent::command( argc, argv);

}



void SillyRouting::recv(Packet* p, Handler* h)
{
	hdr_cmn* cmh = HDR_CMN(p);
	hdr_ip*  iph = HDR_IP(p);


	if( iph->saddr() == myaddr_ ) {
		if(cmh->num_forwards() > 0 ) {
			//if there exists a loop, must drop the packet
			drop(p, DROP_RTR_ROUTE_LOOP);
			return;
		}
		else{
			cmh->direction() = hdr_cmn::DOWN;
			cmh->addr_type_ = NS_AF_INET;
		}
    }
	cmh->num_forwards()++;

	if( cmh->direction() == hdr_cmn::UP && 
		(iph->daddr() == myaddr_ || iph->daddr()==(nsaddr_t)IP_BROADCAST ) ) {
		dmux_->recv(p, (Handler*)0);
	}
	else {
		cmh->direction() = hdr_cmn::DOWN;
		cmh->addr_type_ = NS_AF_INET;
		//send to ll layer directly to avoid ARP
		ll_->recv(p);
		//Scheduler::instance().schedule(ll_, p, 0.0);
		//Scheduler::instance().schedule(target_, p, 0.0);	
	}

	return;
/*	if( cmh->direction() == hdr_cmn::UP &&
        (iph->daddr()== myaddr_ || iph->daddr()==(nsaddr_t)IP_BROADCAST) )
    {
        dmux_->recv(p, (Handler*)0);
        return;
    }
*/

	//forward the pkt
	/*
	if( iph->daddr() == (nsaddr_t)IP_BROADCAST &&
			iph->saddr() == myaddr_ )
	{
		cmh->next_hop() = IP_BROADCAST;
		cmh->direction() = hdr_cmn::DOWN;
		cmh->addr_type_ = NS_AF_INET;
		Scheduler::instance().schedule( target_, p, BROAD_JITTER);
		return;
	}
	*/
/*
	//forward
	cmh->direction() = hdr_cmn::DOWN;
	cmh->addr_type_ = NS_AF_INET;
	if( (u_int32_t)iph->daddr() == IP_BROADCAST )
		cmh->next_hop() = IP_BROADCAST;
	else
		cmh->next_hop() = myaddr_-1;
	Scheduler::instance().schedule(target_, p, 0.0);
*/
}



