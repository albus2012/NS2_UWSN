/*
 * This is a traffic generator. 
 * The traffic pattern follows Poisson Process
 * author: Yibo Zhu (yibo.zhu@engr.uconn.edu)
 */


#include <stdlib.h>
 
#include "random.h"
#include "trafgen.h"
#include "ranvar.h"
#include <agent.h>


class UW_Poi_Traffic : public TrafficGenerator {
public:
	UW_Poi_Traffic();
	virtual void init();
	virtual double next_interval(int &size);
	
protected:
	int	maxpkts_;  //negative number means infinity
	int seqno_;
	double data_rate_; /*initialize it as negative number to 
					 *check if it's set by user*/
};

UW_Poi_Traffic::UW_Poi_Traffic(): maxpkts_(-1), 
	seqno_(0), data_rate_(-1)
{
	size_ = 1;
	bind("maxpkts_", &maxpkts_);
	bind("packetSize_", &size_);
	bind("data_rate_", &data_rate_);
	
}

void UW_Poi_Traffic::init()
{
	if( !(data_rate_>0 && size_>0) ) {
		fprintf(stderr, "Must set data_rate_ and packetSize_ before using UW_Poi_Traffic");
		exit(1);
	}
	Random::seed_heuristically();
	if( agent_ ) {
		agent_->set_pkttype(PT_UW_MESSAGE);
	}
}


double UW_Poi_Traffic::next_interval(int &size)
{
	double R=Random::uniform();
	double lambda_=data_rate_;
	double t = -log(R)/lambda_;
	
	if( (maxpkts_ < 0) || ( (maxpkts_ >= 0) && (++seqno_ < maxpkts_)) )
		return(t);
	else
		return(-1);  //with -1, timeout() will stop function
}


static class UW_POI_TrafficClass : public TclClass {
 public:
	UW_POI_TrafficClass() : TclClass("Application/Traffic/UW_POI") {}
	TclObject* create(int, const char*const*) {
		return (new UW_Poi_Traffic());
	}
} class_uw_poi_traffic;




