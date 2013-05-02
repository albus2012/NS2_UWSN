#include "copemac.h"
#include "underwatersensor/uw_routing/vectorbasedforward.h"

int hdr_otman::offset_;
static class OTMAN_HeaderClass: public PacketHeaderClass{
public:
	OTMAN_HeaderClass():PacketHeaderClass("PacketHeader/OTMAN",sizeof(hdr_otman))
	{
		bind_offset(&hdr_otman::offset_);
	}
}class_otman_hdr;

static class OTMANClass : public TclClass {
public:
	OTMANClass():TclClass("Mac/UnderwaterMac/OTMAN") {}
	TclObject* create(int, const char*const*) {
		return (new OTMAN());
	}
}class_otman;




void OTMAN_CallbackHandler::handle(Event* e)
{
	mac_->CallbackProcess(e);
}

void OTMAN_StatusHandler::handle(Event *e)
{
	mac_->StatusProcess(e);
}


void OTMAN_SendDelayHandler::handle(Event *e)
{
	mac_->SendPkt( (Packet*)e );
}


void OTMAN_BackoffHandler::handle(Event* e)
{
	counter++;
	if(counter < MAXIMUMCOUNTER) {
		mac_->TxProcess(pkt_);
		pkt_ = NULL;
	}
	else 
	{
		clear();
		printf("backoffhandler: too many backoffs\n");
		if( mac_->drop_ )
			mac_->drop_->recv(pkt_, "Backoff too many times");		
		else
			Packet::free(pkt_);
	}
}

void OTMAN_BackoffHandler::clear()
{
	counter = 0;
}

void OTMAN_NDProcess_Initor::expire(Event* e)
{
	mac_->CtrlPktQ_.insert(mac_->makeND(), Random::uniform(mac_->NDWin));
	mac_->ndreply_initor.resched(mac_->NDWin + 0.9); //1.0 is delay
	MaxTimes_ --;
	if( MaxTimes_ > 0 ) {
		resched(mac_->NDInterval_);
	}
}

void OTMAN_NDReply_Initor::expire(Event *e)
{
	Packet* nd_reply = mac_->makeNDReply();
	if( nd_reply != NULL ) {
		mac_->CtrlPktQ_.insert(nd_reply, Random::uniform(mac_->NDReplyWin));
		//mac_->PreSendPkt(nd_reply, Random::uniform(mac_->NDReplyWin));
	}
}


void OTMAN_DataSendTimer::expire(Event* e)
{
	resched(mac_->DataAccuPeriod+Random::uniform(2.0));
	mac_->startHandShake();
}

void OTMAN_RevAckAccumTimer::expire(Event* e)
{
	if( mac_->PendingRevs.size() != 0 ) {
		Packet* tmp = mac_->makeMultiRevAck();
		//mac_->PreSendPkt();
		mac_->CtrlPktQ_.insert(tmp, 
			mac_->RevQ_.getValidStartTime(HDR_CMN(tmp)->txtime()));
		mac_->PendingRevs.clear();
	}
}

void OTMAN_DataAckAccumTimer::expire(Event *e)
{
	Packet* tmp = mac_->makeDataAck();
	//mac_->PreSendPkt(mac_->makeDataAck());
	mac_->CtrlPktQ_.insert(tmp, 
			mac_->RevQ_.getValidStartTime(HDR_CMN(tmp)->txtime()));
	mac_->PendingDataAcks.clear();
}

void OTMAN_InitiatorTimer::expire(Event *e)
{
	mac_->start();
}


void PktWareHouse::insert2PktQs(Packet* pkt)
{
	hdr_cmn* cmh=HDR_CMN(pkt);
	PktElem* temp = new PktElem(pkt);

	CachedPktNum_ ++;
	if( Queues[cmh->next_hop()].head == NULL ) {
		Queues[cmh->next_hop()].tail = temp;
		Queues[cmh->next_hop()].head = temp;
	} 
	else {
		Queues[cmh->next_hop()].tail->next_ = temp;
		Queues[cmh->next_hop()].tail = temp;
	}
}

bool PktWareHouse::deletePkt(nsaddr_t Recver, int SeqNum)
{

	PktElem* pos = Queues[Recver].head;
	PktElem* pre_pos = NULL;

	while ( pos != NULL ) {
		if( HDR_CMN(pos->pkt_)->uid() == SeqNum ) {
			if( pre_pos == NULL ) {
				Queues[Recver].head = pos->next_;
			}
			else {
				pre_pos->next_ = pos->next_;
			}

			Packet::free(pos->pkt_);
			pos->pkt_ == NULL;
			delete pos;
			CachedPktNum_ --;
			return true;
		}
		pre_pos = pos;
		pos = pos->next_;
	}
	
	return false;
}














/**********************RevQueues******************************************/
RevElem::RevElem(){
	send_timer = NULL;
	next = NULL;
}


RevElem::RevElem(int RevID_, Time StartTime_, Time EndTime_, 
				 nsaddr_t Reservor_, RevType rev_type_):
	StartTime(StartTime_), EndTime(EndTime_), 
		Reservor(Reservor_), rev_type(rev_type_), RevID(RevID_)
{
	send_timer = NULL;
	next = NULL;
}


