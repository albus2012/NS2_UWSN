#include "uw_drouting.h"
#include "cmu-trace.h"
#include "address.h"


int hdr_uw_drouting_pkt::offset_;//??????????? what does this offset mean?
static class uwdroutHeaderClass : public PacketHeaderClass {
public:
  uwdroutHeaderClass() : PacketHeaderClass("PacketHeader/uw_drouting",//?????what's relation?
  sizeof(hdr_uw_drouting_pkt)) {
	bind_offset(&hdr_uw_drouting_pkt::offset_);
  }
  
} class_rtuwdroutinghdr;

static class uw_droutingClass : public TclClass {
public:
  uw_droutingClass() : TclClass("Agent/uw_drouting") {}
  TclObject* create(int argc, const char*const* argv) {
	assert(argc == 5);
	return (new uw_drouting((nsaddr_t)Address::instance().str2addr(argv[4])));
  }
} class_rtuw_drouting;



void uw_drouting_PktTimer::expire(Event* e) {
  agent_->send_uw_drouting_pkt();
  //agent_->reset_uw_drouting_pkt_timer();
  resched(update_interval()+agent_->broadcast_jitter(10));
}


uw_drouting::uw_drouting(nsaddr_t id) : Agent(PT_UW_DROUTING), pkt_timer_(this, 50) {
  bind_bool("accessible_var_", &accessible_var_);
  ra_addr_ = id;
  rtable_.node_id() = id;
  coun=0;
}


int uw_drouting::command(int argc, const char*const* argv) {
  if (argc == 2) {
	if (strcasecmp(argv[1], "start") == 0) {
	  pkt_timer_.resched(0.0000001+broadcast_jitter(10));
	  return TCL_OK;
	}
	else if (strcasecmp(argv[1], "print_rtable") == 0) {
	  if (logtarget_ != 0) {
		sprintf(logtarget_->pt_->buffer(), "P %f _%d_ Routing Table",
				CURRENT_TIME,
				ra_addr());
		logtarget_->pt_->dump();
	  }
	  else {
		fprintf(stdout, "%f _%d_ If you want to print this routing table "
						"you must create a trace file in your tcl script",
						CURRENT_TIME,
						ra_addr());
	  }
	  return TCL_OK;
	}
  }
  else if (argc == 3) {
	// Obtains corresponding dmux to carry packets to upper layers
	if (strcmp(argv[1], "port-dmux") == 0) {
	  dmux_ = (PortClassifier*)TclObject::lookup(argv[2]);
	  if (dmux_ == 0) {
		fprintf(stderr, "%s: %s lookup of %s failed\n",
				__FILE__,
				argv[1],
				argv[2]);
		return TCL_ERROR;
	  }
	  return TCL_OK;
	}
	// Obtains corresponding tracer
	else if (strcmp(argv[1], "log-target") == 0 ||
	  strcmp(argv[1], "tracetarget") == 0) {
	  logtarget_ = (Trace*)TclObject::lookup(argv[2]);
	if (logtarget_ == 0)
	  return TCL_ERROR;
	return TCL_OK;
	  }
  }
  // Pass the command to the base class
  return Agent::command(argc, argv);
}




void uw_drouting::recv(Packet* p, Handler* h) {
  struct hdr_cmn* ch = HDR_CMN(p);
  struct hdr_ip* ih = HDR_IP(p);
  
  ch->uw_flag() = true;
  
  if (ih->saddr() == ra_addr()) {
	// If there exists a loop, must drop the packet
	if (ch->num_forwards() > 0) {
	  drop(p, DROP_RTR_ROUTE_LOOP);
	  return;
	}
	// else if this is a packet I am originating, must add IP header
	else if (ch->num_forwards() == 0)
	  ch->size() += IP_HDR_LEN;
  }
  
  
  else if( ch->next_hop()!= (nsaddr_t)IP_BROADCAST && ch->next_hop() != ra_addr() ) {
	drop(p, DROP_MAC_DUPLICATE);
	return;
  }
  
  
  
  ch->num_forwards() += 1;
  
  // If it is a protoname packet, must process it
  if (ch->ptype() == PT_UW_DROUTING) {
	recv_uw_drouting_pkt(p);
	return;
  }
  // Otherwise, must forward the packet (unless TTL has reached zero)
  else {
	ih->ttl_--;
	if (ih->ttl_ == 0) {
	  drop(p, DROP_RTR_TTL);
	  return;
	}
	forward_data(p);
  }
}


