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
ofstream outfile("/home/yongj/NS2/ns-2.30/result/dmacfile");

int hdr_dmac::offset_;
Topology DMac::topo(4);

static class DMACHeaderClass: public PacketHeaderClass
{

public:
  DMACHeaderClass()
      : PacketHeaderClass("PacketHeader/DMAC", sizeof(hdr_dmac))
  {
    bind_offset(&hdr_dmac::offset_);
  }

} class_dmachdr;



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

  if(mac_->isNodeWaitFlag() || mac_->isNodeListenFlag())
  {

    mac_->setNodeSendFlag();
    mac_->canSend();
  }

  resched(mac_->getRoundTime());
}

void DMac_SleepTimer::expire(Event *e)
{
  if(!mac_->isNodeListenFlag())
  {

    mac_->setNodeListenFlag();
    mac_->updateNodeTime();
  }

  resched(mac_->getRoundTime());
}

void DMac_StartTimer::expire(Event* e)
{
  mac_->start();

}

Topology::Topology(int n)
:delay(n, vector<double>(n))
{
}

void Topology::addNode(int id, int x, int y, int z)
{
  Point p={id, x, y, z};
  points.push_back(p);
}

void Topology::setDelay()
{
  for(int i = 0; i < points.size(); i++)
  {
    for(int j = 0; j < points.size(); j++)
    {
      int n1 = points[i].id;
      int n2 = points[j].id;
      double dx = (points[i].x - points[j].x) / 1500.0;
      double dy = (points[i].y - points[j].y) / 1500.0;
      double dz = (points[i].z - points[j].z) / 1500.0;

      delay[n1][n2] = sqrt(dx*dx + dy*dy + dz*dz);
    }
  }
}

double Topology::getDelay(int i, int j)
{
  return delay[i][j];
}
int subtsp(vector<vector<double> >&paths, vector<int> used, int pos, vector<int>&res)
{
  outfile << "get";
  used.push_back(pos);
  int len = paths.size();
  if(used.size() == len)
  {
    return paths[pos][0];
  }
  else
  {
    int temp = 100;
    int next = 0;
    vector<int> t;
    for(int i = 0; i < len; i++)
    {
      if(find(used.begin(), used.end() ,i) == used.end())
      {
        vector<int> v;
        int rest = subtsp(paths,used, i,v);
        if(rest+paths[pos][i] < temp)
        {
          temp = rest+paths[pos][i];
          next = i;
          t = v;
        }
      }
    }
    t.push_back(next);
    res = t;
    return temp;
  }
}

// use dp method to solve the TSP problem
// get one of the shortest paths
vector<int> tsp(vector<vector<double> >&paths)
{
  vector<int> used;
  vector<int> res;
  outfile <<  subtsp(paths, used, 0, res);
  outfile << endl;
  for(int i = 0; i < res.size(); i++)
    outfile << res[i];
  return res;
}

vector<int> Topology::getPath()
{
  setDelay();
  path = tsp(delay);
}

int Topology::getNextNode(int node)
{
  int size = path.size();
  for(int i = 0; i < size; i++)
  {
    if(node == path[i])
      return (i + 1) % size;
    else
      return -1;
  }
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


//Time DMac::maxPropTime = UnderwaterChannel::Transmit_distance() / 1500.0;//0.7333

Time DMac::maxPropTime = 0;
Time DMac::sendInterval = 2.0;
Time DMac::baseTime = 0.2;
int DMac::nodeCount = 8;




DMac::DMac()
    : UnderwaterMac(), callback_handler(this),
    /*pkt_send_timer(this),*/
    status_handler(this), sleep_timer(this),
    start_timer_(this), wake_timer(this)
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
  outfile << now()
      << " # " << index_
      << " DMac::makeData" << endl;

  hdr_cmn* cmh = HDR_CMN(p);
  cmh->direction() = hdr_cmn::DOWN;
  cmh->txtime() = getTxTime(cmh->size());

  outfile << "DATA packet size is: " << cmh->txtime() << endl;
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


  setNodeSendFlag();
  Time now = Scheduler::instance().clock();

  while(getNodeFlag() == NF_SEND)
  {

    if(waitingPackets_.empty() != true)
    {
      Packet* p = waitingPackets_.front();
      waitingPackets_.pop_front();
      sendData(p);
      return;
    }
    else
    {
      setNodeNoDataFlag();
      sendEndFlag();
      return;
    }
  }

}

void DMac::sendData(Packet* p)
{

  outfile << now() << " # " << index_
      << " sendData" << endl;

  makeData(p);

  sendout(p);

}

void DMac::sendEndFlag()
{

  if(isSendEndFlag())
  {
    Packet* p = makeEndPkt();

    sendout(p);

    setNodeListenFlag();
  }
  else
  {
    setNodeWaitFlag();
  }

}

