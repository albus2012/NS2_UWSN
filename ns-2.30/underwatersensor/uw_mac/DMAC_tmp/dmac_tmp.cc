/*
In this version of DMAC, the major modifications for the 15-6-2013
  version are :

*/

//#include "packet.h"
//#include "underwatersensor/uw_common/underwatersensornode.h"
#include "underwatersensor/uw_routing/vectorbasedforward.h"
//#include "mac.h"
#include "dmac.h"
//#include "../underwaterphy.h"
//#include "../underwatermac.h"
//#include "random.h"
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


Time DMac::InitialCyclePeriod_ = 10.0;
Time DMac::MaxPropTime_ = UnderwaterChannel::Transmit_distance()/1500.0;
Time DMac::MaxTxTime_ = 0.0;
Time DMac::ListenPeriod_ = 0.0;		//the length of listening to the channel after transmission.
Time DMac::hello_tx_len = 0.0;
Time DMac::WakePeriod_ = 0.0;
ofstream outfile("result/dmacfile");

DMac::DMac():
  UnderwaterMac(),
  callback_handler(this),
  /*pkt_send_timer(this),*/
  status_handler(this),
  sleep_timer(this),
  start_timer_(this),
  WakeSchQue_(this)
{
	CycleCounter_ = 2;
	NumPktSend_ = 0;
	bind("AvgCyclePeriod", &AvgCyclePeriod_);
	bind("StdCyclePeriod", &StdCyclePeriod_);
	start_timer_.resched(0.001);
	next_hop_num = 0;
	//ofstream outfile;
	//outfile.open();
	if(!outfile)
		cout << "outfile wrong!!!";

}


void DMac::send_info() {
	FILE* result_f = fopen("send.data", "a");
	fprintf(result_f, "MAC(%d) : num_send = %d\n", index_, NumPktSend_);
	printf("MAC(%d) : num_send = %d\n", index_, NumPktSend_);
	fclose(result_f);
}


void DMac::sendFrame(Packet* p, bool IsMacPkt, Time delay)
{
	outfile << index_ << "DMac::sendFrame" << endl;
	hdr_cmn* cmh = HDR_CMN(p);
	cmh->direction() = hdr_cmn::DOWN;
	cmh->txtime() = cmh->size()*encoding_efficiency_/bit_rate_;

	DMac_PktSendTimer *tmp = new DMac_PktSendTimer(this);
	tmp->tx_time() = HDR_CMN(p)->txtime();
	tmp->p_ = p;
	tmp->resched(delay);
	PktSendTimerSet_.insert(tmp);

	//pkt_send_timer.tx_time() = HDR_CMN(p)->txtime();
	//pkt_send_timer.p_ = p;
	//pkt_send_timer.resched(delay);  //set transmission status when this timer expires

	//Scheduler::instance().schedule(downtarget_, p, delay+0.0001);
	/*if( !IsMacPkt ) {
		Scheduler::instance().schedule(&callback_handler,
						&callback_event, delay_time+DMac_CALLBACK_DELAY);
	}*/
	//callback_handler ?????????
}


