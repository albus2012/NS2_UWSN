#include "sfama.h"



int hdr_SFAMA::offset_;
static class SFAMA_HeaderClass: public PacketHeaderClass{
public:
	SFAMA_HeaderClass():PacketHeaderClass("PacketHeader/SFAMA",sizeof(hdr_SFAMA))
	{
		bind_offset(&hdr_SFAMA::offset_);
	}
}class_SFAMA_hdr;



static class SFAMAClass : public TclClass {
public:
	SFAMAClass():TclClass("Mac/UnderwaterMac/SFAMA") {}
	TclObject* create(int, const char*const*) {
		return (new SFAMA());
	}
}class_SFAMA;


/*expire functions and handle functions*/
void SFAMA_Wait_Send_Timer::expire(Event *e)
{
	mac_->WaitSendTimerProcess(pkt_);
	pkt_ = NULL;  /*reset pkt_*/
}


void SFAMA_Wait_Reply_Timer::expire(Event* e)
{
	mac_->WaitReplyTimerProcess();
}


void SFAMA_Backoff_Timer::expire(Event* e)
{
	mac_->BackoffTimerProcess();
}


void SFAMA_CallBackHandler::handle(Event* e)
{
	mac_->CallBackProcess(e);
}

void SFAMA_MAC_StatusHandler::handle(Event* e)
{
	mac_->StatusProcess(slotnum());
}

void SFAMA_DataSend_Timer::expire(Event *e)
{
	mac_->DataSendTimerProcess();
}

void SFAMA_SlotInitHandler::handle(Event* e)
{
	mac_->initSlotLen();
}


SFAMA::SFAMA():UnderwaterMac(), status_(IDLE_WAIT), guard_time_(0.00001), slot_len_(0.0),
	is_in_round(false), is_in_backoff(false), max_backoff_slots_(4), max_burst_(1),
	data_sending_interval_(0.0000001), callback_handler(this), status_handler(this), 
	slotinit_handler(this), wait_send_timer(this), 	wait_reply_timer(this), 
	backoff_timer(this), datasend_timer(this)
{
	bind("guard_time_", &guard_time_);
	bind("max_backoff_slots_", &max_backoff_slots_);	
	bind("max_burst_", &max_burst_);
	
	Random::seed_heuristically();

	Scheduler::instance().schedule(&slotinit_handler, 
		&slotinit_event, 0.05 /*callback delay*/);
}


int SFAMA::command(int argc, const char *const *argv)
{
	return UnderwaterMac::command(argc, argv);
}


void SFAMA::RecvProcess(Packet *p)
{
	hdr_SFAMA* SFAMAh = hdr_SFAMA::access(p);

#ifdef SFAMA_DEBUG
			printf("Time: %f:node %d, %f: node %d recv from node %d\n", 
				   NOW, index_, NOW, HDR_MAC(p)->macDA(), HDR_MAC(p)->macSA());
#endif
	
		switch( SFAMAh->packet_type ) {
			case hdr_SFAMA::SFAMA_RTS:
				processRTS(p);
				break;
			case hdr_SFAMA::SFAMA_CTS:
				processCTS(p);
				break;
			case hdr_SFAMA::SFAMA_DATA:
				processDATA(p);
				break;
			case hdr_SFAMA::SFAMA_ACK:
				processACK(p);
				break;
			default:
				/*unknown packet type. error happens*/
				printf("unknown packet type in SFAMA::RecvProcess");

				break;
		}
	
	

	Packet::free(p);
}


void SFAMA::TxProcess(Packet *p)
{
	//hdr_cmn* cmh = hdr_cmn::access(p);
	//hdr_SFAMA* SFAMAh = hdr_SFAMA::access(p);

	Scheduler::instance().schedule(&callback_handler, 
		&callback_event, 0.0001 /*callback delay*/);
	
	fillDATA(p);
/*
	SFAMAh->SA = index_;
	SFAMAh->DA = cmh->next_hop();

	cmh->error() = 0;
	cmh->size() += hdr_SFAMA::getSize(hdr_SFAMA::SFAMA_DATA);
	cmh->txtime() = getTxtimeByPktSize(cmh->size());
*/

#ifdef SFAMA_DEBUG
printf("TxProcess(before)\n");
printAllQ();
#endif
	CachedPktQ_.push(p);
#ifdef SFAMA_DEBUG
printf("TxProcess(after)\n");
printAllQ();
#endif
	if( CachedPktQ_.size() == 1 && getStatus() == IDLE_WAIT ) {
		prepareSendingDATA();
	}
}