void uw_drouting::recv_uw_drouting_pkt(Packet* p) {
  
  struct hdr_uw_drouting_pkt* ph = HDR_UW_DROUTING_PKT(p);
  
  // All routing messages are sent from and to port RT_PORT,
  // so we check it.
  assert(ih->sport() == RT_PORT);
  assert(ih->dport() == RT_PORT);  
  // take out the packet, rtable
  
  DN temp_DN;
  rtable_t temp_rt;
  nsaddr_t temp1;
  unsigned char* walk = (unsigned char*)p->accessdata();
  for(uint i=0; i < ph->entry_num(); i++) {
	
	temp1 = *((nsaddr_t*)walk);
	walk += sizeof(nsaddr_t);
	
	temp_DN.first = *((nsaddr_t*)walk);
	//temp2 = *((nsaddr_t*)walk);
	walk += sizeof(nsaddr_t);
	
	//temp_DN[temp2]== *((int*)walk);
	temp_DN.second = *((int*)walk);	
	walk += sizeof(int);
	temp_rt[temp1]=temp_DN;  
  }
  
  
  rtable_.update(&temp_rt, ph->pkt_src()); 
  
  if (rtable_.ifchg() ==1){
	pkt_timer_.update_interval() = 30.0;
  }
  
  if (rtable_.ifchg() ==0)
  {
	coun++;
  }
  
  if (coun ==2){
	pkt_timer_.update_interval() = 100.0;
	coun=0;
  }
  
  
  // Release resources
  Packet::free(p);
}

double uw_drouting::broadcast_jitter(double range)
{
	return range*Random::uniform();
}


void uw_drouting::send_uw_drouting_pkt() {
  Packet* p = allocpkt();
  struct hdr_cmn* ch = HDR_CMN(p);
  struct hdr_ip* ih = HDR_IP(p);
  
  //add by jun
  struct hdr_uw_drouting_pkt* ph = HDR_UW_DROUTING_PKT(p);
  
  ph->pkt_src() = ra_addr();
  ph->pkt_len() = 7;
  ph->pkt_seq_num() = seq_num_++;
  ph->entry_num() = rtable_.size();
  
  ph->pkt_len()=sizeof(ph->pkt_len())+sizeof(ph->pkt_src())+sizeof(ph->pkt_seq_num())+sizeof(ph->entry_num());
  
  int DataSize = (rtable_.size())*(2*sizeof(nsaddr_t)+sizeof(int)) ;
  
  
  p->allocdata( DataSize );
  
  unsigned char* walk = (unsigned char*)p->accessdata();  
  
  for(rtable_t::iterator it=rtable_.rt_.begin(); it!=rtable_.rt_.end(); it++)    {  
	
	*(nsaddr_t*)walk = it->first;       
	
	walk += sizeof(it->first);  
	
	*(nsaddr_t*)walk = it->second.first;  
	
	walk += sizeof(it->second.first);  
	
	*(int*)walk = it->second.second;     
	
	walk += sizeof(it->second.second);   
	
  }
  
  ch->ptype() = PT_UW_DROUTING;
  
  ih->saddr() = ra_addr();
  ih->daddr() = IP_BROADCAST;
  ih->sport() = RT_PORT;
  ih->dport() = RT_PORT;
  ih->ttl() = 1;
  
  ch->direction() = hdr_cmn::DOWN;
  ch->size() = IP_HDR_LEN + ph->pkt_len()+DataSize;
  ch->error() = 0;
  ch->next_hop() = IP_BROADCAST;
  ch->addr_type() = NS_AF_INET;
  ch->uw_flag() = true;
  
  Scheduler::instance().schedule(target_, p, JITTER);
}


void uw_drouting::reset_uw_drouting_pkt_timer() {
  pkt_timer_.resched((double)50.0+broadcast_jitter(10));//   should be coustimized
} 

void uw_drouting::forward_data(Packet* p) {
  struct hdr_cmn* ch = HDR_CMN(p);
  struct hdr_ip* ih = HDR_IP(p);
  
  //double t = NOW;
  if (ch->direction() == hdr_cmn::UP &&
	((u_int32_t)ih->daddr() == IP_BROADCAST || ih->daddr() == ra_addr())) {
	ch->size() -= IP_HDR_LEN;
	dmux_->recv(p, (Handler*)NULL);
  return;
	}
	else {
	  ch->direction() = hdr_cmn::DOWN;
	  ch->addr_type() = NS_AF_INET;
	  if ((u_int32_t)ih->daddr() == IP_BROADCAST)
		ch->next_hop() = IP_BROADCAST;
	  else {
		nsaddr_t next_hop = rtable_.lookup(ih->daddr());
		
		if (next_hop == (nsaddr_t)IP_BROADCAST) {
		  debug("%f: Agent %d can not forward a packet destined to %d\n",
					 CURRENT_TIME, ra_addr(), ih->daddr());
  drop(p, DROP_RTR_NO_ROUTE);
  return;
		}
		else
		  ch->next_hop() = next_hop;
	  }
	  Scheduler::instance().schedule(target_, p, 0.0);
	}
}
