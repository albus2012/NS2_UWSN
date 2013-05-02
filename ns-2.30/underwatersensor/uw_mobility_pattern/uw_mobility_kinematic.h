/*
 * random waypoint model for 3D underwatersensor network. 
 * Contributed by Yibo Zhu, email: yibo.zhu@engr.uconn.edu
 * http://uwsn.engr.uconn.edu
 */

#ifndef __UW_MOBILITY_KINEMATIC_H__
#define __UW_MOBILITY_KINEMATIC_H__

#include "uw_mobility_pattern.h"


class UW_Kinematic: public UWMobilityPattern{
public:
	UW_Kinematic(UnderwaterSensorNode* n);
	virtual void update_position();

private:
	double k1;
	double k2;
	double k3;
	double k4;
	double k5;
	double lambda;
	double v;	
};



#endif