void SFAMA::initSlotLen()
{
	slot_len_ = guard_time_ + 
		getTxTime(hdr_SFAMA::getSize(hdr_SFAMA::SFAMA_CTS)) +
		UnderwaterChannel::Transmit_distance()/1500.0;
}

/*
Time SFAMA::getTxtimeByPktSize(int pkt_size)
{
	return pkt_size*8*encoding_efficiency_/bit_rate_;
}
*/


Time SFAMA::getTime2ComingSlot(Time t)
{
	int numElapseSlot = int(t/slot_len_);

	return slot_len_*(1+numElapseSlot)-t;	
}


Packet* SFAMA::makeRTS(nsaddr_t recver, int slot_num)
{
	Packet* rts_pkt = Packet::alloc();
	hdr_cmn* cmh = HDR_CMN(rts_pkt);
	hdr_mac* mach = HDR_MAC(rts_pkt);
	hdr_SFAMA* SFAMAh= hdr_SFAMA::access(rts_pkt);

	cmh->size() = hdr_SFAMA::getSize(hdr_SFAMA::SFAMA_RTS);
	cmh->txtime() = getTxTime(cmh->size());
	cmh->error() = 0;
	cmh->direction() = hdr_cmn::DOWN;
	cmh->ptype() = PT_SFAMA;

	mach->macSA() = index_;
	mach->macDA() = recver;
	
	SFAMAh->packet_type = hdr_SFAMA::SFAMA_RTS;
	//SFAMAh->SA = index_;
	//SFAMAh->DA = recver;
	SFAMAh->SlotNum = slot_num;

	rts_pkt->next_ = NULL;

	return rts_pkt;
}


Packet* SFAMA::makeCTS(nsaddr_t rts_sender, int slot_num)
{
	Packet* cts_pkt = Packet::alloc();
	hdr_cmn* cmh = HDR_CMN(cts_pkt);
	hdr_mac* mach = HDR_MAC(cts_pkt);
	hdr_SFAMA* SFAMAh= hdr_SFAMA::access(cts_pkt);

	cmh->size() = hdr_SFAMA::getSize(hdr_SFAMA::SFAMA_CTS);
	cmh->txtime() = getTxTime(cmh->size());
	cmh->error() = 0;
	cmh->direction() = hdr_cmn::DOWN;
	cmh->ptype() = PT_SFAMA;

	mach->macSA() = index_;
	mach->macDA() = rts_sender;
	
	SFAMAh->packet_type = hdr_SFAMA::SFAMA_CTS;
	//SFAMAh->SA = index_;
	//SFAMAh->DA = rts_sender;
	SFAMAh->SlotNum = slot_num;

	cts_pkt->next_ = NULL;

	return cts_pkt;
}


Packet* SFAMA::fillDATA(Packet *data_pkt)
{
	hdr_cmn* cmh = HDR_CMN(data_pkt);
	hdr_mac* mach = HDR_MAC(data_pkt);
	hdr_SFAMA* SFAMAh = hdr_SFAMA::access(data_pkt);

	cmh->size() += hdr_SFAMA::getSize(hdr_SFAMA::SFAMA_DATA);
	cmh->txtime() = getTxTime(cmh->size());
	cmh->error() = 0;
	cmh->direction() = hdr_cmn::DOWN;
	
	mach->macSA() = index_;
	mach->macDA() = cmh->next_hop();

	SFAMAh->packet_type = hdr_SFAMA::SFAMA_DATA;
	//SFAMAh->SA = index_;
	//SFAMAh->DA = cmh->next_hop();

	return data_pkt;
}


