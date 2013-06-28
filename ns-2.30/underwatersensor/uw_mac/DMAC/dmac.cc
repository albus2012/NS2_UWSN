/*
In this version of DMAC, the major modifications for the 15-6-2013
  version are :

*/


#include "underwatersensor/uw_routing/vectorbasedforward.h"
#include "dmac.h"

#include <stdlib.h>
#include <fstream>
#include <iostream>
using namespace std;

int hdr_dmac::offset_;



static class DMACHeaderClass: public PacketHeaderClass{

 public:
  DMACHeaderClass():PacketHeaderClass("PacketHeader/DMAC",sizeof(hdr_dmac))
  {
    bind_offset(&hdr_dmac::offset_);
  }

} class_dmac_hdr;



void ScheQueue::push(Time SendTime,
		                 nsaddr_t node_id,
		                 Time Interval)
{
	ScheTime* newElem = new ScheTime(SendTime, node_id, mac_);
	newElem->start(Interval);

	ScheTime* pos = Head_->next_;
	ScheTime* pre_pos = Head_;

	//find the position where new element should be insert
	while( pos != NULL ) {
		if( pos->SendTime_ > SendTime ) {
			break;
		}
		else {
			pos = pos->next_;
			pre_pos = pre_pos->next_;
		}

	}
	/*
	 * insert new element after pre_pos
	 */
	newElem->next_ = pos;
	pre_pos->next_ = newElem;
}


//get the top element, but not pop it

ScheTime* ScheQueue::top()
{
	return Head_->next_;
}

//pop the top element
void ScheQueue::pop()
{
	if( Head_->next_ != NULL ) {

		ScheTime* tmp = Head_->next_;
		Head_->next_ = Head_->next_->next_;

		if( tmp->timer_.isPending())//TimerHandler::TIMER_PENDING ) {
		{
		  tmp->timer_.cancel();
		}

		delete tmp;
	}
}




void ScheQueue::clearExpired(Time CurTime)
{
	ScheTime* NextSch = NULL;
	while( (NextSch = top()) && NextSch->SendTime_ < CurTime ) {
		pop();
	}
}


void ScheQueue::print(Time GuardTime,
		                  Time MaxTxTime,
		                  bool IsMe,
		                  nsaddr_t index)
{
	ScheTime* pos = Head_->next_;
	char file_name[30];
	strcpy(file_name, "schedule_");
	file_name[strlen(file_name)+1] = '\0';
	file_name[strlen(file_name)] = char(index+'0');
	FILE* stream = fopen(file_name, "a");
	if( IsMe )
			fprintf(stream, "I send  ");
	while( pos != NULL ) {
		fprintf(stream, "(%f--%f, %f) ", pos->SendTime_,
			pos->SendTime_+MaxTxTime, pos->SendTime_+GuardTime+MaxTxTime);
		pos = pos->next_;
	}
	fprintf(stream, "\n");
	fclose(stream);
}



void DMac_CallbackHandler::handle(Event* e)
{
	mac_->CallbackProcess(e);
}


void DMac_StatusHandler::handle(Event* e) {
	mac_->StatusProcess(e);
}

void DMac_WakeTimer::expire(Event *e)
{

  mac_->wakeup(ScheT_->node_id_);

}

void DMac_SleepTimer::expire(Event *e)
{
	mac_->sleep();
}


void DMac_PktSendTimer::expire(Event *e) {
	mac_->TxPktProcess(e, this);
}

void DMac_DataPktSendTimer::expire(Event *e) {
  mac_->sendDataPkt(p_, ack_);
}

void DMac_StartTimer::expire(Event* e) {
	mac_->start();
}

//bind the tcl object
static class DMacClass : public TclClass {
public:
	DMacClass():TclClass("Mac/UnderwaterMac/DMac") {}
	TclObject* create(int, const char*const*) {
		return (new DMac());
	}
}class_dmac;


Time DMac::InitialCyclePeriod_ = 15.0;
Time DMac::MaxPropTime_ = UnderwaterChannel::Transmit_distance()/1500.0;
Time DMac::MaxTxTime_ = 0.0;
Time DMac::ListenPeriod_ = 0.0;		//the length of listening to the channel after transmission.
Time DMac::hello_tx_len = 0.0;
Time DMac::WakePeriod_ = 0.0;
ofstream outfile("~/NS2/ns-2.30/result/dmacfile");

