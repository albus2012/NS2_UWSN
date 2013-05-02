/*
 * random waypoint model for 3D underwatersensor network. 
 * Contributed by Yibo Zhu, email: yibo.zhu@engr.uconn.edu
 * http://uwsn.engr.uconn.edu
 */
#ifndef __UW_MOBILITY_RWP_H__
#define __UW_MOBILITY_RWP_H__

#include "uw_mobility_pattern.h"



class UW_RWP: public UWMobilityPattern{
public:
	UW_RWP(UnderwaterSensorNode* n);
	virtual void update_position();
	virtual void init();

private:
	inline void prepareNextPoint();


private:
	double destX_;  //the coordinate of next way point
	double destY_;
	double destZ_;
	double originalX_;  //the coordinate of previous way point
	double originalY_;
	double originalZ_;

	/* the ratio between dimensions and the distance between 
	 * previous way point and next way point
	 */
	double ratioX_;    
	double ratioY_;
	double ratioZ_;
	
	double speed_; //for speed of the node

	double distance_;     //the distance to next point
	double start_time_;   //the time when this node start to next point
	double thought_time_; //the time taken by deceiding where I will go

	bool   DuplicatedNamTrace;

};



#endif