Packet* SFAMA::makeACK(nsaddr_t data_sender)
{
	Packet* ack_pkt = Packet::alloc();
	hdr_cmn* cmh = HDR_CMN(ack_pkt);
	hdr_mac* mach = HDR_MAC(ack_pkt);

	hdr_SFAMA* SFAMAh= hdr_SFAMA::access(ack_pkt);

	cmh->size() = hdr_SFAMA::getSize(hdr_SFAMA::SFAMA_ACK);
	cmh->txtime() = getTxTime(cmh->size());
	cmh->error() = 0;
	cmh->direction() = hdr_cmn::DOWN;
	cmh->ptype() = PT_SFAMA;

	mach->macSA() = index_;
	mach->macDA() = data_sender;

	SFAMAh->packet_type = hdr_SFAMA::SFAMA_ACK;
	//SFAMAh->SA = index_;
	//SFAMAh->DA = data_sender;
	
	return ack_pkt;
}


/*process all kinds of packets*/

void SFAMA::processRTS(Packet *rts_pkt)
{
	hdr_SFAMA* SFAMAh = hdr_SFAMA::access(rts_pkt);
	hdr_mac* mach = HDR_MAC(rts_pkt);

	Time time2comingslot = getTime2ComingSlot(NOW);

	if( mach->macDA() == index_ ) {
		if ( (getStatus() == IDLE_WAIT ||
			getStatus() == WAIT_SEND_RTS ||
			getStatus() == BACKOFF_FAIR )   		) {

				stopTimers();		
				setStatus(WAIT_SEND_CTS);
				//reply a cts
				wait_send_timer.pkt_ = makeCTS(mach->macSA(), SFAMAh->SlotNum);
				wait_send_timer.resched(time2comingslot);
		}
	}
	else {
		//do backoff
		Time backoff_time = time2comingslot + 1 /*for cts*/+
			SFAMAh->SlotNum*slot_len_ /*for data*/+ 1 /*for ack*/;

		stopTimers();		
		setStatus(BACKOFF);

		backoff_timer.resched(backoff_time);
		
	}
}

void SFAMA::processCTS(Packet *cts_pkt)
{
	hdr_SFAMA* SFAMAh = hdr_SFAMA::access(cts_pkt);
	hdr_mac* mach = HDR_MAC(cts_pkt);
	Time time2comingslot = getTime2ComingSlot(NOW);

	if( mach->macDA() == index_ && getStatus() == WAIT_RECV_CTS ) {

		//send DATA
		stopTimers();		
		setStatus(WAIT_SEND_DATA);
		//send the packet
		wait_send_timer.pkt_ = NULL;
		wait_send_timer.resched(time2comingslot);
		
		Time wait_time = (1+SFAMAh->SlotNum)*slot_len_+time2comingslot;
		if( time2comingslot < 0.1 ) {
			wait_time += slot_len_;
		}
		
		wait_reply_timer.resched(wait_time);
	}
	else {
		//do backoff
		Time backoff_time = SFAMAh->SlotNum*slot_len_ /*for data*/+ 
			1 /*for ack*/+time2comingslot;

		stopTimers();		
		setStatus(BACKOFF);

		backoff_timer.resched(backoff_time);
	}

}

void SFAMA::processDATA(Packet* data_pkt)
{
	//hdr_SFAMA* SFAMAh = hdr_SFAMA::access(data_pkt);
	hdr_mac* mach = HDR_MAC(data_pkt);

	if( mach->macDA() == index_ && getStatus() == WAIT_RECV_DATA ) {
		//send ACK
		stopTimers();
		setStatus(WAIT_SEND_ACK);

		wait_send_timer.pkt_ = makeACK(mach->macSA());
		wait_send_timer.resched(getTime2ComingSlot(NOW));

		/*send packet to upper layer*/		
		hdr_cmn::access(data_pkt)->size() -= hdr_SFAMA::getSize(hdr_SFAMA::SFAMA_DATA);
		sendUp(data_pkt->copy()); /*the original one will be released*/
	}
	else {
		//do backoff
		Time backoff_time = 1+getTime2ComingSlot(NOW) /*for ack*/;

		stopTimers();		
		setStatus(BACKOFF);

		backoff_timer.resched(backoff_time);
	}
}