RevElem::~RevElem()
{
	if( send_timer != NULL ) {
		if( send_timer->status() == TIMER_PENDING ) {
			send_timer->cancel();
		}
		delete send_timer;
		send_timer = NULL;
	}
}


PktSendTimer::~PktSendTimer()
{
	if( pkt_ != NULL )
		Packet::free(pkt_);
}

void PktSendTimer::expire(Event *e)
{
	mac_->PreSendPkt(pkt_);
	pkt_ = NULL;

}


RevQueues::RevQueues(OTMAN* mac): mac_(mac)
{
	Head_ = NULL;
}


RevQueues::~RevQueues()
{
	RevElem* tmp = NULL;

	while( Head_ != NULL ) {
		tmp = Head_;
		Head_ = Head_->next;
		delete tmp;
	}
}

void RevQueues::clearExpired(Time ExpireTime)
{
	RevElem* tmp = NULL;

	while( Head_ != NULL && Head_->EndTime < ExpireTime + BACKOFF_DELAY_ERROR ) {
		tmp = Head_;
		Head_ = Head_->next;
		delete tmp;
	}
}


bool RevQueues::push(int RevID, Time StartTime, Time EndTime, 
					 nsaddr_t Reservor, RevType rev_type, Packet *pkt)
{
	clearExpired(NOW);

	//insert 

	RevElem* tmp = new RevElem(RevID, StartTime, EndTime, Reservor, rev_type);
	
	if( pkt != NULL ) {
		tmp->send_timer = new PktSendTimer(mac_, pkt);
	}

	if( Head_ == NULL ) {
		Head_ = tmp;
		return true;
	}

	RevElem* pre_pos = Head_;
	RevElem* pos = Head_->next;
	
	while( pos != NULL ) {
		if( pos->EndTime < EndTime ) {
			pre_pos = pos;
			pos = pos->next;
		}
		else {
			break;
		}
	}

	pre_pos->next = tmp;
	tmp->next = pos;
	return true;
}


bool RevQueues::checkAvailability(Time StartTime, Time EndTime, RevType rev_type)
{
	//recheck the code here.
	clearExpired(NOW);

	RevElem* pos = Head_;

	while( pos != NULL ) {
		if( ((pos->StartTime > StartTime) && (pos->StartTime < EndTime) )
			|| ( (pos->EndTime > StartTime)&&(pos->EndTime<EndTime) ) ) {
			return false;
		}

		pos = pos->next;
	}
	return true;
}



void RevQueues::deleteRev(int RevID)
{
	RevElem* pos = Head_;
	
	if( Head_->RevID == RevID ) {
		Head_ = Head_->next;
		delete pos;
		return;
	}

	pos = Head_->next;
	RevElem* pre_pos = Head_;

	while( pos != NULL ) {
		if( pos->RevID == RevID ) {
			pre_pos->next = pos->next;
			
			delete pos;
			return;
		}
		pre_pos = pos;
		pos = pos->next;
	}
	
}


void RevQueues::updateStatus(int RevID, RevType new_type)
{
	RevElem* pos = Head_;
	Time send_time;
	
	while( pos != NULL ) {
		if( pos->RevID == RevID ) {
			pos->rev_type = new_type;
			send_time = pos->StartTime - NOW + mac_->GuardTime/2;
			if( send_time < 0.0 ) {
				printf("handshake takes too long time, cancel sending\n");
				deleteRev(RevID);
				return;
			}

			if( pos->send_timer != NULL && send_time > 0.0) {

				pos->send_timer->resched(send_time);

			}
			return;
		}
		pos = pos->next;
	}
}

void RevQueues::printRevQueue()
{
	RevElem* pos = Head_;
	char file_name[30];
	strcpy(file_name, "schedule_");
	file_name[strlen(file_name)+1] = '\0';
	file_name[strlen(file_name)] = char(mac_->index_+'0');
	FILE* stream = fopen(file_name, "a");

	while( pos != NULL ) {
		fprintf(stream, "node(%d): %d[%f:%f] type:%d\t", pos->Reservor, 
			pos->RevID, pos->StartTime, pos->EndTime, pos->rev_type);
		pos = pos->next;
	}
	fprintf(stream, "\n");
	fclose(stream);


}

Time RevQueues::getValidStartTime(Time Interval, Time SinceTime)
{
	Time now = SinceTime;
	RevElem* pos = Head_;
	RevElem* pre_pos = NULL;
	Time Lowerbound, Upperbound;
	

	while( pos!=NULL && pos->StartTime < now ) {
		pre_pos = pos;
		pos = pos->next;
	}

	Lowerbound = now;

	if( pos == NULL ) {
		return SinceTime + 0.0001 - NOW;
	}
	else {
		Upperbound = pos->StartTime;
	}

	while( Upperbound - Lowerbound < Interval ) {
		pre_pos = pos;
		pos = pos->next;
		Lowerbound = pre_pos->EndTime;
		if( pos == NULL )
			break;
		Upperbound = pos->StartTime;
	}

	return Lowerbound + 0.0001 - NOW;

}