DMac::DMac():
  UnderwaterMac(),
  callback_handler(this),
  /*pkt_send_timer(this),*/
  status_handler(this),
  sleep_timer(this),
  start_timer_(this),
  dataSendHandler(NULL),
  WakeSchQueue_(this)
{
	CycleCounter_ = 1;
	NumPktSend_ = 0;
  NumDataSend_ = 0;
	bind("CyclePeriod", &AvgCyclePeriod_);

	start_timer_.resched(0.001);

	if(!outfile)
		cout << "outfile wrong!!!";

}


void DMac::send_info() {
	FILE* result_f = fopen("send.data", "a");
	fprintf(result_f, "MAC(%d) : num_send = %d\n", index_, NumPktSend_);
	printf("MAC(%d) : num_send = %d\n", index_, NumPktSend_);
	fclose(result_f);
}
Time DMac::getAvailableSendTime(Time startTime, Time interval,
                                Time guardTime, Time maxTxTime)
{
  Time start = startTime - guardTime - maxTxTime;
  Time end   = startTime + interval + guardTime + maxTxTime;
  Time res = -1.0;
  map<NodeID, NodeInfo>::iterator pos;
  for(pos= neighbors_.begin(); pos != neighbors_.end(); ++ pos)
  {
    Time posStart = pos->second.base;
    Time posEnd   = posStart + pos->second.listenInterval;
    if( (start < posStart && end < posStart) || (start > posEnd && end > posEnd) )
        ;
    else
      res = (res > end) ? res : end;
  }
  return res;
}


void DMac::sendFrame(Packet* p, bool IsMacPkt, Time delay)
{
	outfile << index_ << "DMac::sendFrame" << "delay:" << delay << endl;
	hdr_cmn* cmh = HDR_CMN(p);
	cmh->direction() = hdr_cmn::DOWN;
	cmh->txtime() = cmh->size()*encoding_efficiency_/bit_rate_;

	DMac_PktSendTimer *tmp = new DMac_PktSendTimer(this);
	tmp->tx_time() = cmh->txtime();
	tmp->p_ = p;
	tmp->resched(delay);
	PktSendTimerSet_.insert(tmp);

}


void DMac::TxPktProcess(Event *e, DMac_PktSendTimer* pkt_send_timer)
{
	outfile << index_  << " DMac::TxPktProcess" << endl;
	Packet* p = pkt_send_timer->p_;
	pkt_send_timer->p_ = NULL;

	((UnderwaterSensorNode*) node_)->SetTransmissionStatus(SEND);
	//outfile << "DMac::TxPktProcess sendDown" << endl;
	hdr_mac* mh=hdr_mac::access(p);
	int dst = mh->macDA();
	int src = mh->macSA();

	hdr_dmac* dh = HDR_DMAC(p);
	Time now = Scheduler::instance().clock();

	dh->t_send = now;

	outfile << now << " # "<< index_
			        << " src: " << src
					<< " dst: " << dst
					<< " nexttime: " << dh->cycle_period + dh->base
					<< endl;
	//outfile << index_ <<  " DMac::TxProcess" << endl;

	outfile << now <<" # "<< index_
			<<" sendDown dst: "<< dst
			<<" src: "<< src <<endl;
	NumPktSend_++;
	sendDown(p);
	Scheduler::instance().schedule(&status_handler,
				&status_event, pkt_send_timer->tx_time() );

	PktSendTimerSet_.erase(pkt_send_timer);
	delete pkt_send_timer;
}
void DMac::sendDataPkt(Packet* p, bool ack)
{
  outfile << index_  << " DMac::sendDataPkt" << endl;

  hdr_cmn* cmh = HDR_CMN(p);
  cmh->direction() = hdr_cmn::DOWN;
  cmh->txtime() = cmh->size()*encoding_efficiency_/bit_rate_;

  hdr_mac* mh=hdr_mac::access(p);
  int dst = mh->macDA();
  int src = mh->macSA();

  hdr_dmac* dh = HDR_DMAC(p);
  Time now = Scheduler::instance().clock();
  dh->ptype = DP_DATA;
  dh->pk_num = NumPktSend_;    // sequence number
  dh->data_num = NumDataSend_;
  dh->ack = ack;
  dh->sender_addr = src;
  dh->t_make = now;
  dh->receiver_addr = dst;
  dh->base = neighbors_[src].base;
  dh->interval = neighbors_[src].listenInterval;
  dh->t_send = now;
  dh->cycle_period = neighbors_[src].cycle;




  outfile << now << " # "<< index_
          << " src: " << src
          << " dst: " << dst
          << " nexttime: " << dh->cycle_period + dh->base
          << endl;

  if( ((UnderwaterSensorNode*)node_)->TransmissionStatus() == SLEEP )
    Poweron();
  if( sleep_timer.IsPending() )
    sleep_timer.cancel();
  ((UnderwaterSensorNode*) node_)->SetTransmissionStatus(SEND);
  setSleepTimer(WakePeriod_);
  sendDown(p);
  NumPktSend_++;
  NumDataSend_++;
  Scheduler::instance().schedule(&status_handler,
        &status_event, cmh->txtime() );

  if( (!ack) && (waitingPackets_[dst].empty() != true) )
  {
    Packet* p = waitingPackets_[dst].front();
    waitingPackets_[dst].pop_front();
    if(dataSendHandler != NULL)
      delete dataSendHandler;
    dataSendHandler = new DMac_DataPktSendTimer(this,p,false);
    dataSendHandler->resched(MaxTxTime_);
  }

}

