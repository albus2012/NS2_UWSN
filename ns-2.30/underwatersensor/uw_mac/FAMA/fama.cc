#include "fama.h"
#include "underwatersensor/uw_routing/vectorbasedforward.h"



int hdr_FAMA::offset_;
static class FAMA_HeaderClass: public PacketHeaderClass{
public:
	FAMA_HeaderClass():PacketHeaderClass("PacketHeader/FAMA",sizeof(hdr_FAMA))
	{
		bind_offset(&hdr_FAMA::offset_);
	}
}class_FAMA_hdr;


static class FAMAClass : public TclClass {
public:
	FAMAClass():TclClass("Mac/UnderwaterMac/FAMA") {}
	TclObject* create(int, const char*const*) {
		return (new FAMA());
	}
}class_FAMA;


void FAMA_ND_Timer::expire(Event *e)
{
	mac_->sendPkt(mac_->makeND());
	nd_times_--;

	if( nd_times_ > 0 )
		resched(Random::uniform(mac_->NDPeriod));
}


void FAMA_BackoffTimer::expire(Event *e)
{
	mac_->sendRTS(2*mac_->MaxPropDelay
		+mac_->RTSTxTime+mac_->CTSTxTime+mac_->EstimateError);
}


void FAMA_StatusHandler::handle(Event *e)
{
	mac_->StatusProcess();
}


void FAMA_WaitCTSTimer::expire(Event *e)
{
	mac_->doBackoff();
}


void FAMA_DataSendTimer::expire(Event *e)
{
	mac_->processDataSendTimer(this);
}



void FAMA_DataBackoffTimer::expire(Event *e)
{
	mac_->processDataBackoffTimer();
}



void FAMA_RemoteTimer::expire(Event* e)
{
	mac_->processRemoteTimer();
}


void FAMA_CallBackHandler::handle(Event* e)
{
	mac_->CallbackProcess(e);
}


FAMA::FAMA(): UnderwaterMac(), FAMA_Status(PASSIVE), NDPeriod(4), MaxBurst(1),
		DataPktInterval(0.00001), EstimateError(0.001),DataPktSize(500),
		neighbor_id(0), backoff_timer(this), status_handler(this), NDTimer(this), 
		WaitCTSTimer(this),DataBackoffTimer(this),RemoteTimer(this), CallBack_Handler(this)
{
	MaxPropDelay = UnderwaterChannel::Transmit_distance()/1500.0;
	RTSTxTime = MaxPropDelay;
	CTSTxTime = RTSTxTime + 2*MaxPropDelay;
	
	MaxDataTxTime = DataPktSize/bit_rate_;  //1600bits/10kbps

	bind("MaxPropDelay", &MaxPropDelay);

	bind("MaxBurst", &MaxBurst);

	NDTimer.resched(Random::uniform(NDPeriod)+0.000001);
}



void FAMA::sendPkt(Packet *pkt)
{
	hdr_cmn* cmh=HDR_CMN(pkt);
	//hdr_FAMA* FAMAh = hdr_FAMA::access(pkt);

	cmh->direction() = hdr_cmn::DOWN;

	UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;

	double txtime=cmh->txtime();
	Scheduler& s=Scheduler::instance();
	
	switch( n->TransmissionStatus() ) {
		case SLEEP:
			Poweron();

		case IDLE:
			n->SetTransmissionStatus(SEND); 
			cmh->timestamp() = NOW;
			cmh->direction() = hdr_cmn::DOWN;
			sendDown(pkt);
			s.schedule(&status_handler,&status_event,txtime);
			break;
		case RECV:
			printf("RECV-SEND Collision!!!!!\n");
			Packet::free(pkt);
			break;
			
		default:
			//status is SEND
			printf("node%d send data too fast\n",index_);
			Packet::free(pkt);

	}

	return;
}



void FAMA::StatusProcess()
{
	UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
	n->SetTransmissionStatus(IDLE);
	return;
}


void FAMA::TxProcess(Packet* pkt)
{
	Scheduler::instance().schedule(&CallBack_Handler, &callback_event, CALLBACK_DELAY);

	if( NeighborList_.empty() ) {
		Packet::free(pkt);
		return;
	}
	//figure out how to cache the packet will be sent out!!!!!!!
	hdr_cmn* cmh = HDR_CMN(pkt);
	hdr_uwvb* vbh = HDR_UWVB(pkt);
	hdr_FAMA* FAMAh = hdr_FAMA::access(pkt);
	cmh->size() = DataPktSize;
	//cmh->txtime() = MaxDataTxTime;
	cmh->txtime() = getTxTime(cmh->size());
	cmh->error() = 0;
	cmh->direction() = hdr_cmn::DOWN;

	UpperLayerPktType = cmh->ptype();

	cmh->next_hop() = NeighborList_[neighbor_id];
	neighbor_id = (neighbor_id+1)%NeighborList_.size();
	cmh->ptype() = PT_FAMA;

	vbh->target_id.addr_ = cmh->next_hop();
	FAMAh->packet_type = hdr_FAMA::FAMA_DATA;
	FAMAh->SA = index_;
	FAMAh->DA = cmh->next_hop();

	PktQ_.push(pkt);
	
	//fill the next hop when sending out the packet;
	if( (PktQ_.size() == 1) /*the pkt is the first one*/
		&& FAMA_Status == PASSIVE ) {

			if( CarrierDected() ) {
				doRemote(2*MaxPropDelay+EstimateError);
			}
			else{
				sendRTS(2*MaxPropDelay+CTSTxTime+RTSTxTime+EstimateError);
			}
		
	}
}