/**********************OTMAN*************************************/
int OTMAN::RevID = 0;
OTMAN::OTMAN(): UnderwaterMac(), callback_handler(this), 
			status_handler(this), senddelay_handler(this),
			backoff_handler(this), nd_process_initor(this),
			ndreply_initor(this),
			DataSendTimer(this), RevAckAccumTimer(this),
			DataAckAccumTimer(this), InitiatorTimer(this),
			PrintTimer(this), NDInterval_(6),
			DataAccuPeriod(10), 
			RevAckAccumTime_(1), DataAckAccumTime_(1),
			 RevQ_(this), next_hop(0),neighbor_id(0),
			MajorIntervalLB(2),/* MajorIntervalUB(3),IntervalStep(0.1),*/
			DataStartTime(15), GuardTime(0.01),
			NDWin(2.0), NDReplyWin(2.0), AckTimeOut(10),
			CtrlPktQ_(this),PktSize(200), isParallel(1)
			
{
	bind("NDInterval", &NDInterval_);
	bind("DataAccuPeriod", &DataAccuPeriod);  //the reservation period
	bind("RevAckAccumTime", &RevAckAccumTime_);
	bind("DataAckAccumTime", &DataAckAccumTime_);

	//bind("MajorBackupInterval", &MajorBackupInterval);
	bind("MajorIntervalLB", &MajorIntervalLB);
	/*bind("MajorIntervalUB", &MajorIntervalUB);*/
	bind("GuardTime", &GuardTime);
	bind("isParallel", &isParallel);

	nd_process_initor.resched(0.002 );  //start the nd process
	InitiatorTimer.resched(0.001);
	
	
}

void OTMAN::start()
{
	((UnderwaterSensorNode*) node_)->SetTransmissionStatus(IDLE);
	
	DataStartTime = 3*NDInterval_ + 7;
	Time MaxRTT = 2*1000/1500.0;
	AckTimeOut = RevAckAccumTime_ + DataAckAccumTime_ + 2*MaxRTT + 5; //5 is time error
	DataSendTimer.resched(DataStartTime+Random::uniform());
	//PrintTimer.resched(100.0);
	//Random::seed_heuristically();
}

int OTMAN::command(int argc, const char *const *argv)
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



void OTMAN::TxProcess(Packet* pkt)
{
	hdr_mac* mh=hdr_mac::access(pkt);
	hdr_cmn* cmh = HDR_CMN(pkt);
	hdr_uwvb* vbh = HDR_UWVB(pkt);

	cmh->size() = PktSize;
	cmh->txtime()=
			(cmh->size()*encoding_efficiency_)*8/bit_rate_;
	//cmh->next_hop() = ((UnderwaterSensorNode*)node_)->next_hop; //for purpose of test

	if( prop_delays.size() == 0 ) {
		printf("node %d doesn't have neighbor.\n", index_);
		Packet::free(pkt);
		return;
	}

	cmh->error()=0;
	cmh->direction() = hdr_cmn::DOWN;
	mh->macDA() = cmh->next_hop();
	mh->macSA() = index_;

	//insert packet into packet warehouse
	PktWH_.insert2PktQs(pkt);
	/*if( DataSendTimer.status() == TIMER_IDLE ) {
		DataSendTimer.resched(DataAccuPeriod+Random::uniform());
	}*/
	Scheduler::instance().schedule(&callback_handler, 
						&callback_event, OTMAN_CALLBACK_DELAY);
}


void OTMAN::RecvProcess(Packet* pkt)
{
	hdr_mac* mh=hdr_mac::access(pkt);
	int dst = mh->macDA();
	//int src = mh->macSA();
	hdr_cmn* cmh=HDR_CMN(pkt);
	
    if( cmh->error() ) 
    {
     	//printf("broadcast:node %d  gets a corrupted packet at  %f\n",index_,NOW);
     	if(drop_)
			drop_->recv(pkt,"Error/Collision");
     	else
			Packet::free(pkt);

     	return;
    }

	if( dst == index_ || (u_int32_t)dst == MAC_BROADCAST ) {
	
		if( cmh->ptype() == PT_OTMAN ) {
			hdr_otman* hdr_o = hdr_otman::access(pkt);
			switch( hdr_o->packet_type ) {
				case OTMAN_ND:
					processND(pkt);
					break;
				case OTMAN_ND_REPLY:
					processNDReply(pkt);
					break;
				case MULTI_REV:
					processMultiRev(pkt);
					break;
				case MULTI_REV_ACK:
					processMultiRevAck(pkt);
					break;
				case MULTI_DATA_ACK:
					processDataAck(pkt);
					break;
				default:
					;
			}
		}
		else {
			//DATA Packet
			
			printf("RECV Data Pkt\n");
			RecordDataPkt(pkt);
			sendUp(pkt);   //record the data received.!!!!
			//Packet::free(pkt);
			printResult();
			return;
		}
	}
	
	Packet::free(pkt);
}

void OTMAN::PreSendPkt(Packet* pkt, Time delay)
{
	if( delay < 0 )
		delay = delay;
	hdr_cmn* cmh = HDR_CMN(pkt);
	cmh->direction() = hdr_cmn::DOWN;
	Scheduler& s=Scheduler::instance();
	s.schedule(&senddelay_handler, (Event*)pkt, delay);
}


