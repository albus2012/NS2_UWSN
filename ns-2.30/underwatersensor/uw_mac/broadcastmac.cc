#include "packet.h"
#include "random.h"
#include "underwatersensor/uw_common/underwatersensornode.h"
#include "mac.h"
#include "broadcastmac.h"
#include "underwaterphy.h"


StatusHandler:: StatusHandler(BroadcastMac*  p):mac_(p){}

void StatusHandler::handle(Event* e)
{ 
	mac_->StatusProcess(e); 
}

CallbackHandler:: CallbackHandler(BroadcastMac*  p):mac_(p){}

void CallbackHandler::handle(Event* e)
{ 
	mac_->CallbackProcess(e); 
}


BackoffHandler::BackoffHandler(BroadcastMac* p):mac_(p){counter=0;}

void BackoffHandler::handle(Event* e)
{ 
	counter++;
	if(counter<MAXIMUMCOUNTER)
		mac_->TxProcess((Packet*)e);
	else 
	{
		clear();
		printf("backoffhandler: too many backoffs\n");
		mac_->CallbackProcess(e);
		mac_->DropPacket((Packet*) e);
	}
}

void BackoffHandler::clear()
{ 
	counter=0;
}



/* ======================================================================
Broadcast MAC for  underwater sensor
====================================================================== */
static class BroadcastMacClass : public TclClass {
public:
	BroadcastMacClass():TclClass("Mac/UnderwaterMac/BroadcastMac") {}
	TclObject* create(int, const char*const*) {
		return (new BroadcastMac());
	}
}class_broadcastmac;


BroadcastMac::BroadcastMac() : UnderwaterMac(),status_handler(this),backoff_handler(this),callback_handler(this)
{
	bind("packetheader_size_",&packetheader_size_); 
	bind("packet_size_", &packet_size_);
}

/*
this program is used to handle the received packet, 
it should be virtual function, different class may have 
different versions.
*/

void 
BroadcastMac::RecvProcess(Packet* pkt){
	char* mh=(char*)pkt->access(hdr_mac::offset_);
	hdr_cmn* cmh=HDR_CMN(pkt);

	assert(initialized());
	int dst=this->hdr_dst(mh);
	
	//get a packet from modem, remove the sync hdr from txtime first
	//cmh->txtime() -= getSyncHdrLen();

	if (cmh->error()) 
	{
//		printf("broadcast:node %d  gets a corrupted packet at  %f\n",index_,NOW);
		if(drop_) drop_->recv(pkt,"Error/Collision");
		else Packet::free(pkt);
		return;
	}

	if(dst==(int)MAC_BROADCAST || dst == index_ ){
		if( packet_size_ ==0 ) {
			cmh->size() -= packetheader_size_;
		}
		uptarget_->recv(pkt, this);
		return;
	}

//	printf("underwaterbroadcastmac: this is neither broadcast nor my packet, just drop it\n");
	Packet::free(pkt);
	return;
}


void 
BroadcastMac::DropPacket(Packet* pkt)
{
//	printf("broadcast:node %d  gets a stucked packet at  %f\n",index_,NOW);
	if(drop_) drop_->recv(pkt,"Stucked");
	else Packet::free(pkt);
	return;
}


/*
this program is used to handle the transmitted packet, 
it should be virtual function, different class may have 
different versions.
*/


void 
BroadcastMac::TxProcess(Packet* pkt){

	hdr_cmn* cmh=HDR_CMN(pkt);
	hdr_mac* mach = hdr_mac::access(pkt);
	
	mach->macDA() = (int)MAC_BROADCAST;
	mach->macSA() = index_;

	assert(initialized());
	UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;


	if( packet_size_ != 0 )
		cmh->size() = packet_size_;
	else
		cmh->size()+=(packetheader_size_);



	cmh->txtime()=getTxTime(pkt);

	Scheduler& s=Scheduler::instance();
	switch( n->TransmissionStatus() )
	{
	case SLEEP:
		Poweron();
	case IDLE:
		n->SetTransmissionStatus(SEND);
		cmh->direction()=hdr_cmn::DOWN; 
		cmh->addr_type()=NS_AF_ILINK;
		//add the sync hdr
		sendDown(pkt);
		backoff_handler.clear();
		s.schedule(&status_handler,&status_event,cmh->txtime());
		return;
	case RECV:
		{
			double backoff=Random::uniform()*BACKOFF;
			s.schedule(&backoff_handler,(Event*) pkt,backoff);
		}
		return;
	case SEND:
		Packet::free(pkt);
		return;
	default:
		/*
		* all cases have been processed above, so simply return
		*/
		return;			
	}

}


void 
BroadcastMac::CallbackProcess(Event* callback_event)
{
	callback_->handle(callback_event);
	return;
}


void 
BroadcastMac::StatusProcess(Event* p)
{
	UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
	if(SLEEP==n->TransmissionStatus()){
		Scheduler& s=Scheduler::instance();
		s.schedule(&callback_handler,&callback_event,CALLBACK_DELAY);
		return;
	}
	n->SetTransmissionStatus(IDLE);
	Scheduler& s=Scheduler::instance();
	s.schedule(&callback_handler,&callback_event,CALLBACK_DELAY);
	return;
}



int
BroadcastMac::command(int argc, const char*const* argv)
{
	if(argc == 3) {
		if (strcmp(argv[1], "node_on") == 0) {
			Node* n1=(Node*) TclObject::lookup(argv[2]);
			if (!n1) return TCL_ERROR;
			node_ =n1; 
			return TCL_OK;
		}
	}

	return UnderwaterMac::command(argc, argv);
}


