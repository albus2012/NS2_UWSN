/*
This program is the modified version of phy.h, it supports the periodic operation of sensor nodes---modified by xp at 2007

*/

#ifndef ns_underwaterphy_h
#define ns_underwaterphy_h

#include "propagation.h"
#include "underwaterpropagation.h"
#include "modulation.h"
#include "phy.h"
#include "underwatersensor/uw_common/underwatersensornode.h"
#include "timer-handler.h"

class Phy;
class UnderwaterPropagation;
class UnderwaterPhy;





class Underwater_Idle_Timer : public TimerHandler {
public:
	Underwater_Idle_Timer(UnderwaterPhy *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	UnderwaterPhy *a_;
};

class UnderwaterPhy : public Phy {
public:
	UnderwaterPhy();

	void sendDown(Packet *p);
	int sendUp(Packet *p);

	inline double getEnergyspread(){ return K_;} 
	inline double getFrequency(){return freq_;}   
	inline double getL() const {return L_;}
	//inline double getLambda() const {return lambda_;}
	inline Node* node(void) const { return node_; }
	inline double getPtconsume() { return Pt_consume_; }
	// inline PhyStatus getPhyStatus(){return status_;}
	virtual int command(int argc, const char*const* argv);
	virtual void dump(void) const;

	void ResetSenseStatus();
	void power_on();
	void power_off();
	void status_shift(double);

	/* -NEW- */
	inline double getAntennaZ() { return ant_->getZ(); }
	inline double getPt() { return Pt_; }
	inline double getRXThresh() { return RXThresh_; }
	inline double getCSThresh() { return CSThresh_; }
	/* End -NEW- */

	inline double getLambda() { return lambda_; }
	inline Antenna* getAntenna() { return ant_; }
	inline void txtime();
	
	inline double sync_hdr_len() { return sync_hdr_len_; }
	inline double forwarding_delay() { return fowarding_delay_; }

	void initTransitTime() {
		int i, j;
		for( i=0; i<NStatus; i++ ) 
			for( j=0; j<NStatus; j++ ) 
				TransitTime[i][j] = 0.0;
	}
	
	inline double getTransitTime(enum TransmissionStatus current_status, 
							   enum TransmissionStatus new_status)		{
		return TransitTime[current_status][new_status];	  
	}
	
	int DefaultBitError(int pktsize);
	
protected:
	double Pt_;		// transmitted signal power (W)
	double Pt_consume_;	// power consumption for transmission (W)
	double Pr_consume_;	// power consumption for reception (W)
	double P_idle_;         // idle power consumption (W)
	//double last_send_time_;	// the last time the node sends somthing.
	//double channel_idle_time_;	// channel idle time.
	double update_energy_time_;	// the last time we update energy.

	double freq_;           // frequency
	double K_;              // energy spread factor
	double lambda_;		// wavelength (m), we don't use it anymore
	double L_;		// system loss default factor

	double RXThresh_;	// receive power threshold (W)
	double CSThresh_;	// carrier sense threshold (W)
	double CPThresh_;	// capture threshold (db)
	
	//added by Yibo Zhu to simulation the syncronization header of modems
	//the txtime of the syncronization header of the packet (sec)
	//default value: 0
	double sync_hdr_len_;		//in seconds
	double fowarding_delay_;
	//transition delays


	Antenna *ant_;
	// we don't use it anymore, however we need it as an arguments
	Propagation *propagation_;	// Propagation Model
	Modulation *modulation_;	// Modulation Schem


	double TransitTime[NStatus][NStatus];  //the transition time between 

	Underwater_Idle_Timer idle_timer_;

	//Added by Yibo.
	double Energy_turn_on_;	//energy consumption for turning on the modem (J)
	double Energy_turn_off_; //energy consumption for turning off the modem (J)
	
	double bit_error_rate_;  //default value: 0.0

	// SenseHandler sense_handler;

	// Event sense_event;
	//UnderwaterPhy_Status_Timer status_timer;
	// Event  status_update;  	
	//	PhyStatus status_;
//private:    //to make sure its sub-class use the member of this class
			  //sufficiently, I change private to protected
protected:
	inline int initialized() {
		return (node_ && uptarget_ && downtarget_ && propagation_);
	}
	void UpdateIdleEnergy();
	int power_status;  //1: power on 0:power off
	// void ResetStatus();
	// Convenience method
	EnergyModel* em() { return node()->energy_model(); }

	friend class Underwater_Idle_Timer;
	//        friend class SenseHandler;
	// friend class UnderwaterPhy_Status_Timer;
};

#endif /* !ns_UnderwaterPhy_h */

