#include "dmac_base.h"
#include <stdio.h>
#include "iostream"
using namespace std;


//DMac_WakeTimer::DMac_WakeTimer(DMac* mac,ScheTime* ScheT):TimerHandler()
//{
//  mac_ = mac;
//  ScheT_ = ScheT;
//}



void ScheQueue::push(Time SendTime,
                     nsaddr_t node_id,
                     Time Interval)
{
  ScheTime* newElem = new ScheTime(SendTime, node_id, mac_);
  newElem->start(Interval);

  ScheTime* pos = Head_->next_;
  ScheTime* pre_pos = Head_;

  //find the position where new element should be insert
  while( pos != NULL ) {
    if( pos->SendTime_ > SendTime ) {
      break;
    }
    else {
      pos = pos->next_;
      pre_pos = pre_pos->next_;
    }

  }
  /*
   * insert new element after pre_pos
   */
  newElem->next_ = pos;
  pre_pos->next_ = newElem;
}


//get the top element, but not pop it

ScheTime* ScheQueue::top()
{
  return Head_->next_;
}

//pop the top element
void ScheQueue::pop()
{
  if( Head_->next_ != NULL ) {

    ScheTime* tmp = Head_->next_;
    Head_->next_ = Head_->next_->next_;

    if( tmp->timer_.isPending())//TimerHandler::TIMER_PENDING ) {
    {
      tmp->timer_.cancel();
    }

    delete tmp;
  }
}


bool ScheQueue::checkGuardTime(Time SendTime,
                               Time GuardTime,
                               Time MaxTxTime)
{
  ScheTime* pos = Head_->next_;
  ScheTime* pre_pos = Head_;

  while( pos != NULL && SendTime > pos->SendTime_ ) {
    pos = pos->next_;
    pre_pos = pre_pos->next_;
  }

  /*now, pos->SendTime > SendTime > pre_pos->SendTime
   *start to check the sendtime.
   */
  if( pos == NULL ) {
    if( pre_pos == Head_ )
      return true;
    else {
      if( SendTime - pre_pos->SendTime_ >= GuardTime )
        return true;
      else
        return false;
    }
  }
  else {
    if( pre_pos == Head_ ) {
      if( (pos->SendTime_ - SendTime) > (GuardTime + MaxTxTime) )
        return true;
      else
        return false;
    }
    else {
      if( ((pos->SendTime_ - SendTime) > (GuardTime + MaxTxTime))
        && (SendTime - pre_pos->SendTime_ >= GuardTime) )
        return true;
      else
        return false;
    }
  }
}



Time ScheQueue::getAvailableSendTime(Time StartTime,
                           Time OriginalSchedule,
                           Time GuardTime,
                           Time MaxTxTime)
{
  ScheTime* pos = Head_->next_;
  ScheTime* pre_pos = Head_;

  Time DeltaTime = 0.0;
  while( pos != NULL && StartTime > pos->SendTime_ ) {
    pos = pos->next_;
    pre_pos = pre_pos->next_;
  }

  while( pos != NULL ) {
    DeltaTime = pos->SendTime_ - pre_pos->SendTime_
            - (2*GuardTime + MaxTxTime);
    if( DeltaTime > 0 ) {
      return Random::uniform(pre_pos->SendTime_+GuardTime+MaxTxTime,
                       pre_pos->SendTime_+DeltaTime);
    }

    pos = pos->next_;
    pre_pos = pre_pos->next_;
  }

  //there is no available interval, so the time out of range of this queue is returned.
  /*
   * Before calling this function, OriginalSchedule collides with other times,
   * so originalSchedule is at most pre_pos->SendTime_ + GuardTime + MaxTxTime.
   * Otherwise, it cannot collides.
   */
  return pre_pos->SendTime_ + MaxTxTime + GuardTime ;

}


void ScheQueue::clearExpired(Time CurTime)
{
  ScheTime* NextSch = NULL;
  while( (NextSch = top()) && NextSch->SendTime_ < CurTime ) {
    pop();
  }
}


void ScheQueue::print(Time GuardTime,
                      Time MaxTxTime,
                      bool IsMe,
                      nsaddr_t index)
{
  ScheTime* pos = Head_->next_;
  char file_name[30];
  strcpy(file_name, "schedule_");
  file_name[strlen(file_name)+1] = '\0';
  file_name[strlen(file_name)] = char(index+'0');
  FILE* stream = fopen(file_name, "a");
  if( IsMe )
      fprintf(stream, "I send  ");
  while( pos != NULL ) {
    fprintf(stream, "(%f--%f, %f) ", pos->SendTime_,
      pos->SendTime_+MaxTxTime, pos->SendTime_+GuardTime+MaxTxTime);
    pos = pos->next_;
  }
  fprintf(stream, "\n");
  fclose(stream);
}
