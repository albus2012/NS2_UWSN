#include <deque>

class PacketQueue {
public:
	PacketQueue();
	~PacketQueue();

	bool empty() { return dq_.empty(); }
	int size() { return dq_.size(); }

	void pop() { dq_.pop_front(); ); 
	Packet* front() { return dq_.front(); };
	void push(Packet* p);

private:
	deque<Packet*> dq_;
}

