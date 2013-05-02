#include "mobility_pattern_allinone.h"


UW_Kinematic::UW_Kinematic(UnderwaterSensorNode* n):UWMobilityPattern(n)
{
	k1 = Random::normal(PI, 0.1*PI);
	k2 = Random::normal(PI, 0.1*PI);
	k3 = Random::normal(2*PI, 0.2*PI);
	k4 = Random::normal(0, 0.2);
	k5 = Random::normal(0, 0.2);
	lambda = Random::normal(6, 0.3);
	v = Random::normal(1, 0.1);
}



void UW_Kinematic::update_position()
{

	double now = Scheduler::instance().clock();
	double interval = now - n_->position_update_time_;
	double oldX = n_->X_, oldY = n_->Y_;
	double delta_x = 0.0;
	double delta_y = 0.0;
	double v_x = 0.0;
	double v_y = 0.0;


	//update the node position
	n_->X_ = NextX_;
	n_->Y_ = NextY_;
	n_->Z_ = NextZ_;
	bound_position();
	NextX_ = n_->X_;
	NextY_ = n_->Y_;
	NextZ_ = n_->Z_;
	
	if ((interval == 0.0)&&(n_->position_update_time_!=0))
		return;         // ^^^ for list-based imprvmnt 

    //start to calculate the position after the coming interval
	v_y = k5-lambda*v*cos(k2*n_->X_)*sin(k3*n_->Y_);
	v_x = k1*lambda*v*sin(k2*n_->X_)*cos(k3*n_->Y_)+k4+k1*lambda*cos(2*k1*now);	
	
	NextX_ += v_x*interval;
	NextY_ += v_y*interval;
	
	if(oldX != n_->X_)
		n_->T_->updateNodesList(n_, oldX); //X_ is the key value of SortList

    
	n_->position_update_time_ = now;
	
	
	updateGridKeeper();
	NamTrace("n -t %f -s %d -x %f -y %f -z %f -U %f -V %f -T %f",
			now,
			n_->nodeid_,
			n_->X_, n_->Y_, n_->Z_,
			v_x,
			v_y, interval);
}