void SFAMA::processACK(Packet* ack_pkt)
{
	//hdr_SFAMA* SFAMAh = hdr_SFAMA::access(ack_pkt);
	hdr_mac* mach = HDR_MAC(ack_pkt);
#ifdef SFAMA_DEBUG
	printf("processACK(before) \n");
	printAllQ();
	printf("Status is %d", getStatus());
#endif
	if( mach->macDA() == index_ && getStatus() == WAIT_RECV_ACK ) {
		stopTimers();
		setStatus(IDLE_WAIT);

		//release data packets have been sent successfully
		releaseSentPkts();

		//start to prepare for sending next DATA packet
		prepareSendingDATA();
	}
	/*
	 * consider the multi-hop case, cannot stop timers here
	else {
		stopTimers();
		setStatus(IDLE_WAIT);

		//release data packets have been sent successfully
		prepareSendingDATA();	
	}
	*/
#ifdef SFAMA_DEBUG
	printf("processACK(after) \n");
	printAllQ();
#endif
}



void SFAMA::stopTimers()
{
	wait_send_timer.stop();
	wait_reply_timer.stop();
	backoff_timer.stop();
}



void SFAMA::releaseSentPkts()
{
	Packet* tmp = NULL;
	
	while( !SendingPktQ_.empty() ) {
		tmp = SendingPktQ_.front();
		SendingPktQ_.pop();
		Packet::free(tmp);	
	}
}


void SFAMA::prepareSendingDATA()
{
	queue<Packet*> tmpQ_;
	Packet* tmp_pkt;
	nsaddr_t recver_addr;
	int pkt_num = 1;

	if( SendingPktQ_.empty() && CachedPktQ_.empty() ) {
		return;
	}

	if( !SendingPktQ_.empty() && getStatus() == IDLE_WAIT  ) {
		recver_addr = HDR_MAC(SendingPktQ_.front())->macDA();

	} else if( !CachedPktQ_.empty() && getStatus() == IDLE_WAIT ) {
#ifdef SFAMA_DEBUG
	printf("prepareSendingDATA(before) \n");
	printAllQ();
#endif
		tmp_pkt = CachedPktQ_.front();
		recver_addr = HDR_MAC(tmp_pkt)->macDA();
		CachedPktQ_.pop();
		SendingPktQ_.push(tmp_pkt);
		pkt_num = 1;

		/*get at most max_burst_ DATA packets with same receiver*/
		while( (pkt_num < max_burst_) && (!CachedPktQ_.empty()) ) {
			tmp_pkt = CachedPktQ_.front();
			CachedPktQ_.pop();

			if( recver_addr == HDR_MAC(tmp_pkt)->macDA() ) {
				SendingPktQ_.push(tmp_pkt);
				pkt_num ++;
			}
			else {
				tmpQ_.push(tmp_pkt);
			}
		
		}

		//make sure the rest packets are stored in the original order
		while( !CachedPktQ_.empty() ) {
			tmpQ_.push(CachedPktQ_.front());
			CachedPktQ_.pop();		
		}

		while( !tmpQ_.empty() ) {
			CachedPktQ_.push(tmpQ_.front());
			tmpQ_.pop();
		}
		
#ifdef SFAMA_DEBUG
	printf("prepareSendingDATA(after) \n");
	printAllQ();
#endif
	}
	

	Time additional_txtime = getPktTrainTxTime()-
			getTxTime(hdr_SFAMA::getSize(hdr_SFAMA::SFAMA_CTS));
		

	scheduleRTS(recver_addr, int(additional_txtime/slot_len_)+1 /*for ceil*/
	+1/*the basic slot*/ );
}


Time SFAMA::getPktTrainTxTime()
{
	Time txtime = 0.0;

	int q_size = SendingPktQ_.size();
#ifdef SFAMA_DEBUG
	printf("getPktTrainTxTime(before) \n");
	printAllQ();
#endif
	for(int i=0; i<q_size; i++ ) {
		txtime += hdr_cmn::access(SendingPktQ_.front())->txtime();
		SendingPktQ_.push(SendingPktQ_.front());
		SendingPktQ_.pop();
	}
#ifdef SFAMA_DEBUG
	printf("getPktTrainTxTime(after) \n");
	printAllQ();
#endif
	
	txtime += (q_size-1)*data_sending_interval_;

	return txtime;
}