void OTMAN::SendPkt(Packet *pkt)
{
	hdr_cmn* cmh=HDR_CMN(pkt);

	UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;

	cmh->txtime()=(cmh->size()*encoding_efficiency_)/bit_rate_;


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
			backoff_handler.clear();
			s.schedule(&status_handler,&status_event,txtime);
			break;

		case RECV:
			/*printf("node %d backoff at %f\n", index_, NOW);
			if( backoff_handler.pkt_ == NULL ) {
				backoff_handler.pkt_ = pkt;
				s.schedule(&backoff_handler, &backoff_event,
					0.01+Random::uniform()*OTMAN_BACKOFF_TIME);
			}
			else {*/
				Packet::free(pkt);
			/*}*/
			break;
			
		default:
			//status is SEND
			printf("node%d send data too fast\n",index_);
			Packet::free(pkt);

	}

	return;

}



Packet* OTMAN::makeND()
{
	Packet* pkt = Packet::alloc();
	hdr_otman* hdr_o = hdr_otman::access(pkt);
	hdr_cmn* cmh = HDR_CMN(pkt);

	hdr_o->packet_type = OTMAN_ND;
	//hdr_o->hdr.ndh.send_time_ = NOW;

	cmh->size() = hdr_o->size();
    cmh->next_hop() = MAC_BROADCAST; 
    cmh->direction()=hdr_cmn::DOWN; 
    cmh->addr_type()=NS_AF_ILINK;
    cmh->ptype() = PT_OTMAN;

	hdr_mac* mh=hdr_mac::access(pkt);
	mh->macDA() = MAC_BROADCAST;
	mh->macSA() = index_;

	//fill the neighbors that this node already knows the delay
	uint data_size = sizeof(uint)+ sizeof(nsaddr_t)*prop_delays.size();
	pkt->allocdata( data_size);
	//cmh->size() += data_size;

	unsigned char* walk = (unsigned char*)pkt->accessdata();
    *(uint*)walk = prop_delays.size();
    walk += sizeof(uint);

	for( map<nsaddr_t, Time>::iterator pos=prop_delays.begin();
         pos != prop_delays.end(); pos++)
    {
		*((nsaddr_t*)walk) = pos->first;
        walk += sizeof(nsaddr_t);
    }

	return pkt;
}


void OTMAN::processND(Packet* pkt)
{
	//hdr_otman* hdr_o = hdr_otman::access(pkt);
	hdr_mac* mh=hdr_mac::access(pkt);
	hdr_cmn* cmh = HDR_CMN(pkt);

	//printf("node %d recve ND from node %d\n", index_, mh->macSA());

	unsigned char* walk = (unsigned char*)pkt->accessdata();
	uint node_num_ = *((uint*)walk);
    walk += sizeof(uint);

	for( uint i=0; i<node_num_; i++) {
		
		if( index_ == *((nsaddr_t*)walk) ) {
			return;
		}
		walk += sizeof(nsaddr_t);
	}

	PendingND[mh->macSA()].nd_sendtime = cmh->timestamp();
	PendingND[mh->macSA()].nd_recvtime = NOW - cmh->txtime();


	return;
}


Packet* OTMAN::makeNDReply()
{
	if( PendingND.size() == 0 ) {
		printf("no pending nd at node %d\n", index_);
		return NULL;
	}

	Packet* pkt = Packet::alloc();
	hdr_otman* hdr_o = hdr_otman::access(pkt);
	hdr_cmn* cmh = HDR_CMN(pkt);

	hdr_o->packet_type = OTMAN_ND_REPLY;
	//hdr_o->hdr.nd_reply_h.nd_send_time_ = nd_depart_neighbor_time[NDSender];
	//hdr_o->hdr.nd_reply_h.delay_at_receiver = NOW - nd_receive_time[NDSender];
	
	cmh->size() = hdr_o->size();
    cmh->next_hop() = MAC_BROADCAST; 
    cmh->direction()=hdr_cmn::DOWN; 
    cmh->addr_type()=NS_AF_ILINK;
    cmh->ptype() = PT_OTMAN;

	uint data_size = sizeof(uint) + (2*sizeof(Time)+sizeof(nsaddr_t))*PendingND.size();
	pkt->allocdata( data_size);
	//cmh->size() += data_size;
	unsigned char* walk = (unsigned char*)pkt->accessdata();
	*(uint*)walk = PendingND.size();
	walk += sizeof(uint);
	for( map<nsaddr_t, NDRecord>::iterator pos = PendingND.begin();
		pos != PendingND.end(); pos++) {
			*(nsaddr_t*)walk = pos->first;
			walk += sizeof(nsaddr_t);
			*(Time*)walk = pos->second.nd_sendtime;
			walk += sizeof(Time);
			*(Time*)walk = pos->second.nd_recvtime;
			walk += sizeof(Time);
	}

	hdr_mac* mh=hdr_mac::access(pkt);
	mh->macDA() = cmh->next_hop();
	mh->macSA() = index_;

	processNDReply(pkt);

	return pkt;
}


void OTMAN::processNDReply(Packet* pkt)
{
	//hdr_otman* hdr_o = hdr_otman::access(pkt);
	hdr_mac* mh=hdr_mac::access(pkt);
	hdr_cmn* cmh = HDR_CMN(pkt);
	printf("node %d recve ND reply from node %d at %f\n", index_, mh->macSA(), NOW);

	unsigned char* walk = (unsigned char*)pkt->accessdata();
	uint rec_num = *(uint*)walk;
	walk += sizeof(uint);

	NDRecord tmp;
	for( uint i=0; i<rec_num; i++) {
		if( (*(nsaddr_t*)walk) == index_ ) {
			walk += sizeof(nsaddr_t);
			tmp.nd_sendtime = *(Time*)walk;
			walk += sizeof(Time);
			tmp.nd_recvtime = *(Time*)walk;
			walk += sizeof(Time);
			prop_delays[mh->macSA()] = ((NOW-tmp.nd_sendtime) -
				(cmh->timestamp()-tmp.nd_recvtime-cmh->txtime()))/2.0;
			break;
		}
		walk += sizeof(nsaddr_t);
		walk += sizeof(Time)*2;
	}
}



