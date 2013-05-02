#include <stdio.h>
#include "pkt_cache.h"

// Constructor
PktCache::
PktCache()
{
	int i;

	max_size_ = 1500;
	size_ = 0;

	pcache_ = new PacketID [1500];

	//for (i = 0; i < 100; i++)
	//	pcache_[i] = new Packet;
}

PktCache::
~PktCache()
{
	int i;

	//for (i = 0; i < max_size_; i++)
	//	delete pcache_[i];

	delete[] pcache_;
}

int
PktCache::
accessPacket(PacketID p)
{
	int i, j;
	PacketID tmp;
	
	for (i = 0; i < size_; i++) {
		if (pcache_[i] == p) {
			// if the pkt is existing
			// put it to the tail 
			tmp = p;
			for (j = i; j < size_ - 1; j++)
				pcache_[j] = pcache_[j+1];
			pcache_[size_-1] = tmp;

			return 1;
		}
	}

	return 0;
}

void
PktCache::
addPacket(PacketID p)
{
	if (size_ == max_size_) {
		printf("Cache is full!\n");
		return;
	}

	pcache_[size_] = p;
	size_++;

	return;
}

void
PktCache::
deletePacket(PacketID p)
{
}

void
PktCache::
dump(void)
{
	int i;

	for (i = 0; i < size_; i++)
		fprintf(stderr, "[%d]: %d\n", i, pcache_[i]);
}

#if 0
// test main function
int main(void)
{
	PktCache pc;
	Packet p1, p2, p3;

	pc.addPacket(&p1);
	pc.addPacket(&p2);
	pc.addPacket(&p3);

	pc.dump();

	pc.accessPacket(&p1);
	pc.dump();

	return 0;
}
#endif
