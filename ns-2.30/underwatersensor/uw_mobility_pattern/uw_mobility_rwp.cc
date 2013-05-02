#include "mobility_pattern_allinone.h"


UW_RWP::UW_RWP(UnderwaterSensorNode* n):UWMobilityPattern(n)
{
}

void UW_RWP::init()
{
	destX_ = n_->X_ = Random::uniform(n_->T_->lowerX(), n_->T_->upperX());
	destY_ = n_->Y_ = Random::uniform(n_->T_->lowerY(), n_->T_->upperY());
	destZ_ = n_->Z_ = Random::uniform(n_->T_->lowerZ(), n_->T_->upperZ());
	prepareNextPoint();
	start_time_ = Scheduler::instance().clock();
	DuplicatedNamTrace = false;
}

void UW_RWP::prepareNextPoint()
{
	speed_ = (Random::uniform() * (n_->max_speed-n_->min_speed))+n_->min_speed;

	originalX_ = destX_;
	originalY_ = destY_;
	originalZ_ = destZ_;
	//calculate the next way point
	destX_ = Random::uniform(n_->T_->lowerX(), n_->T_->upperX());
	destY_ = Random::uniform(n_->T_->lowerY(), n_->T_->upperY());
	destZ_ = Random::uniform(n_->T_->lowerZ(), n_->T_->upperZ());

	distance_ = sqrt( (destX_ - originalX_)*(destX_ - originalX_)
		+(destY_ - originalY_)*(destY_ - originalY_)
		+(destZ_ - originalZ_)*(destZ_ - originalZ_) );

	ratioX_ = (destX_ - originalX_)/distance_;
	ratioY_ = (destY_ - originalY_)/distance_;
	ratioZ_ = (destZ_ - originalZ_)/distance_;	

	thought_time_ = Random::uniform(0, n_->max_thought_time_);
}


void UW_RWP::update_position()
{
	double now = Scheduler::instance().clock();
	//the time since node start to move from previous way point
	double passed_len = speed_*(now - start_time_);
	double oldX = n_->X_;

	if( passed_len < distance_ )
	{
		n_->X_ = originalX_ + passed_len * ratioX_;
		n_->Y_ = originalY_ + passed_len * ratioY_;
		n_->Z_ = originalZ_ + passed_len * ratioZ_;
	}
	else{
		//now I must arrive at the way point
		
		if( now - start_time_ - distance_/speed_ < thought_time_ )
		{
			//I am still thinking of that where I will go
			n_->X_ = destX_;
			n_->Y_ = destY_;
			n_->Z_ = destZ_;
		}
		else{
			//I am on the way to next way point again.
			start_time_ = start_time_ + distance_/speed_ + thought_time_;
			prepareNextPoint();		
			DuplicatedNamTrace = true;
			update_position();
			DuplicatedNamTrace = false;			
		}
	}

	if(oldX != n_->X_)
		n_->T_->updateNodesList(n_, oldX); //X_ is the key value of SortList
 
	n_->position_update_time_ = now;	
	
	updateGridKeeper();
	/*current nam does not support 3D, so we just write x, y velocity.
	 *After using our animator, z velocity will also be traced.
	 */
	if( !DuplicatedNamTrace )
	{
		NamTrace("n -t %f -s %d -x %f -y %f -z %f -U %f -V %f -T %f",
			now,
			n_->nodeid_,
			n_->X_, n_->Y_, n_->Z_,
			speed_*ratioX_, speed_*ratioY_,
			now - n_->position_update_time_);

		DuplicatedNamTrace = false;
	}
}