void DMac::CallbackProcess(Event* e)
{
	callback_->handle(e);
}


void DMac::StatusProcess(Event *e)
{
	UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
	if( SEND == n->TransmissionStatus() ){
		n->SetTransmissionStatus(IDLE);
	}
  	return;
}


Packet* DMac::makeSYNCPkt(Time CyclePeriod, nsaddr_t Recver)
{
	outfile << "DMac::makeSYNCPkt:"<< endl;
	Packet* p = Packet::alloc();


	hdr_dmac *dh = HDR_DMAC(p);//hdr_dmac::access(p);
	dh->cycle_period = AvgCyclePeriod_;
	dh->ptype = DP_SYNC;
	dh->base = CyclePeriod;
	dh->interval = ListenPeriod_;

	hdr_cmn* cmh = HDR_CMN(p);// (hdr_cmn::access(p))
  cmh->size() = hdr_dmac::size();
  cmh->next_hop() = Recver;  //the sent packet??
  cmh->direction()=hdr_cmn::DOWN;
  cmh->addr_type()=NS_AF_ILINK;
  cmh->ptype()=PT_DMAC;

	hdr_mac* mh=HDR_MAC(p);
	mh->macDA() = Recver;
	mh->macSA() = index_;

	return p;
}



void DMac::wakeup(nsaddr_t node_id)
{

	Time now = Scheduler::instance().clock();
	outfile << now <<" # " << index_ << " DMac::wakeup for " << node_id << endl;
	WakeSchQueue_.clearExpired(now);
	neighbors_[index_].base = now;
  WakeSchQueue_.push(neighbors_[node_id].cycle + now,//neighbors_[node_id].base,
                         node_id,
                         neighbors_[node_id].cycle);

	if (node_id == index_)
	{
		if( ((UnderwaterSensorNode*)node_)->TransmissionStatus() == SLEEP )
		  Poweron();
	  if( sleep_timer.IsPending() )
	    sleep_timer.cancel();

	  setSleepTimer(WakePeriod_);

	}
	else if( waitingPackets_[node_id].empty() != true)
	{
	  Packet* p = waitingPackets_[node_id].front();
    waitingPackets_[node_id].pop_front();
    if(dataSendHandler != NULL)
      delete dataSendHandler;
    dataSendHandler = new DMac_DataPktSendTimer(this,p,false);
    dataSendHandler->resched(MaxTxTime_);

//	  while(waitingPackets_[node_id].size()> 1)
//	  {
//	    p = waitingPackets_[node_id].front();
//
//	    waitingPackets_[node_id].pop_front();
//
//	    sendDataPkt(p,false);
//	  }
//
//	  p = waitingPackets_[node_id].front();
//
//    waitingPackets_[node_id].pop_front();
//
//    sendDataPkt(p,true);
	}

}



void DMac::sleep()
{
	Poweroff();
}



void DMac::setSleepTimer(Time Interval)
{
	sleep_timer.resched(Interval);
}


/*
 *process the incoming packet
 */
