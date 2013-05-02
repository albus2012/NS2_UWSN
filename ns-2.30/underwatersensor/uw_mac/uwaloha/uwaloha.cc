#include "packet.h"
#include "random.h"
#include "underwatersensor/uw_common/underwatersensornode.h"
#include "mac.h"
#include "uwaloha.h"
#include "../underwaterphy.h"


int hdr_UWALOHA::offset_;
static class UWALOHA_HeaderClass: public PacketHeaderClass{
public:
	UWALOHA_HeaderClass():PacketHeaderClass("PacketHeader/UWALOHA",sizeof(hdr_UWALOHA))
	{
		bind_offset(&hdr_UWALOHA::offset_);
	}
}class_UWALOHA_hdr;


static class UWALOHAClass : public TclClass {
public:
	UWALOHAClass():TclClass("Mac/UnderwaterMac/UWALOHA") {}
	TclObject* create(int, const char*const*) {
		return (new UWALOHA());
	}
}class_UWALOHA;



/*===========================UWALOHA Timer===========================*/
long UWALOHA_ACK_RetryTimer::id_generator = 0;

void UWALOHA_ACK_RetryTimer::expire(Event* e)
{
	  mac_->processRetryTimer(this);
}

void UWALOHA_BackoffTimer::expire(Event *e)
{
	mac_->sendDataPkt();
}

void UWALOHA_WaitACKTimer::expire(Event *e) //WaitACKTimer expire
{
	mac_->doBackoff();	
}



//construct function
UWALOHA::UWALOHA(): UnderwaterMac(), bo_counter(0), UWALOHA_Status(PASSIVE), Persistent(1.0),
		ACKOn(1), Min_Backoff(0.0), Max_Backoff(1.5), MAXACKRetryInterval(0.05), 
		blocked(false), BackoffTimer(this), WaitACKTimer(this),
		CallBack_handler(this), status_handler(this)
{
	MaxPropDelay = UnderwaterChannel::Transmit_distance()/1500.0;

	bind("Persistent",&Persistent);
	bind("ACKOn",&ACKOn);
	bind("Min_Backoff",&Min_Backoff);
	bind("Max_Backoff",&Max_Backoff);
	bind("WaitACKTime",&WaitACKTime);
}


void UWALOHA::doBackoff()
{
//	printf("node %d doBackoff at %f \n", index_,NOW);
	  Time BackoffTime=Random::uniform(Min_Backoff,Max_Backoff);
	  bo_counter++;
	  if (bo_counter < MAXIMUMCOUNTER) {
		  UWALOHA_Status = BACKOFF;
		  BackoffTimer.resched(BackoffTime);
	  }
	  else {
		  bo_counter=0;
		  printf("backoffhandler: too many backoffs\n");
		  Packet::free(PktQ_.front());
		  PktQ_.pop();
		  processPassive();		
	  }
}


void UWALOHA::processPassive()
{
	if (UWALOHA_Status == PASSIVE && !blocked) {
		if (!PktQ_.empty() )
			sendDataPkt();
	}
}


void UWALOHA_StatusHandler::handle(Event *e)//What's this?
{
	mac_->StatusProcess(is_ack_);
}

void UWALOHA_CallBackHandler::handle(Event* e)
{
	mac_->CallbackProcess(e);
}

void UWALOHA::StatusProcess(bool is_ack)
{
	UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
	n->SetTransmissionStatus(IDLE);
	
	if( blocked ) {
		blocked = false;
		processPassive();
		return;
	}
	
	if( !ACKOn ) {
		/*Must be DATA*/
		UWALOHA_Status = PASSIVE;
		processPassive();
	}
	else if (ACKOn && !is_ack ) {
		UWALOHA_Status = WAIT_ACK;
	}
	
}

void UWALOHA::CallbackProcess(Event* e)
{
	callback_->handle(e);
}


/*===========================Send and Receive===========================*/

void UWALOHA::TxProcess(Packet* pkt)
{
	Scheduler::instance().schedule(&CallBack_handler, &callback_event, CALLBACK_DELAY);

	hdr_cmn* cmh = HDR_CMN(pkt);
	hdr_UWALOHA* UWALOHAh = hdr_UWALOHA::access(pkt);
	cmh->size() += hdr_UWALOHA::size();
	cmh->txtime() = getTxTime(cmh->size());
	cmh->error() = 0;
	cmh->direction() = hdr_cmn::DOWN;

	Time t = NOW;
	if( t > 500 ) 
	  t = NOW;
	UWALOHAh->packet_type = hdr_UWALOHA::DATA;
	UWALOHAh->SA = index_;
	
	if( cmh->next_hop() == (nsaddr_t)IP_BROADCAST ) {
		UWALOHAh->DA = MAC_BROADCAST;
	}
	else {
		UWALOHAh->DA = cmh->next_hop();
	}

	PktQ_.push(pkt);//push packet to the queue
	
	//fill the next hop when sending out the packet;
	if(UWALOHA_Status == PASSIVE 
		&& PktQ_.size() == 1 && !blocked ) 
	{
		sendDataPkt();
	}
}


void UWALOHA::sendDataPkt()
{	
	double P = Random::uniform(0,1);
	Packet* tmp = PktQ_.front();
	nsaddr_t recver = HDR_CMN(tmp)->next_hop();
	
	UWALOHA_Status = SEND_DATA;
	
	if( P<=Persistent ) {
		if( HDR_CMN(tmp)->next_hop() == recver )// {
			sendPkt(tmp->copy());
	}
	else {
		//Binary Exponential Backoff
		bo_counter--;
		doBackoff();
	}

	return;

}


