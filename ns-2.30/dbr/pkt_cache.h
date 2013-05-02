#ifndef	_PKT_CACHE_H_
#define	_PKT_CACHE_H_

#include <packet.h>

typedef	int	PacketID;

class PktCache
{
public:
	PktCache();
	~PktCache();

	int& size(void)
	{ return size_; }

	int accessPacket(PacketID p);
	void addPacket(PacketID p);
	void deletePacket(PacketID p);
	void dump(void);

private:
	PacketID *pcache_;			// packet cache
	int	size_;					// cache size
	int max_size_;				// max cache size
};

#endif

