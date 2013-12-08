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
DMac_Topology DMac::topo;

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

void DMac_DataSendTimer::expire(Event *e)
{
  mac_->processDataSendTimer(this);
}


void DMac_WakeTimer::expire(Event *e)
{

//  if(mac_->isNodeWaitFlag() || mac_->isNodeListenFlag())
//  {
//
//    mac_->setNodeSendFlag();
//    mac_->canSend();
//  }
  mac_->updateNodeTime();
  resched(mac_->getStartTime());
  mac_->canSend();
}
void DMac_WakeTimer::change(double t)
{
  if(isPending())
    cancel();
  resched(t);
}

void DMac_SleepTimer::expire(Event *e)
{
//  if(!mac_->isNodeListenFlag())
//  {
//
//    mac_->setNodeListenFlag();
//    mac_->updateNodeTime();
//  }
//  mac_->updateNodeTime();
  resched(mac_->getRoundTime());
}

void DMac_StartTimer::expire(Event* e)
{
  mac_->start();
}
void DMac_InitTimer::expire(Event* e)
{
  mac_->init();
}



DMac_Topology::DMac_Topology()
{

}

void DMac_Topology::addNode(int id, double x, double y, double z)
{
  Point p ={id, x, y, z};
  outfile << "point" << p.id << p.x << p.y << p.z << endl;
  points[id] = p;
}

void DMac_Topology::setDelay()
{

  delay.clear();
  for(size_t i = 0; i < points.size(); i++)
  {
    vector<double> v;
    for(int j = 0; j < points.size(); j++)
    {
      int n1 = points[i].id;
      int n2 = points[j].id;
      double dx = (points[i].x - points[j].x) / 1500.0;
      double dy = (points[i].y - points[j].y) / 1500.0;
      double dz = (points[i].z - points[j].z) / 1500.0;

      v.push_back(sqrt(dx*dx + dy*dy + dz*dz));
    }
    delay.push_back(v);
  }
}

double DMac_Topology::getDelay(int i, int j)
{
  return delay[i][j];
}

double DMac_Topology::getMaxProp()
{
  double t = 0;
  setDelay();
  for(int i = 0; i < delay.size(); i++)
    for(int j = i+1; j < delay.size(); j++)
      if(t < delay[i][j])
        t = delay[i][j];
  return t;
}