void UWALOHA::sendPkt(Packet *pkt)
{
	hdr_cmn* cmh=HDR_CMN(pkt);	//what's this?
	hdr_UWALOHA* UWALOHAh = hdr_UWALOHA::access(pkt);

	cmh->direction() = hdr_cmn::DOWN;
	
	UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
	
	double txtime=cmh->txtime();
	Scheduler& s=Scheduler::instance();

	switch( n->TransmissionStatus() ) {
		case SLEEP:
			Poweron();
			
		case IDLE:
		  
			n->SetTransmissionStatus(SEND); 
			cmh->timestamp() = NOW;//why?
			cmh->direction() = hdr_cmn::DOWN;
			
			//ACK doesn't affect the status, only process DATA here
			if (UWALOHAh->packet_type == hdr_UWALOHA::DATA) {
				//must be a DATA packet, so setup wait ack timer 
				if ((UWALOHAh->DA != (nsaddr_t)MAC_BROADCAST) && ACKOn) {
					UWALOHA_Status = WAIT_ACK;
					WaitACKTimer.resched(WaitACKTime+txtime);
				}
				else {
					Packet::free(PktQ_.front());
					PktQ_.pop();
					UWALOHA_Status = PASSIVE;
				}
				status_handler.is_ack() = false;
			}
			else{
				status_handler.is_ack() = true;
			}
			sendDown(pkt);		//UnderwaterMAC.cc
			
			blocked = true;
			s.schedule(&status_handler,&status_event,txtime+0.01);
			break;
			
		case RECV:
			printf("RECV-SEND Collision!!!!!\n");
			if( UWALOHAh->packet_type == hdr_UWALOHA::ACK ) 
				retryACK(pkt);
			else
				Packet::free(pkt);
			
			UWALOHA_Status = PASSIVE;
			break;
			
		default:
		//status is SEND
			printf("node%d send data too fast\n",index_);
			if( UWALOHAh->packet_type == hdr_UWALOHA::ACK ) 
				retryACK(pkt);
			else
				Packet::free(pkt);
			UWALOHA_Status = PASSIVE;
	}
}

void UWALOHA::RecvProcess(Packet *pkt)
{
	hdr_UWALOHA* UWALOHAh = hdr_UWALOHA::access(pkt);
	hdr_cmn* cmh=HDR_CMN(pkt);
	nsaddr_t recver = UWALOHAh->DA;

	if( cmh->error() ) 
	{
	  if(drop_	&&	recver==index_) {
		  drop_->recv(pkt,"Error/Collision");
	  }
	  else
		  Packet::free(pkt);	
	  
	  //processPassive();
	  return;
	}

	if( UWALOHAh->packet_type == hdr_UWALOHA::ACK ) {
			//if get ACK after WaitACKTimer, ignore ACK
			if( recver == index_ && UWALOHA_Status == WAIT_ACK) {
				WaitACKTimer.cancel();
				bo_counter=0;
				Packet::free(PktQ_.front());
				PktQ_.pop();
				UWALOHA_Status=PASSIVE;
				processPassive();
			}
	}
	else if(UWALOHAh->packet_type == hdr_UWALOHA::DATA) {
			//process Data packet
			if( recver == index_ || recver == (nsaddr_t)MAC_BROADCAST ) {
				cmh->size() -= hdr_UWALOHA::size();
				sendUp(pkt->copy());	//UnderwaterMAC.cc
				if ( ACKOn && (recver != (nsaddr_t)MAC_BROADCAST))
					replyACK(pkt->copy());
				else 
					processPassive();
			}

	}
	Packet::free(pkt);	
}

void UWALOHA::replyACK(Packet *pkt)//sendACK
{
	nsaddr_t Data_Sender = hdr_UWALOHA::access(pkt)->SA;
	
	sendPkt(makeACK(Data_Sender));
	bo_counter=0;
	Packet::free(pkt);
}

Packet* UWALOHA::makeACK(nsaddr_t Data_Sender)
{
	Packet* pkt = Packet::alloc();
	hdr_cmn* cmh = HDR_CMN(pkt);
	hdr_UWALOHA* UWALOHAh = hdr_UWALOHA::access(pkt);

	cmh->size() = hdr_UWALOHA::size();
	cmh->txtime() = getTxTime(cmh->size());
	cmh->error() = 0;
	cmh->direction() = hdr_cmn::DOWN;
	cmh->next_hop() = Data_Sender;
	cmh->ptype() = PT_UWALOHA;

	UWALOHAh->packet_type = hdr_UWALOHA::ACK;
	UWALOHAh->SA = index_;
	UWALOHAh->DA = Data_Sender;

	return pkt;
}


void UWALOHA::processRetryTimer(UWALOHA_ACK_RetryTimer* timer)
{
	Packet* pkt = timer->pkt();
	if( RetryTimerMap_.count(timer->id()) != 0 ) {
		RetryTimerMap_.erase(timer->id());
	} else {
		printf("error: cannot find the ack_retry timer\n");
	}
	delete timer;
	sendPkt(pkt);
}


void UWALOHA::retryACK(Packet* ack)
{
	UWALOHA_ACK_RetryTimer* timer = new UWALOHA_ACK_RetryTimer(this, ack);
	timer->resched(MAXACKRetryInterval*Random::uniform());
	RetryTimerMap_[timer->id()] = timer;
}




int UWALOHA::command(int argc, const char *const *argv)
{
	return UnderwaterMac::command(argc, argv);
}