void DMac::RecvProcess(Packet *p)
{
	//sendUp(p);
	Time now = Scheduler::instance().clock();
	outfile << index_  << " DMac::RecvProcess" << endl;

	hdr_mac*  mh = HDR_MAC(p);
	int      dst = mh->macDA();
	int      src = mh->macSA();
	hdr_cmn* cmh = HDR_CMN(p);


  if( cmh->error() )
  {
    outfile << "DMac::RecvProcess cmh-> error" << endl;

    if(drop_)
	    drop_->recv(p,"Error/Collision");
    else
			Packet::free(p);

    return;
  }

  outfile << now << " # "
          << index_ << "DMac::RecvProcess dst:"<< dst << " src:"<< src << endl;

	hdr_dmac* dh = HDR_DMAC(p);

  if(dh->ptype == DP_SYNC)
	{

	  outfile << "DMac::RecvProcess is DP_SYNC" << endl;
	  outfile << now << " # "<< index_
			      << " src: " << src
			      << " nexttime: " << dh->cycle_period + dh->base
			      << " interval: " << dh->interval
			      << endl;
	  neighbors_[src].delay = now - dh->t_send;
	  neighbors_[src].cycle = dh->cycle_period;
	  neighbors_[src].base = dh->base;
	  neighbors_[src].listenInterval = dh->interval;
	  WakeSchQueue_.push(dh->base,
	                     src,
	                     dh->base - now);
	  NextCyclePeriod_ = getAvailableSendTime(neighbors_[index_].base,
	                                          neighbors_[index_].listenInterval,
	                                          2*MaxPropTime_,
	                                          MaxTxTime_);
	  outfile << now << " # "<< index_
	              << " src: " << src
	              << " reschedule: " << NextCyclePeriod_
	              << endl;
    if(NextCyclePeriod_ > 111110.0)
	  {
	    //if it overlaps with others, re-generate a cycle period

	    WakeSchQueue_.push(NextCyclePeriod_, index_, NextCyclePeriod_-now);

	    outfile << now << " # "<< index_
	                << " src: " << src
	                << " schedul: " << neighbors_[index_].base
	                << " reschedul: " << NextCyclePeriod_
	                << endl;
	    neighbors_[index_].base = NextCyclePeriod_;
	    sendFrame(makeSYNCPkt(NextCyclePeriod_),true,1);

	  }
	}
	else if(dh->ptype == DP_DATA)
	{
		outfile << "DMac::RecvProcess is DP_DATA" << endl;

		outfile << now << " # "<< index_
						<< " src: " << src
						<< " nexttime: " << dh->cycle_period + dh->base
						<< endl;


		if( dst == index_)
		{
			outfile << now << " # " << index_  << " DMac::RecvProcess DESTDATA" << endl;

		  handleRecvData(p);
      sendUp(p);
			//TODO
			//sendDown(makeACKDATA)
			//schedule(makeACKDATA)
			return;
		}

	}
	else if(dh->ptype == DP_ACKDATA)
	  {

	    outfile << now << " # "<< index_
	            << " src: " << src
	            << " nexttime: " << dh->cycle_period + dh->base
	            << " DMac::RecvProcess is ACKDATA"
	            << endl;


	    if( dst == index_)
	    {
	      outfile << now << " # " << index_  << " DMac::RecvProcess DESTACKDATA" << endl;
	      //TODO
	      //handle(ACKDATA)
	      return;
	    }

	  }
	outfile << index_  <<" DMac::RecvProcess Packet::free(p)" << endl;
	//packet sent to other nodes will be freed
	Packet::free(p);
}
void DMac::handleRecvData(Packet* p)
{
  Time now = Scheduler::instance().clock();
  hdr_dmac* dh_p = HDR_DMAC(p);

  if (dh_p->ack) {
    Packet* q = Packet::alloc();


    hdr_dmac* dh_q = HDR_DMAC(q);
    int src = dh_p->sender_addr;
    neighbors_[src].delay = now - dh_p->t_send;//update the delay between src and index_


    dh_q->ptype = DP_ACKDATA;
    dh_q->pk_num = dh_p->pk_num+1;    // sequence number
    dh_q->data_num = dh_p->data_num;
    dh_q->ack = false;
    dh_q->sender_addr = index_;
    dh_q->t_make = now;
    dh_q->receiver_addr = src;
    dh_q->base = neighbors_[index_].base;
    dh_q->interval = neighbors_[index_].listenInterval;
    dh_q->t_send = now;
    dh_q->cycle_period = neighbors_[index_].cycle;


    hdr_cmn* cmh_q = HDR_CMN(q);// (hdr_cmn::access(p))
    cmh_q->size() = hdr_dmac::size();
    cmh_q->next_hop() = src;  //the sent packet??
    cmh_q->direction()=hdr_cmn::DOWN;
    cmh_q->addr_type()=NS_AF_ILINK;
    cmh_q->ptype()=PT_DMAC;
    cmh_q->txtime() = cmh_q->size()*encoding_efficiency_/bit_rate_;
    hdr_mac* mh_q=HDR_MAC(q);
    mh_q->macDA() = src;
    mh_q->macSA() = index_;
    outfile << now << " # "
            << index_
            << " dest:" << src
            << " sendDown SENDACKDATA"<< endl;
    NumPktSend_++;
    ((UnderwaterSensorNode*) node_)->SetTransmissionStatus(SEND);
    sendDown(q);

    Scheduler::instance().schedule(&status_handler, &status_event, cmh_q->txtime());
  }
}