void SFAMA::scheduleRTS(nsaddr_t recver, int slot_num)
{
	Time backoff_time = randBackoffSlots()*slot_len_+getTime2ComingSlot(NOW);
	setStatus(WAIT_SEND_RTS);
	wait_send_timer.pkt_ = makeRTS(recver, slot_num);
	wait_send_timer.resched(backoff_time);
}



int SFAMA::randBackoffSlots()
{
	return Random::integer(max_backoff_slots_);
}


void SFAMA::sendPkt(Packet* pkt)
{
	hdr_cmn* cmh=HDR_CMN(pkt);
	hdr_SFAMA* SFAMAh = hdr_SFAMA::access(pkt);

	cmh->direction() = hdr_cmn::DOWN;

	UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;

	double txtime=cmh->txtime();
	Scheduler& s=Scheduler::instance();

	//status_handler.is_ack() = false;
	if( SFAMAh->packet_type == hdr_SFAMA::SFAMA_CTS ) {
		status_handler.slotnum() = SFAMAh->SlotNum;
	}
	/*else if ( SFAMAh->packet_type == hdr_SFAMA::SFAMA_ACK ) {
		status_handler.is_ack() = true;
	}*/
	
	switch( n->TransmissionStatus() ) {
		case SLEEP:
			Poweron();
		case IDLE:
			n->SetTransmissionStatus(SEND); 
			cmh->timestamp() = NOW;
			#ifdef SFAMA_DEBUG
			printf("%f: node %d send to node %d\n", 
				   NOW, HDR_MAC(pkt)->macSA(), HDR_MAC(pkt)->macDA());
			#endif
			sendDown(pkt);
			s.schedule(&status_handler,&status_event,txtime);
			break;
		case RECV:
			printf("RECV-SEND Collision!!!!!\n");
			Packet::free(pkt);
			//do backoff??
			break;
			
		default:
			//status is SEND, send too fast
			printf("node%d send data too fast\n",index_);
			Packet::free(pkt);
			break;

	}

	return;
}



void SFAMA::CallBackProcess(Event *e)
{
	callback_->handle(e);
}

void SFAMA::StatusProcess(int slotnum)
{
	UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
	n->SetTransmissionStatus(IDLE);
	
	
	switch(getStatus()) {
	  case WAIT_SEND_RTS:
		slotnum = 1;
		setStatus(WAIT_RECV_CTS);
		break;
	  case WAIT_SEND_CTS:
		//slotnum += 1;
		setStatus(WAIT_RECV_DATA);
		break;
	  case WAIT_SEND_DATA:
		//cannot reach here
		slotnum = 1;
		setStatus(WAIT_RECV_ACK);
		//wait_reply time has been scheduled.
		return;
	  case WAIT_SEND_ACK:
		WaitReplyTimerProcess(true); //go to next round
		return;
	  default:
		#ifdef SFAMA_DEBUG
		switch (getStatus() ) {
		  case IDLE_WAIT:
		  printf("node %d: status error: %s!\n", index_,"IDLE_WAIT");
		  break;
		  case WAIT_RECV_CTS:
			printf("node %d: status error: %s!\n", index_,"WAIT_RECV_CTS");
			break;
		  case WAIT_RECV_DATA:
			printf("node %d: status error: %s!\n", index_,"WAIT_RECV_DATA");
			break;
		  case BACKOFF:
			printf("node %d: status error: %s!\n", index_,"BACKOFF");
			break;
		}
		#endif
		break;
	  
	} 
	
// 	Time time2comingslot = getTime2ComingSlot(NOW);
// 	if ( time2comingslot <  0.1 ) {
// 	  slotnum++; 
// 	}
	
	//if( ! status_handler.is_ack() ) {
		wait_reply_timer.resched(slot_len_*slotnum+getTime2ComingSlot(NOW));
	//}
	/*else {
	  status_handler.is_ack() = false;
	}*/
	
	return;
}

void SFAMA::BackoffTimerProcess()
{
	setStatus(IDLE_WAIT);
	prepareSendingDATA();
}

void SFAMA::WaitSendTimerProcess(Packet *pkt)
{
		
	if( NULL == pkt ) {
		datasend_timer.resched(0.00001);
	}
	else {
		sendPkt(pkt);
	}
}