double subtsp(vector<vector<double> >&paths, vector<int> used, int pos, vector<int>&res)
{

  used.push_back(pos);
  int len = paths.size();
  if(used.size() == len)
  {
    return paths[pos][0];
  }
  else
  {
    double temp = 10000;
    int next = 0;
    vector<int> t;
    for(int i = 0; i < len; i++)
    {
      if(find(used.begin(), used.end() ,i) == used.end())
      {
        vector<int> v;
        double rest = subtsp(paths,used, i,v);
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
  outfile << "length" << subtsp(paths, used, 0, res);
  outfile << endl;
  return res;
}

vector<int> DMac_Topology::getPath()
{

  path = vector<int>(1,0);
  setDelay();
  for(int i = 0; i < delay.size(); i++)
  {
    for(int j = 0; j < delay[i].size(); j++)
    {
      outfile << delay[i][j] << " ";
    }
    outfile << endl;
  }
  outfile << endl;

  vector<int> t = tsp(delay);
  for(int i = 0 ; i < t.size(); i++)
    path.push_back(t[i]);

  return path;

}

int DMac_Topology::getNextNode(int node)
{
  int size = path.size();
  for(int i = 0; i < size; i++)
  {
    if(node == path[i])
      return path[(i + 1) % size];
  }
}
int DMac_Topology::getPos(int node)
{
  int size = path.size();
  for(int i = 0; i < size; i++)
  {
    if(node == path[i])
      return i;
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


//Time DMac::GuardTime = UnderwaterChannel::Transmit_distance() / 1500.0;//0.7333

Time DMac::GuardTime = 0.1;
Time DMac::sendInterval = 1.9;
Time DMac::baseTime = 0.2;
int DMac::nodeCount = 7;
int DMac::DataSize = 500;
int DMac::ACKSize = 20;

DMac::DMac()
    : UnderwaterMac(), callback_handler(this),
    /*pkt_send_timer(this),*/
    status_handler(this), sleep_timer(this),
    start_timer_(this), wake_timer(this),
    init_timer_(this)
{
  CycleCount = 1;
  NumPktSend_ = 0;
  NumDataSend_ = 0;
  //int nodeC = 0;
  bind("nodeCount",&nodeCount);
  bind("sendInterval", &sendInterval);
  bind("baseTime",&baseTime);
  bind("guardTime",&GuardTime);
  bind("dataSize", &DataSize);
  bind("ackSize", &ACKSize);
  //nodeCount = nodeC;


  init_timer_.resched(0.05);

  if (!outfile)
    cout << "outfile wrong!!!";
//  double X, Y, Z;    // location of this node
    double X, Y, Z;    // location of this node
//    if(node_)
//    ((UnderwaterSensorNode*)node_)->getLoc(&X, &Y, &Z);
//    topo.addNode(index_, X, Y, Z);
//    outfile << "location" << index_ << X << Y << Z << endl;

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
  cmh->txtime() = getTxDataTime(cmh->size());

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

  Time now = Scheduler::instance().clock();



  if(waitingPackets_.empty() != true)
  {
    Packet* p = waitingPackets_.front();
    waitingPackets_.pop_front();
    hdr_mac* mh = HDR_MAC(p);
    hdr_cmn* cmh = HDR_CMN(p);
    int dst = mh->macDA();

    canSendACK(now+cmh->txtime(), dst);
    DMac_DataSendTimer* DataSendTimer = new DMac_DataSendTimer(this);
    DataSendTimer->pkt_ = p;
    DataSendTimer->resched(getTxTime(ACKSize));
    //sendData(p);
    return;
  }
  else
  {
    canSendACK(now, BROADCAST);
    return;
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

  //int dst = (index_+ 1) % nodeCount;
  int dst = topo.getNextNode(index_);

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


  if(dst == index_ || dst == BROADCAST)
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
//  if(dh->ptype == DP_END)
//  {
//    outfile << now()
//        << " # " << index_
//        << "recv end flag"
//        << endl;
//    Packet::free(p);
//    canSend();
//  }

  if(dh->ptype == DP_DATA)
  {
    handleRecvData(p);
    sendUp(p);
  }
  else if(dh->ptype == DP_ACKDATA)
  {
    handleRecvACK(p);
    if(dh->receiver_addr == index_)
    {
      Packet::free(p);
    }
    return;
  }
}

void DMac::handleRecvACK(Packet* p)
{
  hdr_dmac* dh = HDR_DMAC(p);
  int dst = dh->receiver_addr;
  double endTime = dh->t_send;


  if(dst == index_)
  {
    outfile << index_ << " " << "dst recv ACK" << endl;
    if(endTime < now())
    {
      wake_timer.change(0.001);
    }
    else
    {
      changeSendTime(p);
    }
  }
}

void DMac::changeSendTime(Packet* p)
{
  hdr_dmac* dh = HDR_DMAC(p);

  int src = dh->sender_addr;

  double endTime = dh->t_send;
  double backoff = endTime;
  for(int i = 0; i < nodeCount; i++)
  {
    double t = endTime + topo.getDelay(src,i)-topo.getDelay(i,index_);
    if(backoff < t)
    {
      backoff = t;
    }
  }
  wake_timer.change(backoff-now());


}

Packet* DMac::makeACK(Time t, int target)
{
  Time now = Scheduler::instance().clock();

  Packet* q = Packet::alloc();
  int dst = topo.getNextNode(index_);


  hdr_cmn* cmh_q = HDR_CMN(q);
  cmh_q->size() = ACKSize;
  cmh_q->next_hop() = dst;
  cmh_q->direction() = hdr_cmn::DOWN;
  cmh_q->addr_type() = NS_AF_ILINK;
  cmh_q->ptype() = PT_DMAC;
  cmh_q->txtime() = getTxTime(cmh_q->size());
  outfile << "ack packet size is:" << cmh_q->txtime() << endl;

  hdr_dmac* dh_q = HDR_DMAC(q);
  dh_q->ptype = DP_ACKDATA;
  dh_q->pk_num = 1;    // sequence number
  dh_q->data_num = 1;
  dh_q->ack = false;
  dh_q->sender_addr = index_;
  dh_q->t_make = now;
  dh_q->receiver_addr = dst;
  dh_q->t_send = t + cmh_q->txtime();

  hdr_mac* mh_q = HDR_MAC(q);
  mh_q->macDA() = BROADCAST;
  mh_q->macSA() = index_;


  return q;
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
  cmh_q->size() = ACKSize;//hdr_dmac::size();
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
//  hdr_dmac* dh = HDR_DMAC(p);
//  if(dh->ack)
//    canSendACK(p);
  ;
}

void DMac::canSendACK(Packet* p)
{

  Packet* q = makeACK(p);

  sendout(q);

  updateNumPktSend();

}
void DMac::canSendACK(Time t, int target)
{
  Packet* q = makeACK(t, target);
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
    case RECV:
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

//    case RECV:
//      outfile << now()
//            << " # " << index_
//            << " RECV-SEND Collision"
//            << " src:" << src
//            << " dest:" << dst
//            << endl;
//      printf("RECV-SEND Collision!\n");
//      Packet::free(p);
//
//      break;

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

  cmh->size() = DataSize;
  cmh->next_hop() = dst;
  cmh->direction()=hdr_cmn::DOWN;
  cmh->addr_type()=NS_AF_ILINK;
  cmh->ptype() == PT_DMAC;
  cmh->txtime() = getTxDataTime(cmh->size());

  dh->ptype = DP_DATA;


  mh->macDA() = dst;
  mh->macSA() = index_;

  waitingPackets_.push_back(p);

  Scheduler::instance().schedule(&callback_handler,
      &callback_event, DMAC_CALLBACK_DELAY);

}
void DMac::init()
{
  double X, Y, Z;    // location of this node
  ((UnderwaterSensorNode*)node_)->getLoc(&X, &Y, &Z);
  topo.addNode(index_, X, Y, Z);
  topo.getPath();
  start_timer_.resched(0.05);
}


void DMac::start()
{
  outfile << "DMac::start" << endl;

//  double X, Y, Z;    // location of this node
//  ((UnderwaterSensorNode*)node_)->getLoc(&X, &Y, &Z);
//  topo.addNode(index_, X, Y, Z);
//  topo.getPath();
//  for(int i = 0; i < res.size(); i++)
//    outfile << res[i] << endl;
  initNodeTime();
//  double X, Y, Z;    // location of this node
//  ((UnderwaterSensorNode*)node_)->getLoc(&X, &Y, &Z);
//  topo.addNode(index_, X, Y, Z);
//  outfile << "location" << index_ << X << Y << Z << endl;

  vector<int> res = topo.getPath();
  for(int i = 0; i < res.size(); i++)
    outfile << res[i];
  outfile << "path" << endl;

  sleep_timer.resched(getEndTime());
  wake_timer.resched(getStartTime());
  Random::seed_heuristically();

//  bind("nodeCount", &nodeCount);
//  bind("sendInterval", &sendInterval);
//  bind("baseTime",&baseTime);
//  bind("GuardTime",&GuardTime);
//  if(index_ ==0)
//    canSend();
//  MaxTxTime_ = 1610 * encoding_efficiency_ / bit_rate_;
//  hello_tx_len = (hdr_dmac::size()) * 8 * encoding_efficiency_ / bit_rate_;



}
Time DMac::getStartTime()
{
  return nodeTime.startSend - now();
}

Time DMac::getEndTime()
{
  return nodeTime.endSend - now();
}



Time DMac::getRoundTime()
{
  return nodeCount * sendInterval + nodeCount * GuardTime;
}

void DMac::initNodeTime()
{

  nodeTime.startSend = baseTime +  topo.getPos(index_)* (sendInterval + GuardTime);
  nodeTime.endSend = nodeTime.startSend + sendInterval;
}

void DMac::updateNodeTime()
{
  nodeTime.startSend += getRoundTime();
  nodeTime.endSend += getRoundTime();
  outfile <<index_ << " " << nodeTime.startSend << " " << nodeTime.endSend << endl;
  addCycleCount();
}

void DMac::processDataSendTimer(DMac_DataSendTimer * DataSendTimer)
{
  DMac_DataSendTimer* tmp = DataSendTimer;
  sendData(DataSendTimer->pkt_);
  DataSendTimer->pkt_ = NULL;
  delete tmp;
}

bool DMac::isSendEndFlag()
{

  if(nodeTime.startSend - now() >
        nodeCount * sendInterval - nodeCount * GuardTime)
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
//      double X, Y, Z;
//      ((UnderwaterSensorNode*)node_)->getLoc(&X, &Y, &Z);
//      topo.addNode(index_, X, Y, Z);
      return TCL_OK;
    }
  }

  return UnderwaterMac::command(argc, argv);
}