void FAMA::RecvProcess(Packet *pkt)
{
	hdr_FAMA* FAMAh = hdr_FAMA::access(pkt);
	nsaddr_t dst = FAMAh->DA;
	//int src = mh->macSA();
	hdr_cmn* cmh=HDR_CMN(pkt);

	
	if( backoff_timer.status() == TIMER_PENDING )
	{
		backoff_timer.cancel();
		doRemote(2*MaxPropDelay+EstimateError);
	}
	else if( RemoteTimer.status() == TIMER_PENDING )
	{
		RemoteTimer.cancel();
		RemoteTimer.ExpiredTime = -1;
	}

	/*ND is not a part of FAMA. We just want to use it to get next hop
	 *So we do not care wether it collides with others
	 */
	if( (cmh->ptype()==PT_FAMA) && (FAMAh->packet_type==hdr_FAMA::ND) )
	{
		processND(pkt);
		Packet::free(pkt);
		return;
	}
	
    if( cmh->error() ) 
    {
     	//printf("broadcast:node %d  gets a corrupted packet at  %f\n",index_,NOW);
     	if(drop_)
     	  drop_->recv(pkt,"Error/Collision");
     	else
     	  Packet::free(pkt);

     	doRemote(2*MaxPropDelay+EstimateError);
     	return;
    }


	if( WaitCTSTimer.status() == TIMER_PENDING )
	{

		//printf("%f: node %d receive RTS\n", NOW, index_);
		WaitCTSTimer.cancel();
		if( (cmh->ptype() == PT_FAMA ) 
			&& (FAMAh->packet_type==hdr_FAMA::CTS)
			&& (cmh->next_hop()==index_) )
		{
			//receiving the CTS
			sendDataPkt();
		}
		else
		{
			doBackoff();
		}

		Packet::free(pkt);
		return;
	}


	if( cmh->ptype() == PT_FAMA )
	{

		switch( FAMAh->packet_type )
		{

			case hdr_FAMA::RTS:
				//printf("%f: node %d receive RTS\n", NOW, index_);
				if( dst == index_ )
				{
					processRTS(pkt);
				}
				doRemote(CTSTxTime+2*MaxPropDelay+EstimateError);

				break;
			case hdr_FAMA::CTS:
				//printf("%f: node %d receive CTS\n", NOW, index_);
				// this CTS must be not for this node
				doRemote(2*MaxPropDelay+EstimateError);
				break;
			default:
				//printf("%f: node %d receive DATA\n", NOW, index_);
				//process Data packet
				if( dst == index_ )
				{
					cmh->ptype() = UpperLayerPktType;
					sendUp(pkt);
					return;
				}
				else
				{
					doRemote(MaxPropDelay+EstimateError);
				}
				
		}
	}
	
	Packet::free(pkt);	
}




void FAMA::sendDataPkt()
{
	int PktQ_Size = PktQ_.size();
	int SentPkt = 0;
	Time StartTime = NOW;
	nsaddr_t recver = HDR_CMN(PktQ_.front())->next_hop();
	Packet* tmp = NULL;
	for(int i=0; i<PktQ_Size && SentPkt<MaxBurst; i++) {
		tmp = PktQ_.front();
		PktQ_.pop();
		if( HDR_CMN(tmp)->next_hop() == recver ) {
			SentPkt++;
			FAMA_DataSendTimer* DataSendTimer = new FAMA_DataSendTimer(this);
			DataSendTimer->pkt_ = tmp;
			DataSendTimer->resched(StartTime-NOW);
			DataSendTimerSet.insert(DataSendTimer);
			if( !PktQ_.empty() ) {
				StartTime += HDR_CMN(PktQ_.front())->txtime() + DataPktInterval;
			}
			else {
				break;
			}
		}
		else{
			PktQ_.push(tmp);
		}
	}

	FAMA_Status = WAIT_DATA_FINISH;

	DataBackoffTimer.resched(MaxPropDelay+StartTime-NOW);
}


void FAMA::processDataSendTimer(FAMA_DataSendTimer * DataSendTimer)
{
	FAMA_DataSendTimer* tmp = DataSendTimer;
	sendPkt(DataSendTimer->pkt_);
	DataSendTimer->pkt_ = NULL;
	DataSendTimerSet.erase(DataSendTimer);
	delete tmp;
}



