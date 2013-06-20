

#ifndef __ns_underwatersensornode_h__
#define __ns_underwatersensornode_h__

#include "object.h"
#include "trace.h"
#include "lib/bsd-list.h"
#include "phy.h"
#include "topography.h"
#include "arp.h"
#include "node.h"
#include "gridkeeper.h" // I am not sure if it is useful in our case
#include "energy-model.h"
#include "location.h"
#include "underwatersensor/uw_mobility_pattern/mobility_pattern_allinone.h"

enum TransmissionStatus {SLEEP,IDLE,SEND,RECV, NStatus};
#include "underwatersensor/uw_mac/underwaterphy.h"

class GridKeeper; // think it later

 //NStatus is used to get the number of elements in this enums



#if COMMENT_ONLY
-----------------------
|			|
|	Upper Layers	|
|			|
-----------------------
|		    |
|		    |
-------	 -------
|	|	|	|
|  LL	|	|  LL	|
|	|	|	|
-------	 -------
|		    |
|		    |
-------	 -------
|	|	|	|
| Queue	|	| Queue	|
|	|	|	|
-------	 -------
|		    |
|		    |
-------	 -------
|	|	|	|
|  Mac	|	|  Mac	|
|	|	|	|
-------	 -------
|		    |
|		    |
-------	 -------	 -----------------------
|	|	|	|	|			|
| Netif	| <---	| Netif | <---	|	Mobile Node	|
|	|	|	|	|			|
-------	 -------	 -----------------------
|		    |
|		    |
-----------------------
|			|
|	Channel(s) 	|
|			|
-----------------------
#endif


class UnderwaterSensorNode;

class UnderwaterPositionHandler : public Handler {
public:
	UnderwaterPositionHandler(UnderwaterSensorNode* n) : node(n) {}
	void handle(Event*);
private:
	UnderwaterSensorNode *node;
};


class UnderwaterSensorNode : public MobileNode 
{
	friend class UnderwaterPositionHandler;
	friend class UWMobilityPattern; 	//added by Yibo
	friend class UW_Kinematic;		
	friend class UW_RWP;
public:

	UnderwaterSensorNode();
	~UnderwaterSensorNode();
	virtual int command(int argc, const char*const* argv);
	double propdelay(UnderwaterSensorNode*);
	void	move();
	void start();
	int clearSinkStatus();// added by peng xie
	int setSinkStatus();// added by peng xie
	void check_position();	

	// added by peng Xie m
	inline double CX() { return CX_; }
	inline double CY() { return CY_; }
	inline double CZ() { return CZ_; }
	inline int sinkStatus(){return sinkStatus_;}
	inline int failure_status(){return failure_status_;}
	inline double failure_pro(){return failure_pro_;}
	inline double failure_status_pro(){return failure_status_pro_;}
	
	inline void SetTransmissionStatus(enum TransmissionStatus status) {
		trans_status = status;
	}
	
	//set status considering state transition
	/*
	inline double SetTransmissionStatus(enum TransmissionStatus status, UnderwaterPhy* phy){
		//compare the pre_trans, current_status, and i
		//then decide the transition time
		double transit_time = 0.0;
		if( trans_status == IDLE ) {
			if( NOW-status_change_time() >= phy->getTransitTime(trans_status, status)  ) {
				transit_time = phy->getTransitTime(trans_status, status);
			}
			else {
			
			}
		}
		
		if( status != trans_status ) {
			pre_trans_status = trans_status;
			status_change_time_ = NOW;
			trans_status = status;
		}
		trans_status = status;
	}
	*/
	
	inline void SetCarrierSense(bool f){
		carrier_sense=f;
		carrier_id=f;
	}
	inline enum TransmissionStatus TransmissionStatus(){return trans_status;}
	inline bool CarrierSense(){return carrier_sense;}
	inline bool CarrierId(){return carrier_id;}
	inline void ResetCarrierSense(){ carrier_sense=false;}
	inline void ResetCarrierId(){carrier_id=false;}
	
	inline enum TransmissionStatus PreTransStatus() { return pre_trans_status; }
	inline double& status_change_time() { return status_change_time(); }

	double CX_;
	double CY_;
	double CZ_;
	int sinkStatus_;
	int failure_status_;// 1 if node fails, 0 otherwise
	int setHopStatus; // used by test-rmac.tcl. add by peng xie
	int next_hop;// used by test-rmac.tcl. add by peng xie

	enum  TransmissionStatus trans_status;
	enum  TransmissionStatus pre_trans_status; //the previous status
	double status_change_time_;  //the time when changing pre_trans_status to trans_status
	
	//add by peng xie, 1  indicates the node is in failure state, 0 normal state
	bool 	carrier_sense;
	bool 	carrier_id;
	double 	failure_pro_;
	// add by peng xie to indicate the error probability of receiving packets
	//   int destination_status; 
	double 	failure_status_pro_;
	// add by peng xie& zheng  to indicate the probability to set the failure status of this node
	//   int destination_status; 
	UnderwaterPositionHandler uw_pos_handle_;
	Event uw_pos_intr_; 

protected:

	double max_speed;
	double min_speed;
	void random_speed();
	void random_destination();
	void generateFailure();
	UWMobilityPattern * UWMP_;	//added by Yibo to implement mobility model
	void bindMobilePattern(const char* PatternName);

private:
	inline int initialized(){
		return (T_ &&
			X_ >= T_->lowerX() && X_ <= T_->upperX() &&
			Y_ >= T_->lowerY() && Y_ <= T_->upperY() &&
			Z_ >= T_->lowerZ() && Z_ <= T_->upperZ() );  //Z_ is added by Yibo

	}
	void	random_position();
	void	bound_position();
	int		random_motion_;	// is mobile
	double  max_thought_time_;  //for underwater random waypoint model
	int soundspeed_;

	Topography* T_;

};

#endif // ns_underwatersensornode_h