void OTMAN::startHandShake()
{
	/*if( !PktWH_.IsEmpty() ) {*/
		Packet* pkt = makeMultiRev();
		if( pkt == NULL )
			return;

		CtrlPktQ_.insert(pkt, RevQ_.getValidStartTime(HDR_CMN(pkt)->txtime()) );
		//PreSendPkt(pkt, Random::uniform()*OTMAN_REV_DELAY);	
	/*}*/
}

//int OTMAN::round2Slot(Time time)
//{
//	return int( (time-NOW)/TimeSlotLen_ );
//}
//
//Time OTMAN::Slot2Time(int SlotNum, Time BaseTime)
//{
//	return BaseTime+SlotNum*TimeSlotLen_;
//}
//
//Time OTMAN::round2RecverSlotBegin(Time time, nsaddr_t recver)
//{
//	//return int( (time-NOW)
//	int SlotNum = int((time+prop_delays[recver])/TimeSlotLen_);
//	if( time+ prop_delays[recver] - SlotNum*TimeSlotLen_  > 0.00001 )
//		SlotNum++;
//	return SlotNum*TimeSlotLen_ - prop_delays[recver];
//}

Packet* OTMAN::makeMultiRev()
{
	//RevQ_.printRevQueue();
	Packet* pkt = Packet::alloc();
	hdr_otman* hdr_o = hdr_otman::access(pkt);
	hdr_cmn* cmh = HDR_CMN(pkt);

	hdr_o->packet_type = MULTI_REV;	
	
    cmh->next_hop() = MAC_BROADCAST; 
    cmh->direction()=hdr_cmn::DOWN; 
    cmh->addr_type()=NS_AF_ILINK;
    cmh->ptype() = PT_OTMAN;

	hdr_mac* mh=hdr_mac::access(pkt);
	mh->macDA() = MAC_BROADCAST;
	mh->macSA() = index_;

	uint rev_num = 0;
	map<nsaddr_t, PktList>::iterator pos;
	for( pos = PktWH_.Queues.begin();
			pos != PktWH_.Queues.end(); pos++ )
	{
		if( pos->second.head != NULL ) {
			rev_num ++;		
		}
	}

	if( rev_num == 0 ) {
		Packet::free(pkt);
		return NULL;
	}
	/*
	 * The format of entry in multi-rev is
	 *		nsaddr_t requestor; 
	 *		Time	PktLen;
	 *		int		MajorRevID;
	 *		Time	MajorStartTime;
	 *		int		BackupRevID;
	 *		Time	BackupStartTime;
	 */
	//fill the neighbors to which this node already knows the delay
	uint ithneighbor = neighbor_id%rev_num;
	if( !isParallel ) {
		rev_num = 1;
	}

	pkt->allocdata( sizeof(uint)+ sizeof(Time)+
					(sizeof(nsaddr_t)+sizeof(Time)*3+sizeof(int)*2)*rev_num);
	unsigned char* walk = (unsigned char*)pkt->accessdata();
    *(uint*)walk = rev_num;
    walk += sizeof(uint);
	*(Time*)walk = NOW;   //record the time when filling the packet
	walk += sizeof(Time);

	
	PktElem* tmp;
	uint i=0;
	for( pos = PktWH_.Queues.begin();
			pos != PktWH_.Queues.end(); pos++ )
	{
		if( (!isParallel) && (i<ithneighbor) ) {
			i++;
			continue;
		}

		if( pos->second.head != NULL ) {

			*((nsaddr_t*)walk) = pos->first;
			walk += sizeof(nsaddr_t);
			tmp = pos->second.head;

			Packet* TmpPkt = pos->second.head->pkt_->copy();

			Time MajorInterval, BackupInterval;  //both refer to the start time of the interval
			Time PktLen_ = 
				HDR_CMN(TmpPkt)->txtime() +GuardTime; //guardtime is used to avoid collision

			//Time LowerInterval = MajorIntervalLB;
			//Time UpperInterval = MajorIntervalUB;
			//get major interval 
			//Time SinceTime= round2RecverSlotBegin(NOW, pos->first);
			MajorInterval = RevQ_.getValidStartTime(PktLen_, NOW+MajorIntervalLB);
			/*do{
				counter++;
				if( counter == 100 ) {
					UpperInterval += IntervalStep;
					counter = 0;
				}
				MajorInterval =	Random::uniform(LowerInterval,
												UpperInterval);
			}while(!RevQ_.checkAvailability(NOW+MajorInterval,
				NOW+MajorInterval+PktLen_, PRE_REV) );*/


			//get backup send interval
			BackupInterval = RevQ_.getValidStartTime(PktLen_, NOW+MajorInterval + PktLen_);
			/*Time tmpInterval = MajorBackupInterval;
			do{
				counter++;
				if( counter == 100 ) {
					tmpInterval += IntervalStep;
					counter = 0;
				}
				BackupInterval = MajorInterval+PktLen_ + 
								 Random::uniform(tmpInterval);
			}while( !RevQ_.checkAvailability(NOW+BackupInterval, 
								NOW+BackupInterval+PktLen_, PRE_REV) );*/

			RevID++;
			RevQ_.push(RevID, NOW+MajorInterval, NOW+MajorInterval+PktLen_,
						index_, PRE_REV, TmpPkt->copy());
			
			*(Time*)walk = PktLen_;   //already includes GuardTime
			walk += sizeof(Time);
			*(int*)walk = RevID;
			walk += sizeof(int);
			*(Time*)walk = MajorInterval;
			walk += sizeof(Time);

			RevID++;
			RevQ_.push(RevID, NOW+BackupInterval, 
				NOW+BackupInterval+PktLen_, index_, PRE_REV, TmpPkt);
			*(int*)walk = RevID;
			walk += sizeof(int);
			*(Time*)walk = BackupInterval;   //this is the backup time slot. One of the 10 slots after major slot
			walk += sizeof(Time); 


			//remove the packet from PacketWH_ and insert it into 
			//AckWaitingList
			pos->second.head = tmp->next_;
			insertAckWaitingList(tmp->pkt_->copy(), AckTimeOut); 
			PktWH_.deletePkt(pos->first, HDR_CMN(tmp->pkt_)->uid());

			if( !isParallel ) {
				break;
			}
		}
	}

	//node id use 8 bits, first time slot use 10bits, backup time slot use 4 bits 
	
	cmh->size() = (4+(8+8+8+8))*rev_num/8;

	neighbor_id = (neighbor_id+1)%prop_delays.size();
	//RevQ_.printRevQueue();
	return pkt;
}