void FAMA::processDataBackoffTimer()
{
	if( !PktQ_.empty() )
		doBackoff();
	else
		FAMA_Status = PASSIVE;
}


Packet* FAMA::makeND()
{
	Packet* pkt = Packet::alloc();
	hdr_cmn* cmh = HDR_CMN(pkt);
	hdr_FAMA* FAMAh = hdr_FAMA::access(pkt);

	cmh->size() = 2*sizeof(nsaddr_t)+1;
	cmh->txtime() = getTxTimebyPktSize(cmh->size());
	cmh->txtime() = getTxTime(cmh->size());
	cmh->error() = 0;
	cmh->direction() = hdr_cmn::DOWN;
	cmh->ptype() = PT_FAMA;
	cmh->next_hop() = MAC_BROADCAST;

	FAMAh->packet_type = hdr_FAMA::ND;
	FAMAh->SA = index_;
	FAMAh->DA = MAC_BROADCAST;

	return pkt;
}




void FAMA::processND(Packet *pkt)
{
	hdr_FAMA* FAMAh = hdr_FAMA::access(pkt);
	NeighborList_.push_back(FAMAh->SA);
	return;
}



Packet* FAMA::makeRTS(nsaddr_t Recver)
{
	Packet* pkt = Packet::alloc();
	hdr_cmn* cmh = HDR_CMN(pkt);
	hdr_FAMA* FAMAh = hdr_FAMA::access(pkt);


	cmh->size() = getPktSizebyTxTime(RTSTxTime);
	cmh->txtime() = RTSTxTime;
	//cmh->txtime() = getTxTime(cmh->size());
	cmh->error() = 0;
	cmh->direction() = hdr_cmn::DOWN;
	cmh->ptype() = PT_FAMA;
	cmh->next_hop() = Recver;

	FAMAh->packet_type = hdr_FAMA::RTS;
	FAMAh->SA = index_;
	FAMAh->DA = Recver;

	return pkt;
}



void FAMA::sendRTS(Time DeltaTime)
{ 
	hdr_cmn* cmh = HDR_CMN( PktQ_.front() );
	sendPkt( makeRTS(cmh->next_hop()) );
	FAMA_Status = WAIT_CTS;
	WaitCTSTimer.resched(DeltaTime);
}


void FAMA::processRTS(Packet *pkt)
{
	nsaddr_t RTS_Sender = hdr_FAMA::access(pkt)->SA;
	sendPkt(makeCTS(RTS_Sender));
	FAMA_Status = WAIT_DATA;
}



Packet* FAMA::makeCTS(nsaddr_t RTS_Sender)
{
	Packet* pkt = Packet::alloc();
	hdr_cmn* cmh = HDR_CMN(pkt);
	hdr_FAMA* FAMAh = hdr_FAMA::access(pkt);

	cmh->size() = getPktSizebyTxTime(CTSTxTime);
	cmh->txtime() = CTSTxTime;
	//cmh->txtime() = getTxTime(cmh->size());
	cmh->error() = 0;
	cmh->direction() = hdr_cmn::DOWN;
	cmh->ptype() = PT_FAMA;
	cmh->next_hop() = RTS_Sender;

	FAMAh->packet_type = hdr_FAMA::CTS;
	FAMAh->SA = index_;
	FAMAh->DA = RTS_Sender;

	return pkt;
}



/*PktSize is in byte*/
Time FAMA::getTxTimebyPktSize(int PktSize)
{
	return (PktSize*8)*encoding_efficiency_/bit_rate_;
}

int FAMA::getPktSizebyTxTime(Time TxTime)
{
	return int((TxTime*bit_rate_)/(8*encoding_efficiency_));
}


bool FAMA::CarrierDected()
{
	UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
	if( n->TransmissionStatus() == RECV
		|| n->TransmissionStatus() == SEND )  {
		
		return true;
	}
	else {
		return false;
	}

}

void FAMA::doBackoff()
{
	Time BackoffTime = Random::uniform(10*RTSTxTime);
	FAMA_Status = BACKOFF;
	if( backoff_timer.status() == TIMER_PENDING ) {
		backoff_timer.cancel();
	}
	
	backoff_timer.resched(BackoffTime);
}


void FAMA::doRemote(Time DeltaTime)
{
	FAMA_Status = REMOTE;
	
	if( NOW+DeltaTime > RemoteTimer.ExpiredTime ) {
		RemoteTimer.ExpiredTime = NOW+DeltaTime;
		if( RemoteTimer.status() == TIMER_PENDING ) {
			RemoteTimer.cancel();		
		}
		RemoteTimer.resched(DeltaTime);	
	}
}




void FAMA::processRemoteTimer()
{
	if( PktQ_.empty() ) {
		FAMA_Status = PASSIVE;
	}
	else {
		doBackoff();
		//sendRTS(2*MaxPropDelay+CTSTxTime+RTSTxTime+EstimateError);
	}
}



void FAMA::CallbackProcess(Event* e)
{
	callback_->handle(e);
}


int FAMA::command(int argc, const char *const *argv)
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


