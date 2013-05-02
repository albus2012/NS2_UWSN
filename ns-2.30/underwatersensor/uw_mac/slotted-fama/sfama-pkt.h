#ifndef __sfama_pkt_h__
#define __sfama_pkt_h__

#include <packet.h>

struct hdr_SFAMA{
	//nsaddr_t SA;
	//nsaddr_t DA;

	u_int16_t	SlotNum;  //the number of slots required for transmitting the DATA packet

	enum PacketType {
		SFAMA_RTS,
		SFAMA_CTS,
		SFAMA_DATA,
		SFAMA_ACK
	} packet_type;

	static int getSize(enum PacketType p_type) {

		int pkt_size = 2*sizeof(nsaddr_t); //source and destination addr in hdr_mac

		if( p_type == SFAMA_RTS || p_type == SFAMA_CTS ) {
			pkt_size += sizeof(u_int16_t)+1; //size of packet_type and slotnum
		}
		
		return pkt_size;
	}

	static int offset_;
	inline static int& offset() {  return offset_; }

	inline static hdr_SFAMA* access(const Packet*  p) {
		return (hdr_SFAMA*) p->access(offset_);
	}

};


#endif