void OTMAN::processMultiRev(Packet* pkt)
{
	//RevQ_.printRevQueue();
	//TRANSMIT THE TIME IN AVOIDING ITEM TO MY OWN TIME VIEW
	hdr_mac* mh=hdr_mac::access(pkt);
	hdr_cmn* cmh = HDR_CMN(pkt);

	unsigned char* walk = (unsigned char*)pkt->accessdata();
    uint rev_num = *(uint*)walk;
    walk += sizeof(uint);

	Time delta_time = cmh->timestamp() - (*(Time*)walk);
	//reservation time = majorinterval -delta_time +NOW
	walk += sizeof(Time);

	/*store the rev requests in Pending Revs and 
	 *process it when RevAckAccumTimer expires
	 */
	
	Time PktLen_;
	for(uint i=0; i<rev_num; i++) {
		if( *((nsaddr_t*)walk) == index_ ) {
			walk += sizeof(nsaddr_t);

			if( RevAckAccumTimer.status() == TIMER_IDLE ) {
				RevAckAccumTimer.resched(RevAckAccumTime_);
			}

			RevReq* tmp = new RevReq();
			tmp->requestor = mh->macSA();
			//tmp->Sincetime = map2OwnTime(cmh->ts_, mh->macSA());
				
			PktLen_ = *((Time*)walk);
			walk += sizeof(Time);
			tmp->AcceptedRevID = *((int*)walk);
			walk += sizeof(int);

			//covert the time based on this node's timeline
			tmp->StartTime = *((Time*)walk)-delta_time + NOW;   
			walk += sizeof(Time);
			tmp->EndTime = tmp->StartTime + PktLen_;

			//check major slot
			if( RevQ_.checkAvailability(tmp->StartTime,tmp->EndTime, RECVING) ) {
				tmp->RejectedRevID = *((int*)walk);
				walk += sizeof(int)+sizeof(Time);
			
				RevQ_.push(RevID, tmp->StartTime, tmp->EndTime, 
					tmp->requestor, RECVING);
			}
			else{
				tmp->RejectedRevID = tmp->AcceptedRevID;
				tmp->AcceptedRevID = *((int*)walk);
				walk += sizeof(int);

				//covert the time based on this node's timeline
				tmp->StartTime = *((Time*)walk)-delta_time + NOW;
				walk += sizeof(Time);			
				tmp->EndTime = tmp->StartTime + PktLen_;

				if( RevQ_.checkAvailability(tmp->StartTime, tmp->EndTime, RECVING) ) {

					RevQ_.push(RevID, tmp->StartTime, tmp->EndTime, 
						tmp->requestor, RECVING);
				}
				else {
					//give a wrong rev time interval, so the requestor will know both are wrong
					tmp->StartTime = -1.0;
					tmp->EndTime = -1.0;
				}
				
			}
			PendingRevs.push_back(tmp);

		}
		else{
			//this entry has nothing to do with this node. skip it
			walk += sizeof(nsaddr_t);
			walk += 3*sizeof(Time);
			walk += 2*sizeof(int);
		}
	}
	//RevQ_.printRevQueue();
}


/*
 * MultiRevAck format
 *	nsaddr_t	requestor;
 *	Time		StartTime;  //related to current time
 *	Time		EndTime;
 *	int			AcceptedRevID;
 *	int			RejectedRevID;
 */
