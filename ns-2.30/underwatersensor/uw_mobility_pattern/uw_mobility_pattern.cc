#include "mobility_pattern_allinone.h"



mpt_info mpt_names;
const char* mpt_info::name_[MPT_NTYPE+1];



UWMobilityPattern::UWMobilityPattern(UnderwaterSensorNode* n)
{ 
	n_ = n;
	NextX_ = n_->X_;
	NextY_ = n_->Y_;
	NextZ_ = n_->Z_;
}



void UWMobilityPattern::NamTrace(char* fmt, ...)
{
	va_list ap;

	if (n_->namChan_ == 0)
    	return;

	va_start(ap, fmt);
  	vsprintf(n_->nwrk_, fmt, ap);
  	n_->namdump();
  	va_end (ap);
}


void UWMobilityPattern::updateGridKeeper()
{
	if (GridKeeper::instance()){
		GridKeeper* gp =  GridKeeper::instance();
		gp-> new_moves(n_);
	} 
}


void UWMobilityPattern::bound_position()
{
	double minX;
	double maxX;
	double minY;
	double maxY;
	double minZ;
	double maxZ;
	int recheck = 1;


	minX = n_->T_->lowerX();
	maxX = n_->T_->upperX();
	minY = n_->T_->lowerY();
	maxY = n_->T_->upperY();
	minZ = n_->T_->lowerZ();
	maxZ = n_->T_->upperZ();

	while (recheck) {
		recheck = 0;
		if (n_->X_ < minX) {
			n_->X_ = minX + (minX - n_->X_);
			recheck = 1;
		}
		if (n_->X_ > maxX) {
			n_->X_ = maxX - (n_->X_ - maxX);
			recheck = 1;
		}
		if (n_->Y_ < minY) {
			n_->Y_ = minY + (minY - n_->Y_);
			recheck = 1;
		}
		if (n_->Y_ > maxY) {
			n_->Y_ = maxY- (n_->Y_ - maxY);
			recheck = 1;
		}
		if (n_->Z_ < minZ) {
			n_->Z_ = minZ + (minZ - n_->Z_);
			recheck = 1;
		}
		if (n_->Z_ > maxZ) {
			n_->Z_ = maxZ - (n_->Z_ - maxZ);
			recheck = 1;
		}
		if (recheck) {
			fprintf(stderr, "Adjust position of node %d\n",n_->address_);
		}
	}
}