Packet* DMac::makeEndPkt()
{

  Time now = Scheduler::instance().clock();

  Packet* p = Packet::alloc();
  hdr_dmac* dh = HDR_DMAC(p);

  int dst = (index_+ 1) % nodeCount;

  dh->ptype = DP_END;
  dh->ack = false;
  dh->sender_addr = index_;
  dh->t_make = now;
  dh->receiver_addr = dst;
  dh->t_send = now;


  hdr_cmn* cmh = HDR_CMN(p);
  cmh->size() = hdr_dmac::size();
  //cmh->next_hop() = dst;
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

  hdr_mac* mh = HDR_MAC(p);
  int dst = mh->macDA();
  int src = mh->macSA();
  hdr_cmn* cmh = HDR_CMN(p);

  if (cmh->error())
  {
    outfile << now()
        << "DMac::RecvProcess cmh-> error"
        << " at:" << index_
        << " src:" << src
        << " dest" << dst
        << endl;

    if (drop_)
      drop_->recv(p, "Error/Collision");
    else
      Packet::free(p);

    return;
  }

  outfile << now()
      << " # " << index_
      << " RecvProcess dst:" << dst
      << " src:" << src << endl;


  if(dst == index_)
  {

    outfile << "DMac::RecvProcess arrive at the dest" << endl;
    handleRecvPkt(p);
  }
}

//handle any packet that arrived at the destination
void DMac::handleRecvPkt(Packet* p)
{

  hdr_dmac* dh = HDR_DMAC(p);

  //the previous node has no data to send
  if(dh->ptype == DP_END)
  {
    outfile << now()
        << " # " << index_
        << "recv end flag"
        << endl;
    Packet::free(p);
    canSend();
  }

  else if(dh->ptype == DP_DATA)
  {
    handleRecvData(p);
    sendUp(p);
  }
  else if(dh->ptype == DP_ACKDATA)
  {
    Packet::free(p);
    return;
  }
}

Packet* DMac::makeACK(Packet* p)
{
  Time now = Scheduler::instance().clock();

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

  Packet* q = makeACK(p);

  sendout(q);

  updateNumPktSend();

}

int DMac::sendout(Packet* p)
{
  UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;

  hdr_cmn* cmh = HDR_CMN(p);
  hdr_mac* mh = HDR_MAC(p);
  int dst = mh->macDA();
  int src = mh->macSA();

  switch( n->TransmissionStatus() )
  {
    case SLEEP:
      Poweron();

    case IDLE:
      outfile << now()
            << " # " << index_
            << " send out"
            << " src:" << src
            << " dest:" << dst
            << endl;
      n->SetTransmissionStatus(SEND);
      cmh->timestamp() = now();
      updateNumPktSend();
      sendDown(p);
      Scheduler::instance().schedule(&status_handler,&status_event,cmh->txtime());
      break;

    case RECV:
      outfile << now()
            << " # " << index_
            << " RECV-SEND Collision"
            << " src:" << src
            << " dest:" << dst
            << endl;
      printf("RECV-SEND Collision!\n");
      Packet::free(p);

      break;

    default:
      //status is SEND, send too fast
      outfile << now()
      << " # " << index_
      << " send too fast"
      << " src:" << src
      << " dest:" << dst
      << endl;

      printf("node%d send data too fast\n",index_);
      Packet::free(p);
      break;
  }
  return  1;
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

  int src = vbh->sender_id.addr_;
  int dst = vbh->target_id.addr_;

  outfile << now() <<" # "<< index_
  <<" TxProcess"
  << " src:"<< src;


  outfile << " id:" << cmh->uid_
  << " dest:" << dst
  << endl;

  HDR_CMN(p)->size() = 600;
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

  initNodeTime();

  topo.addPoint(0, 0, 0, 0);
  topo.addPoint(1, 0, 1, 0);
  topo.addPoint(2, 1, 1, 0);
  topo.addPoint(3, 1, 0, 0);
  vector<int> res = topo.getPath();
  for(int i = 0; i < res.size(); i++)
    outfile << res[i];
  outfile << "path" << endl;

  sleep_timer.resched(getInitEndTime());
  wake_timer.resched(getInitStartTime());
  Random::seed_heuristically();

//  bind("nodeCount", &nodeCount);
//  bind("sendInterval", &sendInterval);
//  bind("baseTime",&baseTime);
//  bind("maxPropTime",&maxPropTime);
//  if(index_ ==0)
//    canSend();
//  MaxTxTime_ = 1610 * encoding_efficiency_ / bit_rate_;
//  hello_tx_len = (hdr_dmac::size()) * 8 * encoding_efficiency_ / bit_rate_;



}
Time DMac::getInitStartTime()
{
  return nodeTime.startSend - now();
}

Time DMac::getInitEndTime()
{
  return nodeTime.endSend - now();
}

Time DMac::getRoundTime()
{
  return nodeCount * sendInterval + (nodeCount - 1) * maxPropTime;
}

void DMac::initNodeTime()
{
  nodeTime.startSend = baseTime + index_ * sendInterval + index_*maxPropTime;
  nodeTime.endSend = nodeTime.startSend + sendInterval;
}

void DMac::updateNodeTime()
{
  nodeTime.startSend += getRoundTime();
  nodeTime.endSend += getRoundTime();
  addCycleCount();
}

bool DMac::isSendEndFlag()
{

  if(nodeTime.startSend - now() >
        nodeCount * sendInterval - nodeCount * maxPropTime)
    return false;
  else
    return true;
}
Time DMac::now()
{
  Time t = Scheduler::instance().clock();
  return t;
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