Packet* OTMAN::makeMultiRevAck()
{
	//overhear neighbors rev
	Packet* pkt = Packet::alloc();
	hdr_otman* hdr_o = hdr_otman::access(pkt);
	hdr_cmn* cmh = HDR_CMN(pkt);

	hdr_o->packet_type = MULTI_REV_ACK;
	
	
    cmh->next_hop() = MAC_BROADCAST; 
    cmh->direction()=hdr_cmn::DOWN; 
    cmh->addr_type()=NS_AF_ILINK;
    cmh->ptype() = PT_OTMAN;

	hdr_mac* mh=hdr_mac::access(pkt);
	mh->macDA() = MAC_BROADCAST;
	mh->macSA() = index_;

	pkt->allocdata( sizeof(uint)+ sizeof(Time)+
				(sizeof(nsaddr_t)+2*sizeof(int)+2*sizeof(Time))*PendingRevs.size() );
	unsigned char* walk = (unsigned char*)pkt->accessdata();
    *(uint*)walk = PendingRevs.size();
    walk += sizeof(uint);
	*(Time*)walk = NOW;
	walk += sizeof(Time);

	for( vector<RevReq*>::iterator pos=PendingRevs.begin();
         pos != PendingRevs.end(); pos++)
    {
		*(nsaddr_t*)walk = (*pos)->requestor;  //ack to whom
        walk += sizeof(nsaddr_t);
		*(Time*)walk = (*pos)->StartTime - NOW;
		walk += sizeof(Time);
		*(Time*)walk = (*pos)->EndTime - NOW;
		walk += sizeof(Time);
		*(int*)walk = (*pos)->AcceptedRevID;
		walk += sizeof(int);		
		*(int*)walk = (*pos)->RejectedRevID;
		walk += sizeof(int);
    }
	
	//schedule the rev req.
	//node id use 10 bits, first time slot use 10bits, backup time slot use 4 bits 
	//hdr_o->size() = (PendingRevs.size()*(10+10+6+4))/8;
	cmh->size() = (PendingRevs.size()*(8+10+6+4))/8;

	return pkt;
}

void OTMAN::processMultiRevAck(Packet *pkt)
{
	//RevQ_.printRevQueue();
	//overhear neighbors revack
	//change the pre_rev to SENDING, and delete the other slot rev
	//start the the timer in the rev_elem
	//hdr_otman* hdr_o = hdr_otman::access(pkt);
	hdr_mac* mh=hdr_mac::access(pkt);
	hdr_cmn* cmh = HDR_CMN(pkt);

	unsigned char* walk = (unsigned char*)pkt->accessdata();
    uint rev_num = *(uint*)walk;
    walk += sizeof(uint);
	Time delta_time = cmh->timestamp() - (*(Time*)walk);
	walk += sizeof(Time);


	RevReq* tmp = new RevReq();
	for(uint i=0; i<rev_num; i++) {

		tmp->requestor = *(nsaddr_t*)walk;
		walk += sizeof(nsaddr_t);
		tmp->StartTime = *(Time*)walk;
		walk += sizeof(Time);
		tmp->EndTime = *(Time*)walk;
		walk += sizeof(Time);
		tmp->AcceptedRevID = *(int*)walk;
		walk += sizeof(int);		
		tmp->RejectedRevID = *(int*)walk;
		walk += sizeof(int);

		if( tmp->StartTime > 0 ) {
			if( tmp->requestor == index_ ) {
				//start timer in updateStatus
				RevQ_.updateStatus(tmp->AcceptedRevID, SENDING);  
				RevQ_.deleteRev(tmp->RejectedRevID);
			}
			else {
				RevQ_.push(tmp->AcceptedRevID, NOW+tmp->StartTime -delta_time - prop_delays[mh->macSA()],
							NOW+tmp->EndTime -delta_time - prop_delays[mh->macSA()],
							mh->macSA(), AVOIDING);
			}
		}
		else if( tmp->requestor == index_ ) {
			RevQ_.deleteRev(tmp->AcceptedRevID);
			RevQ_.deleteRev(tmp->RejectedRevID);
		}
		
	}
	delete tmp;
	//RevQ_.printRevQueue();

}

void OTMAN::RecordDataPkt(Packet* pkt)
{
	hdr_cmn* cmh = HDR_CMN(pkt);
	hdr_mac* mh=hdr_mac::access(pkt);

	DataAck* tmp = new DataAck;
	tmp->Sender = mh->macSA();
	tmp->SeqNum = cmh->uid();
	PendingDataAcks.push_back(tmp);

	SucDataNum[mh->macSA()]++;
	// startAckTimer
	if( DataAckAccumTimer.status() == TIMER_IDLE ) {
		DataAckAccumTimer.resched(DataAckAccumTime_);
	}
}


