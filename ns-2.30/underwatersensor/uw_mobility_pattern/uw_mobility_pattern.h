/*
 * the base class for mobility model for 3D underwatersensor network. 
 * Contributed by Yibo Zhu, email: yibo.zhu@engr.uconn.edu
 * http://uwsn.engr.uconn.edu
 */
#ifndef __uw_mobility_pattern_h__
#define __uw_mobility_pattern_h__

#include <string.h>
#include <timer-handler.h>
#include <math.h>
#include <random.h>


#define UWMobilePatternFriendshipDeclaration \
	friend class UW_Kinematic;


//#include "uw_node_and_mobility_pattern.h"
//#include "underwatersensornode.h"


#ifndef PI
#define PI 3.1415926
#endif
#define INTEGRAL_INTERVAL 0.001


class UnderwaterSensorNode;

//uw mobility pattern type
enum MobilityPatternType {
	MPT_KINEMATIC,
	MPT_RWP,
	MPT_NTYPE	
};

//name of uw mobility pattern
class mpt_info {

private:
	static const char* name_[MPT_NTYPE+1];

public:
	mpt_info(){
		name_[MPT_KINEMATIC]="kinematic";
		name_[MPT_RWP]="RWP";
		
		name_[MPT_NTYPE]="undefined";
	}
	
	const char* name(MobilityPatternType p) const { 
		if ( p <= MPT_NTYPE ) 
			return name_[p];
		else
	 		return NULL;
	}

	MobilityPatternType getTypeByName(const char* PatternName){
		for( int i=0; i < (MPT_NTYPE); i++) {
			if( strcmp(PatternName, name_[i]) == 0 )
				return (MobilityPatternType)i;
		}
		return MPT_NTYPE;
	}
		
};

extern mpt_info mpt_names;


//call the UWMobilePattern::updateCoordinate() in UnderwaterPositionHandler::handle()

//impelement any mobility Pattern of UW as a child class of UWMobilityPattern
class UWMobilityPattern{
public:
	UWMobilityPattern(UnderwaterSensorNode* n);	
	virtual void update_position() {  return; }
	virtual void init() { return; };
	void NamTrace(char* format, ...);

protected:	
	void updateGridKeeper();
	void bound_position();
	

protected:
	UnderwaterSensorNode* n_;
	//pre-calculate the position after coming interval so as to record movement into the nam file
	double NextX_;
	double NextY_;
	double NextZ_;
	
};

	


#endif