void DMac::TxPktProcess(Event *e, DMac_PktSendTimer* pkt_send_timer)
{
	outfile << index_  << " DMac::TxPktProcess" << endl;
	Packet* p = pkt_send_timer->p_;
	pkt_send_timer->p_ = NULL;

//	if( ((UnderwaterSensorNode*) node_)->TransmissionStatus() == SEND
//			|| ((UnderwaterSensorNode*) node_)->TransmissionStatus() == RECV ) {
//		//if the status is not IDLE (SEND or RECV), the scheduled event cannot be
//		//execute on time. Thus, drop the packet.
//		outfile << index_  << " DMac::TxPktProcess status is not IDLE " << endl;
//		if(drop_)
//			drop_->recv(p,"Schedule Failure");
//		else
//			Packet::free(p);
//		PktSendTimerSet_.erase(pkt_send_timer);
//		return;
//	}


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
					<< " nexttime: " << dh->cycle_period + now
					<< endl;
	//outfile << index_ <<  " DMac::TxProcess" << endl;

	outfile << now <<" # "<< index_
			<<" sendDown dst: "<< dst
			<<" src: "<< src <<endl;

	sendDown(p);
	Scheduler::instance().schedule(&status_handler,
				&status_event, pkt_send_timer->tx_time() );

	PktSendTimerSet_.erase(pkt_send_timer);
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
	dh->cycle_period = CyclePeriod;
	dh->ptype = DP_SYNC;
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


//Packet* DMac::fillMissingList(Packet* p)
//{
//	outfile << "DMac::fillMissingList:"<< endl;
//	hdr_cmn* cmh = HDR_CMN(p);
//	set<nsaddr_t> ML_;
//	set_difference(neighbors_.begin(), neighbors_.end(),
//		CL_.begin(), CL_.end(),
//		insert_iterator<set<nsaddr_t> >(ML_, ML_.begin()));
//
//	p->allocdata( sizeof(uint) + ML_.size()*sizeof(nsaddr_t) );
//	cmh->size() += 8*( sizeof(uint) + ML_.size()*sizeof(nsaddr_t) );
//
//    unsigned char* walk = (unsigned char*)p->accessdata();
//    *(uint*)walk = ML_.size();
//    walk += sizeof(uint);
//
//    for( set<nsaddr_t>::iterator pos=ML_.begin();
//         pos != ML_.end(); pos++)
//    {
//        *(nsaddr_t*)walk = *pos;
//        walk += sizeof(nsaddr_t);
//    }
//
//    return p;
//}



//Packet* DMac::fillSYNCHdr(Packet *p, Time CyclePeriod)
//{
//	outfile << "DMac::fillSYNCHdr" << endl;
//	hdr_cmn* cmh = HDR_CMN(p);
//	hdr_SYNC *hdr_s = hdr_SYNC::access(p);
//	hdr_s->cycle_period() = CyclePeriod;
//
//	cmh->size() += hdr_s->size();
//
//	return p;
//}


void DMac::wakeup(nsaddr_t node_id)
{

	Time now = Scheduler::instance().clock();
	outfile << now <<" # " << index_ << " DMac::wakeup for " << node_id << endl;
	WakeSchQue_.clearExpired(now);
  WakeSchQue_.push(neighbors_[node_id].cycle + neighbors_[node_id].base,
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
	  sendFrame(p, true, 0);
	}

//
//
//	if( node_id == index_ ) {
//		//generate the time when this node will send out next packet
//		CycleCounter_ = (++CycleCounter_) % 1000;
//
//		switch( CycleCounter_ ) {
//			case 0:
//				//This node would keep awake for InitialCyclePeriod_.
//				//And this is set in start().
//				SYNCSchedule();  //node keeps awake in this period
//				return;
///*
//			case 9:
//				NextCyclePeriod_ = InitialCyclePeriod_ + now;
//				break;
//*/
//			default:
//				//not this node's SYNC period, just send out the data packet
//				NextCyclePeriod_ = genNxCyclePeriod();
//				break;
//		}
//        //cout <<"maxproptime: " << MaxPropTime_
//        //     <<"maxproptime: " << MaxTxTime_ << endl;
//		if( ! WakeSchQue_.checkGuardTime(NextCyclePeriod_, 2*MaxPropTime_, MaxTxTime_) ) {
//			NextCyclePeriod_ =
//				WakeSchQue_.getAvailableSendTime(now+WakePeriod_,
//										NextCyclePeriod_, 2*MaxPropTime_, MaxTxTime_);
//			cout << "re cyclePeriod" << endl;
//		}
//
//		WakeSchQue_.push(NextCyclePeriod_, index_, NextCyclePeriod_-now);
//		//WakeSchQue_.print(2*MaxPropTime_, MaxTxTime_, true, index_);
//		if( PacketQueue_.empty() )
//			sendFrame(makeSYNCPkt(NextCyclePeriod_-now),true);
//		else
//			sendoutPkt(NextCyclePeriod_);
//	}
//	else {
//		//WakeSchQue_.push(genNxCyclePeriod(), node_id, genNxCyclePeriod()-now);
//		CL_.erase(node_id);
//	}
//
//	//set the sleep timer
//	if( sleep_timer.IsPending() )
//		sleep_timer.cancel();
//
//	setSleepTimer(WakePeriod_);

}


void DMac::sleep()
{

	//if( setWakeupTimer() ) {   //This node set the timer to wake up itself
	//	Poweroff();
	//	((UnderwaterSensorNode*)Node_)->SetTransmissionStatus(SLEEP);
	//}
	Poweroff();
}



void DMac::setSleepTimer(Time Interval)
{
	sleep_timer.resched(Interval);
}



Time DMac::genNxCyclePeriod()
{
	//return NextCyclePeriod_ + Random::normal(AvgCyclePeriod_, StdCyclePeriod_);
	return NextCyclePeriod_ +AvgCyclePeriod_;
	//+Random::uniform(AvgCyclePeriod_-StdCyclePeriod_,AvgCyclePeriod_+StdCyclePeriod_);
}


/*
 *process the incoming packet
 */
void DMac::RecvProcess(Packet *p)
{
	sendUp(p);
	Time now = Scheduler::instance().clock();
	outfile << index_  << " DMac::RecvProcess" << endl;

	hdr_mac* mh=HDR_MAC(p);
	int dst = mh->macDA();
	int src = mh->macSA();
	hdr_cmn* cmh=HDR_CMN(p);


  if( cmh->error() )
  {
    outfile << "DMac::RecvProcess cmh-> error" << endl;
    //printf("broadcast:node %d  gets a corrupted packet at  %f\n",index_,NOW);
    if(drop_)
	    drop_->recv(p,"Error/Collision");
    else
			Packet::free(p);

    return;
  }

  outfile << index_ << "DMac::RecvProcess dst:"<< dst << " src:"<< src << endl;

	//CL_.insert(src);			//update the contact list

	hdr_dmac* dh = HDR_DMAC(p);
	//dh->cycle_period() -= PRE_WAKE_TIME;

	if(dh->ptype == DP_SYNC)
	{

	  outfile << "DMac::RecvProcess is PT_AUV_SYNC" << endl;
	  outfile << now << " # "<< index_
			      << " src: " << src
			      << " nexttime: " << dh->cycle_period+now
			      << endl;
	  neighbors_[src].delay = now - dh->t_send;
	  neighbors_[src].cycle = dh->cycle_period;
	  neighbors_[src].base = dh->t_send;
	  neighbors_[src].listenInterval = dh->interval;
	  WakeSchQue_.push(dh->cycle_period + dh->t_send,
	                         src,
	                         dh->cycle_period);

		//the process to hello packet is same to SYNC packet
		//WakeSchQue_.push(SYNC_h->cycle_period()+now ,
		//		               src,
		//		               SYNC_h->cycle_period() );

		//WakeSchQue_.print(2*MaxPropTime_, MaxTxTime_, false, index_);
	}
	else if(dh->ptype == DP_DATA)
	{
		 outfile << "DMac::RecvProcess is DP_DATA" << endl;
		    /*
		    * it must be data packet. we should extract the SYNC hdr & missing list
		    */
			/*either unicasted or broadcasted*/
			//need overhearing!
			//update the schedule queue
			//then send packet to upper layers
		if( index_ == dst )
			//printf("node(%d) recv %s\n", index_, packet_info.name(cmh->ptype()));
		outfile << now << " # "<< index_
						<< " src: " << src
						<< " nexttime: " << dh->cycle_period + now
						<< endl;
//		WakeSchQue_.push(SYNC_h->cycle_period_+now ,
//				               src,
//				               SYNC_h->cycle_period());
//
//		WakeSchQue_.print(2*MaxPropTime_,
//		                    MaxTxTime_,
//		                    false,
//		                    index_);

		//extract Missing list
		//processMissingList(p->accessdata(), src);  //hello is sent to src in this function

		if( dst == index_)
		{
			outfile << now << " # " << index_  << " DMac::RecvProcess DEST DP_DATA" << endl;
			//sendUp(p);
			//TODO
			//sendDown(makeACKDATA)
			//schedule(makeACKDATA)
			return;
		}

	}
	outfile << index_  <<" DMac::RecvProcess Packet::free(p)" << endl;
	//packet sent to other nodes will be freed
	Packet::free(p);
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

	int src = mh->macSA();
	Time now = Scheduler::instance().clock();
	outfile << now <<" # "<< index_
			    <<" TxProcess"

          << " src:"<< src;
          //<< endl;

	hdr_cmn* cmh=HDR_CMN(p);
	hdr_uwvb* vbh = HDR_UWVB(p);
  int dst = vbh->target_id.addr_;
  outfile << " id:" << cmh->uid_
      		<< " dest:" << vbh->target_id.addr_;

  assert(cmh->ptype()== DP_DATA);
//	switch (cmh->ptype_)
//	{
//		case PT_AUV_HELLO:
//		case PT_AUV_SYNC:
//			outfile << " PT_AUV_SYNC" << endl;
//			break;
//		case PT_UWVB:
//			outfile << " PT_UWVB" << endl;
//			break;
//		default:
//			outfile << " other" << endl;
//			break;
//	}

	HDR_CMN(p)->size() = 1600;
	waitingPackets_[dst].push_back(p);

	Scheduler::instance().schedule(&callback_handler,
						&callback_event, DMAC_CALLBACK_DELAY);
}


void DMac::SYNCSchedule(bool initial)
{
	outfile << "DMac::SYNCSchedule" << endl;
	//time is not well scheduled!!!!!
	Time now = Scheduler::instance().clock();
	NextCyclePeriod_ = InitialCyclePeriod_ + now;
	if( initial ) {
		outfile << "DMac::SYNCSchedule initial" << endl;
		Time RandomDelay = Random::uniform(0, InitialCyclePeriod_);
		WakeSchQue_.push(NextCyclePeriod_+RandomDelay, index_, NextCyclePeriod_+RandomDelay-now);
		//WakeSchQue_.print(2*MaxPropTime_, MaxTxTime_, true, index_);

		sendFrame(makeSYNCPkt(NextCyclePeriod_-now), true, RandomDelay);
		return;
	}

	//NextCyclePeriod_ = genNxCyclePeriod();
	//check whether next cycle period is available.
	//if( ! WakeSchQue_.checkGuardTime(NextCyclePeriod_, 2*MaxPropTime_, MaxTxTime_) ) {
		//if it overlaps with others, re-generate a cycle period
		//cout << "adjust" << endl;
		//NextCyclePeriod_ = WakeSchQue_.getAvailableSendTime(now+WakePeriod_,
			//			NextCyclePeriod_, 2*MaxPropTime_, MaxTxTime_);
	//}

	//WakeSchQue_.push(NextCyclePeriod_, index_, NextCyclePeriod_-now);
	//WakeSchQue_.print(2*MaxPropTime_, MaxTxTime_, true, index_);
	//sendFrame(makeSYNCPkt(NextCyclePeriod_-now),true);
}


void DMac::start()
{
	outfile << "DMac::start" << endl;
	//init WakeSchQueue. Before sleep, Wake Schedule Queue will pop this value.
	//WakeSchQue_.push(0.0, index_, -1); //the timer will not start
	((UnderwaterSensorNode*)node_)->SetTransmissionStatus(IDLE);

	Random::seed_heuristically();

	SYNCSchedule(true);
	MaxTxTime_ = 1610*encoding_efficiency_/bit_rate_;
	hello_tx_len = (hdr_dmac::size())*8*encoding_efficiency_/bit_rate_;
	ListenPeriod_ = 10*hello_tx_len + 2*MaxPropTime_ + MaxTxTime_;
	WakePeriod_ = ListenPeriod_ + MaxTxTime_;
}


/*
 * send out one packet from upper layer
 */
//void DMac::sendoutPkt(Time NextCyclePeriod)
//{
//	outfile << "DMac::sendoutPkt" << endl;
//	if( PacketQueue_.empty() ) {
//			return; /*because there is no packet, this node cannot sendout packet.
//					 * This is due to the stupid idea proposed by the authors of this protocol.
//					 * They think mac protocol cannot when it will sendout the next packet.
//					 * However, even a newbie knows it is impossible.
//					 */
//	}
//
//	Time now = Scheduler::instance().clock();
//	//get a packet cached in the queue
//	Packet* pkt = PacketQueue_.front();
//	PacketQueue_.pop();
//	NumPktSend_++;
//	//send_info();
//	outfile << "DMac::sendoutPkt NumPktSend:" << NumPktSend_ << endl;
//
//	//hdr_mac* mh=hdr_mac::access(p);
//	//int dst = mh->macDA();
//	//int src = mh->macSA();
//
//	hdr_cmn* cmh = hdr_cmn::access(pkt);
//	hdr_uwvb* vbh = hdr_uwvb::access(pkt);
//	outfile << index_ << "DMac::hdr_uwvb::access(pkt):" << vbh->target_id.addr_ << endl;
//	/*next_hop() is set in IP layerequal to the */
//
//	//fill the SYNC & Missing list header
//	fillSYNCHdr(pkt, NextCyclePeriod-now);
//	//whether backoff?
//	fillMissingList(pkt);
//
//    cmh->direction()=hdr_cmn::DOWN;
//    cmh->addr_type()=NS_AF_ILINK;
//
//	nsaddr_t next_hop = MAC_BROADCAST;
//
//	if( neighbors_.size() != 0 ) {
//		set<nsaddr_t>::iterator pos = neighbors_.begin();
//		for(uint i=0; i<next_hop_num; i++, pos++);
//		cmh->next_hop() = *pos;
//		next_hop_num = (next_hop_num+1)%neighbors_.size();
//		//vbh->target_id.addr_ = cmh->next_hop();
//	}
//	cmh->next_hop() = vbh->target_id.addr_;
//	hdr_mac* mh=hdr_mac::access(pkt);
//	int dst = mh->macDA();
//	int src = mh->macSA();
//	outfile << index_ << "DMac::sendoutPkt dst:"<< dst << " src:"<< src << endl;
//	outfile << "DMac::sendoutPkt mh->macDA() = cmh->next_hop():" << mh->macDA()<< endl;
//	mh->macDA() = cmh->next_hop();
//	mh->macSA() = index_;
//
//	sendFrame(pkt, false);
//}



//void DMac::processMissingList(unsigned char* ML, nsaddr_t src)
//{
//	uint node_num_ = *((uint*)ML);
//	ML += sizeof(uint);
//	nsaddr_t tmp_addr;
//
//	for(uint i=0; i<node_num_; i++ ) {
//		tmp_addr = *((nsaddr_t*)ML);
//		if( index_ == tmp_addr ) {
//			//make and send out the hello packet
//			Packet* p = Packet::alloc();
//			hdr_SYNC *hdr_s = hdr_SYNC::access(p);
//			hdr_s->cycle_period() = NextCyclePeriod_
//										- Scheduler::instance().clock();
//
//			hdr_cmn* cmh = HDR_CMN(p);
//
//			cmh->next_hop() = src;
//			cmh->direction()=hdr_cmn::DOWN;
//			cmh->addr_type()=NS_AF_ILINK;
//			cmh->ptype_ = PT_AUV_HELLO;
//			cmh->size() = hdr_SYNC::size();
//
//			hdr_mac* mh=hdr_mac::access(p);
//			mh->macDA() = src;
//			mh->macSA() = index_;
//
//			//rand the sending slot and then send out
//			sendFrame(p, true, Random::integer(10)*hello_tx_len);   //hello should be delayed!!!!!!
//			return;
//		}
//		ML += sizeof(nsaddr_t);
//	}
//
//}



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