Packet* OTMAN::makeDataAck()
{
	Packet* pkt = Packet::alloc();
	hdr_otman* hdr_o = hdr_otman::access(pkt);
	hdr_cmn* cmh = HDR_CMN(pkt);

	hdr_o->packet_type = MULTI_DATA_ACK;
	
	
    cmh->next_hop() = MAC_BROADCAST; 
    cmh->direction()=hdr_cmn::DOWN; 
    cmh->addr_type()=NS_AF_ILINK;
    cmh->ptype() = PT_OTMAN;

	hdr_mac* mh=hdr_mac::access(pkt);
	mh->macDA() = MAC_BROADCAST;
	mh->macSA() = index_;

	uint DataAckNum = PendingDataAcks.size();

	//hdr_o->size() = DataAckNum*((10+10)/8);
	cmh->size() = DataAckNum*((10+10)/8);

	pkt->allocdata(sizeof(uint)+DataAckNum*(sizeof(nsaddr_t)+sizeof(int)));
	unsigned char* walk = (unsigned char*)pkt->accessdata();
	*(uint*)walk = DataAckNum;
    walk += sizeof(uint);


	for(vector<DataAck*>::iterator pos = PendingDataAcks.begin(); 
		pos != PendingDataAcks.end(); pos++ )
	{
		*(nsaddr_t*)walk = (*pos)->Sender;
		walk += sizeof(nsaddr_t);
		*(int*)walk = (*pos)->SeqNum;
		walk += sizeof(int);
	}

	return pkt;
}


void OTMAN::processDataAck(Packet* pkt)
{
	unsigned char* walk = (unsigned char*)pkt->accessdata();
    uint AckNum = *(uint*)walk;
    walk += sizeof(uint);

	/*store the rev requests in Pending Revs and 
	 *process it when RevAckAccumTimer expires
	 */
	int pkt_id;
	for(uint i=0; i<AckNum; i++) {
		if( *((nsaddr_t*)walk) == index_ ) {
			walk += sizeof(nsaddr_t);
			/*nsaddr_t recver = hdr_mac::access(pkt)->macSA();
			if( PktWH_.deletePkt(recver, *(int*)walk) ) {
				if( SucDataNum.count(recver) == 0 )
					SucDataNum[recver] = 1;
				else
					SucDataNum[recver]++;
			}

			walk += sizeof(int);	*/
			pkt_id = (*(int*)walk);
			if( AckWaitingList.count(pkt_id) != 0 ) {
				if( AckWaitingList[pkt_id].status() == TIMER_PENDING ) {
					AckWaitingList[pkt_id].cancel();
				}
				if( AckWaitingList[pkt_id].pkt_!= NULL ) {
					Packet::free(AckWaitingList[pkt_id].pkt_);
					AckWaitingList[pkt_id].pkt_ = NULL;		
				}
			}
			walk += sizeof(int);
		}
		else{
			walk += sizeof(nsaddr_t);
			walk += sizeof(int);
		}
	}
}


void OTMAN::CallbackProcess(Event* e)
{
	if( callback_ != NULL )
		callback_->handle(e);
}


void OTMAN::StatusProcess(Event *e)
{
	//perhaps not right
	UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
	if( SEND == n->TransmissionStatus() ){
		n->SetTransmissionStatus(IDLE);
	}
  	return;
}



Time OTMAN::map2OwnTime(Time SenderTime, nsaddr_t Sender)
{
	return SenderTime+prop_delays[Sender];
}





void OTMAN::printResult()
{
	int totalPkt = 0;
	map<nsaddr_t, int>::iterator pos = SucDataNum.begin();
	for(; pos != SucDataNum.end(); pos++) {
		totalPkt += pos->second;
	}
	printf("node %d receive %d packets\n", index_, totalPkt);
}



void OTMAN::clearAckWaitingList()
{
	
	set<int> DelList;
	for(map<int, AckWaitTimer>::iterator pos = AckWaitingList.begin();
			pos!=AckWaitingList.end(); pos++)				 {
		if( pos->second.pkt_ == NULL )
			DelList.insert(pos->first);
	}

	for(set<int>::iterator pos = DelList.begin();
		pos!=DelList.end(); pos++)    {
			AckWaitingList.erase(*pos);
	}
}

void OTMAN::insertAckWaitingList(Packet* p, Time delay)
{
	clearAckWaitingList();
	int uid_ = HDR_CMN(p)->uid();
	AckWaitingList[uid_].mac_ = this;
	AckWaitingList[uid_].pkt_ = p;
	AckWaitingList[uid_].resched(delay);
}

//
//void CtrlPktTimer::expire(Event *e)
//{
//	mac_->sendDown(pkt_);
//}
void AckWaitTimer::expire(Event *e)
{
	//int pkt_id = HDR_CMN(pkt_)->uid();
	mac_->PktWH_.insert2PktQs(pkt_);
	pkt_ = NULL;
}


void CtrlPktTimer::expire(Event *e)
{
	mac_->SendPkt(pkt_);
	pkt_ = NULL;
}


void ctrlpktqueue::insert(Packet *ctrl_p, Time delay)
{
	CtrlPktTimer* tmp = new CtrlPktTimer(mac_);
	tmp->pkt_ = ctrl_p;
	tmp->resched(delay);
	CtrlQ_.push(tmp);
}

void ctrlpktqueue::clearExpiredElem()
{
	CtrlPktTimer* tmp = NULL;
	while( (CtrlQ_.size() >0) && ((CtrlQ_.front())->pkt_ == NULL) ) {
		tmp = CtrlQ_.front();
		CtrlQ_.pop();
		delete tmp;
	}
}

void OTMAN::printDelayTable()
{
	FILE* stream = fopen("distance", "a");

	map<nsaddr_t, Time>::iterator pos=prop_delays.begin();
	for(;pos != prop_delays.end(); pos++) {
		fprintf(stream, "%f\n", pos->second*1500);
	}

	fclose(stream);
}

void PrintDistanceTimer::expire(Event *e)
{
	mac_->printDelayTable();
}




