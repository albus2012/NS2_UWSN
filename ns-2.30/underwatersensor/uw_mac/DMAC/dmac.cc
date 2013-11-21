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

static class DMACHeaderClass: public PacketHeaderClass
{

public:
  DMACHeaderClass()
      : PacketHeaderClass("PacketHeader/DMAC", sizeof(hdr_dmac))
  {
    bind_offset(&hdr_dmac::offset_);
  }

} class_dmac_hdr;

void ScheQueue::push(Time SendTime, nsaddr_t node_id, Time Interval)
{
  ScheTime* newElem = new ScheTime(SendTime, node_id, mac_);
  newElem->start(Interval);

  ScheTime* pos = Head_->next_;
  ScheTime* pre_pos = Head_;

  //find the position where new element should be insert
  while (pos != NULL)
  {
    if (pos->SendTime_ > SendTime)
    {
      break;
    }
    else
    {
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
  if (Head_->next_ != NULL)
  {

    ScheTime* tmp = Head_->next_;
    Head_->next_ = Head_->next_->next_;

    if (tmp->timer_.isPending()) //TimerHandler::TIMER_PENDING ) {
    {
      tmp->timer_.cancel();
    }

    delete tmp;
  }
}

void ScheQueue::clearExpired(Time CurTime)
{
  ScheTime* NextSch = NULL;
  while ((NextSch = top()) && NextSch->SendTime_ < CurTime)
  {
    pop();
  }
}

void ScheQueue::print(Time GuardTime, Time MaxTxTime, bool IsMe, nsaddr_t index)
{
  ScheTime* pos = Head_->next_;
  char file_name[30];
  strcpy(file_name, "schedule_");
  file_name[strlen(file_name) + 1] = '\0';
  file_name[strlen(file_name)] = char(index + '0');
  FILE* stream = fopen(file_name, "a");
  if (IsMe)
    fprintf(stream, "I send  ");
  while (pos != NULL)
  {
    fprintf(stream, "(%f--%f, %f) ", pos->SendTime_, pos->SendTime_ + MaxTxTime,
        pos->SendTime_ + GuardTime + MaxTxTime);
    pos = pos->next_;
  }
  fprintf(stream, "\n");
  fclose(stream);
}

void DMac_CallbackHandler::handle(Event* e)
{
  mac_->CallbackProcess(e);
}

void DMac_StatusHandler::handle(Event* e)
{
  mac_->StatusProcess(e);
}

void DMac_WakeTimer::expire(Event *e)
{
  mac_->wakeup(ScheT_->node_id_);
}

void DMac_SleepTimer::expire(Event *e)
{
  mac_->idle();
  mac_->setNodeListen();

}

void DMac_StartTimer::expire(Event* e)
{
  mac_->start();
}

//bind the tcl object
static class DMacClass: public TclClass
{
public:
  DMacClass()
      : TclClass("Mac/UnderwaterMac/DMac")
  {
  }
  TclObject* create(int, const char* const *)
  {
    return (new DMac());
  }
} class_dmac;


Time DMac::maxPropTime = UnderwaterChannel::Transmit_distance() / 1500.0;
Time DMac::sendInterval = 0.5;
Time DMac::baseTime = 0.0;
int DMac::nodeCount = 8;

ofstream outfile("/home/yongj/NS2/ns-2.30/result/dmacfile");

DMac::DMac()
    : UnderwaterMac(), callback_handler(this),
    /*pkt_send_timer(this),*/
    status_handler(this), sleep_timer(this), start_timer_(this), WakeSchQueue_(this)
{
  CycleCount = 1;
  NumPktSend_ = 0;
  NumDataSend_ = 0;

  start_timer_.resched(0.001);

  if (!outfile)
    cout << "outfile wrong!!!";

}

void DMac::send_info()
{
  FILE* result_f = fopen("send.data", "a");
  fprintf(result_f, "MAC(%d) : num_send = %d\n", index_, NumPktSend_);
  printf("MAC(%d) : num_send = %d\n", index_, NumPktSend_);
  fclose(result_f);
}

void DMac::makeData(Packet* p)
{
  outfile << index_ << " DMac::makeData" << endl;

  hdr_cmn* cmh = HDR_CMN(p);
  cmh->direction() = hdr_cmn::DOWN;
  cmh->txtime() = getTxTime(cmh->size());

  hdr_mac* mh = hdr_mac::access(p);
  int dst = mh->macDA();
  int src = mh->macSA();

  hdr_dmac* dh = HDR_DMAC(p);
  Time now = Scheduler::instance().clock();
  dh->ptype = DP_DATA;
  dh->pk_num = NumPktSend_;    // sequence number
  dh->data_num = NumDataSend_;

  dh->sender_addr = src;
  dh->t_make = now;
  dh->receiver_addr = dst;

  dh->t_send = now;

  ((UnderwaterSensorNode*) node_)->SetTransmissionStatus(SEND);

  sendDown(p);
  NumPktSend_++;
  NumDataSend_++;
  Scheduler::instance().schedule(&status_handler, &status_event, cmh->txtime());


}

void DMac::CallbackProcess(Event* e)
{
  callback_->handle(e);
}

void DMac::StatusProcess(Event *e)
{
  UnderwaterSensorNode* n = (UnderwaterSensorNode*) node_;
  if (SEND == n->TransmissionStatus())
  {
    n->SetTransmissionStatus(IDLE);
  }
  return;
}

void DMac::canSend()
{
  nodeFlag = NF_SEND;
  Time now = Scheduler::instance().clock();
  Time wakeTime = CycleCount * nodeCount * sendInterval + baseTime - now;
  outfile << now << " # " << index_ << " DMac::canSend";

  WakeSchQueue_.clearExpired(now);

  setSleepTimer(wakeTime);

  while(nodeFlag == NF_SEND)
  {

    if(waitingPackets_.empty() != true)
    {
      Packet* p = waitingPackets_.front();
      waitingPackets_.pop_front();
      sendData(p);
    }
    else
    {
      sendEndFlag();
      return;
    }
  }

}

void DMac::sendData(Packet* p)
{
  makeData(p);

  ((UnderwaterSensorNode*) node_)->SetTransmissionStatus(SEND);
  hdr_cmn* cmh = HDR_CMN(p);
  sendDown(p);
  NumPktSend_++;
  NumDataSend_++;
  Scheduler::instance().schedule(&status_handler, &status_event, cmh->txtime());

}

void DMac::sendEndFlag()
{

  ((UnderwaterSensorNode*) node_)->SetTransmissionStatus(SEND);
  Packet* p = makeEndPkt();
  hdr_cmn* cmh = HDR_CMN(p);
  sendDown(p);
  updateNumPktSend();
  Scheduler::instance().schedule(&status_handler, &status_event,
      cmh->txtime());

}

Packet* DMac::makeEndPkt()
{

  Time now = Scheduler::instance().clock();

  outfile << now
      << " # " << index_
      << " Make EndFlag"
      << endl;

  Packet* p = Packet::alloc();
  hdr_dmac* dh = HDR_DMAC(p);

  int dst = index_+1;

  dh->ptype = DP_END;
  dh->ack = false;
  dh->sender_addr = index_;
  dh->t_make = now;
  dh->receiver_addr = dst;
  dh->t_send = now;


  hdr_cmn* cmh = HDR_CMN(p);
  cmh->size() = hdr_dmac::size();
  cmh->next_hop() = dst;
  cmh->direction() = hdr_cmn::DOWN;
  cmh->addr_type() = NS_AF_ILINK;
  cmh->ptype() = PT_DMAC;
  cmh->txtime() = getTxTime(cmh->size());


  hdr_mac* mh = HDR_MAC(p);
  mh->macDA() = dst;
  mh->macSA() = index_;


  return p;

}

void DMac::idle()
{
  ((UnderwaterSensorNode*) node_)->SetTransmissionStatus(IDLE);
}

void DMac::wakeup(nsaddr_t node_id)
{
  ;
}

void DMac::setSleepTimer(Time Interval)
{
  sleep_timer.resched(Interval);
}

//process the incoming packet
void DMac::RecvProcess(Packet *p)
{
  //sendUp(p);
  Time now = Scheduler::instance().clock();
  outfile << index_ << " DMac::RecvProcess" << endl;

  hdr_mac* mh = HDR_MAC(p);
  int dst = mh->macDA();
  int src = mh->macSA();
  hdr_cmn* cmh = HDR_CMN(p);

  if (cmh->error())
  {
    outfile << "DMac::RecvProcess cmh-> error" << endl;

    if (drop_)
      drop_->recv(p, "Error/Collision");
    else
      Packet::free(p);

    return;
  }

  outfile << now
      << " # " << index_
      << "DMac::RecvProcess dst:" << dst
      << " src:" << src << endl;


  if(dst == index_)
  {

    outfile << "DMac::RecvProcess arrive at the dest" << endl;
    handleRecvData(p);
  }
}

//handle any packet that arrived at the destination
void DMac::handleRecvPkt(Packet* p)
{
  Time now = Scheduler::instance().clock();
  hdr_dmac* dh = HDR_DMAC(p);

  //the previous node has no data to send
  if(dh->ptype == DP_END)
  {
    canSend();
  }

  else if(dh->ptype == DP_DATA)
  {

    handleRecvData(p);
  }
  else if(dh->ptype == DP_ACKDATA)
  {
    return;
  }
}

Packet* DMac::makeACK(Packet* p)
{
  Time now = Scheduler::instance().clock();
  outfile << now
      << " # " << index_
      << " Make ACK"
      << endl;

  Packet* q = Packet::alloc();
  hdr_dmac* dh_p = HDR_DMAC(p);
  hdr_dmac* dh_q = HDR_DMAC(q);

  int src = dh_p->sender_addr;
  neighbors_[src].delay = now - dh_p->t_send; //update the delay between src and index_

  dh_q->ptype = DP_ACKDATA;
  dh_q->pk_num = dh_p->pk_num + 1;    // sequence number
  dh_q->data_num = dh_p->data_num;
  dh_q->ack = false;
  dh_q->sender_addr = index_;
  dh_q->t_make = now;
  dh_q->receiver_addr = src;
  dh_q->t_send = now;


  hdr_cmn* cmh_q = HDR_CMN(q);
  cmh_q->size() = hdr_dmac::size();
  cmh_q->next_hop() = src;
  cmh_q->direction() = hdr_cmn::DOWN;
  cmh_q->addr_type() = NS_AF_ILINK;
  cmh_q->ptype() = PT_DMAC;
  cmh_q->txtime() = getTxTime(cmh_q->size());


  hdr_mac* mh_q = HDR_MAC(q);
  mh_q->macDA() = src;
  mh_q->macSA() = index_;


  return q;

}

void DMac::handleRecvData(Packet* p)
{
  hdr_dmac* dh = HDR_DMAC(p);
  if(dh->ack)
    canSendACK(p);
}

void DMac::canSendACK(Packet* p)
{
  ((UnderwaterSensorNode*) node_)->SetTransmissionStatus(SEND);
  Packet* q = makeACK(p);
  hdr_cmn* cmh_q = HDR_CMN(q);
  sendDown(q);
  updateNumPktSend();
  Scheduler::instance().schedule(&status_handler, &status_event,
      cmh_q->txtime());

}

//process the outgoing packet
void DMac::TxProcess(Packet *p)
{
  /* because any packet which has nothing to do with this node is filtered by
  * RecvProcess(), p must be qualified packet.
  * Simply cache the packet to simulate the pre-knowledge of next transmission time
  */

  hdr_mac* mh = HDR_MAC(p);
  hdr_cmn* cmh=HDR_CMN(p);
  hdr_uwvb* vbh = HDR_UWVB(p);
  hdr_dmac *dh = HDR_DMAC(p);  //hdr_dmac::access(p);

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
  cmh->next_hop() = dst;
  cmh->direction()=hdr_cmn::DOWN;
  cmh->addr_type()=NS_AF_ILINK;
  cmh->ptype() == PT_DMAC;

  dh->ptype = DP_DATA;


  mh->macDA() = dst;
  mh->macSA() = index_;

  waitingPackets_.push_back(p);

  Scheduler::instance().schedule(&callback_handler,
      &callback_event, DMAC_CALLBACK_DELAY);


}


void DMac::start()
{
  outfile << "DMac::start" << endl;

  idle();

  Random::seed_heuristically();
//  bind("nodeCount", &nodeCount);
//  bind("sendInterval", &sendInterval);
//  bind("baseTime",&baseTime);
//  bind("maxPropTime",&maxPropTime);
  if(index_ ==0)
    canSend();
//  MaxTxTime_ = 1610 * encoding_efficiency_ / bit_rate_;
//  hello_tx_len = (hdr_dmac::size()) * 8 * encoding_efficiency_ / bit_rate_;

}

int DMac::command(int argc, const char * const *argv)
{
  if (argc == 3)
  {
    if (strcmp(argv[1], "node_on") == 0)
    {
      Node* n1 = (Node*) TclObject::lookup(argv[2]);
      if (!n1)
        return TCL_ERROR;
      node_ = n1;
      return TCL_OK;
    }
  }

  return UnderwaterMac::command(argc, argv);
}