/*
 * process the outgoing packet
 */
void DMac::TxProcess(Packet *p)
{
	/* because any packet which has nothing to do with this node is filtered by
	 * RecvProcess(), p must be qualified packet.
	 * Simply cache the packet to simulate the pre-knowledge of next transmission time
	 */

	hdr_mac* mh=HDR_MAC(p);
  hdr_cmn* cmh=HDR_CMN(p);
  hdr_uwvb* vbh = HDR_UWVB(p);
  hdr_dmac *dh = HDR_DMAC(p);//hdr_dmac::access(p);

	int src = mh->macSA();
	Time now = Scheduler::instance().clock();
	outfile << now <<" # "<< index_
			    <<" TxProcess"
          << " src:"<< src;
          //<< endl;


  int dst = vbh->target_id.addr_;
  outfile << " id:" << cmh->uid_
      		<< " dest:" << dst
          << endl;

  HDR_CMN(p)->size() = 1600;
  cmh->next_hop() = dst;  //the sent packet??
  cmh->direction()=hdr_cmn::DOWN;
  cmh->addr_type()=NS_AF_ILINK;
  cmh->ptype() == PT_DMAC;


  dh->cycle_period = neighbors_[index_].cycle;
  dh->base = neighbors_[index_].base;
  dh->ptype = DP_DATA;
  dh->interval = ListenPeriod_;


  mh->macDA() = dst;
  mh->macSA() = index_;

	waitingPackets_[dst].push_back(p);

	Scheduler::instance().schedule(&callback_handler,
						&callback_event, DMAC_CALLBACK_DELAY);
}


void DMac::SYNCSchedule(bool initial)
{
	outfile << "DMac::SYNCSchedule" << endl;
	//time is not well scheduled!!!!!
	Time now = Scheduler::instance().clock();
	Time RandomDelay = Random::uniform(1, InitialCyclePeriod_);
	outfile <<"node"  << index_ << "delay" << RandomDelay << endl;

	NextCyclePeriod_ = RandomDelay + InitialCyclePeriod_ + now;
	if( initial ) {
		outfile << "DMac::SYNCSchedule initial" << endl;

    neighbors_[index_].delay = 0;
    neighbors_[index_].cycle = AvgCyclePeriod_;
    neighbors_[index_].base = NextCyclePeriod_;
    neighbors_[index_].listenInterval = ListenPeriod_;
		WakeSchQueue_.push(NextCyclePeriod_,  index_, NextCyclePeriod_ - now);
		//WakeSchQueue_.print(2*MaxPropTime_, MaxTxTime_, true, index_);

		sendFrame(makeSYNCPkt(NextCyclePeriod_), true, RandomDelay);
		return;
	}

}


void DMac::start()
{
	outfile << "DMac::start" << endl;

	((UnderwaterSensorNode*)node_)->SetTransmissionStatus(IDLE);

	Random::seed_heuristically();

	SYNCSchedule(true);
	MaxTxTime_ = 1610*encoding_efficiency_/bit_rate_;
	hello_tx_len = (hdr_dmac::size())*8*encoding_efficiency_/bit_rate_;
	ListenPeriod_ = 10*hello_tx_len + 2*MaxPropTime_ + MaxTxTime_;
	WakePeriod_ = ListenPeriod_ + MaxTxTime_;
}


int DMac::command(int argc, const char *const *argv)
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