void SFAMA::WaitReplyTimerProcess(bool directcall)
{
	/*do backoff*/
	Time backoff_time = randBackoffSlots()*slot_len_ + getTime2ComingSlot(NOW);
	#ifdef SFAMA_DEBUG
	if( !directcall ) 
	  printf("%f node %d TIME OUT!!!!!\n", NOW, index_);
	#endif  //SFAMA_DEBUG
	if( directcall ) {
		setStatus(BACKOFF_FAIR);
		backoff_timer.resched(getTime2ComingSlot(NOW));
	} else {
		setStatus(BACKOFF);
		backoff_timer.resched(backoff_time);
	}
	
	
}


void SFAMA::DataSendTimerProcess()
{	
  
#ifdef SFAMA_DEBUG
	printf("DataSendTimerProcess(before) \n");
	printAllQ();
#endif
	if( !SendingPktQ_.empty() ) {
		Packet* pkt = SendingPktQ_.front();
		Time txtime = hdr_cmn::access(pkt)->txtime();
		Backup_SendingPktQ_.push(pkt);

		SendingPktQ_.pop();

		sendDataPkt(pkt->copy());

		datasend_timer.resched(data_sending_interval_+txtime);
	}
	else {
	  while( !Backup_SendingPktQ_.empty() ) {
		//push all packets into SendingPktQ_. After getting ack, release them
		SendingPktQ_.push(Backup_SendingPktQ_.front());
		Backup_SendingPktQ_.pop();
	  }
	  //status_handler.is_ack() = false;
	  Scheduler::instance().schedule(&status_handler,&status_event, 0.0000001);
	  /*
		UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
		n->SetTransmissionStatus(IDLE); 
		wait_reply_timer.resched(slot_len_+getTime2ComingSlot(NOW));
			switch(getStatus()) {
			  case WAIT_SEND_RTS:
				setStatus(WAIT_RECV_CTS);
				break;
			  case WAIT_SEND_CTS:
				setStatus(WAIT_RECV_DATA);
				break;
			  case WAIT_SEND_DATA:
				setStatus(WAIT_RECV_ACK);
				break;
			  default:
				printf("status error!\n");
				break;
			  
			} 
			*/
	}
#ifdef SFAMA_DEBUG
	printf("DataSendTimerProcess(after) \n");
	printAllQ();
#endif
}


void SFAMA::sendDataPkt(Packet* pkt)
{
	hdr_cmn* cmh=HDR_CMN(pkt);
	//hdr_FAMA* FAMAh = hdr_FAMA::access(pkt);

	cmh->direction() = hdr_cmn::DOWN;

	UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
	
	switch( n->TransmissionStatus() ) {
		case SLEEP:
			Poweron();
		case IDLE:
			n->SetTransmissionStatus(SEND); 
			cmh->timestamp() = NOW;
			#ifdef SFAMA_DEBUG
			printf("%f: node %d send to node %d\n", 
				   NOW, HDR_MAC(pkt)->macSA(), HDR_MAC(pkt)->macDA());
			#endif
			sendDown(pkt);
			break;
		case RECV:
			printf("RECV-SEND Collision!!!!!\n");
			Packet::free(pkt);
			//do backoff??
			break;
			
		default:
			//status is SEND, send too fast
			printf("node%d send data too fast\n",index_);
			Packet::free(pkt);
			break;
	}

	return;
}

void SFAMA::setStatus(enum SFAMA_Status status)
{
	if( status == BACKOFF ) {
	  status = status;
	}
    status_ = status;
}

enum SFAMA_Status SFAMA::getStatus()
{
    return status_;
}


#ifdef SFAMA_DEBUG
void SFAMA::printAllQ(){
printf("Time %f node %d . CachedPktQ_:\n", NOW, index_);
printQ(CachedPktQ_);
printf("\nSendingPktQ_:\n");
printQ(SendingPktQ_);
printf("\n");

  
}
void SFAMA::printQ(queue< Packet* >& my_q)
{
	queue<Packet*> tmp_q;
	while(!my_q.empty()) {
		printf("%d\t", HDR_CMN(my_q.front())->uid());
		tmp_q.push(my_q.front());
		my_q.pop();
	}
	
	while(!tmp_q.empty()) {
	  my_q.push(tmp_q.front());
	  tmp_q.pop();
	}
}

#endif
