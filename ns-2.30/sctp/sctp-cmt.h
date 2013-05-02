/*
 * Copyright (c) 2001-2006 by the Protocol Engineering Lab, U of Delaware
 * All rights reserved.
 *
 * Janardhan R. Iyengar <iyengar@@cis,udel,edu>
 * Preethi Natarajan <nataraja@@cis,udel,edu>
 * Keyur Shah <shah@@cis,udel,edu>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Concurrent Multipath Transfer extension: To allow SCTP to correctly use
 * multiple paths available at the transport.
 */

#ifndef ns_sctp_cmt_h
#define ns_sctp_cmt_h


#include "agent.h"
#include "node.h"
#include "packet.h"
#include "sctp.h"

/* CMT rtx policies
 */
typedef enum CmtRtxPolicy_E
{
  RTX_ASAP,
  RTX_TO_SAME,
  RTX_SSTHRESH,
  RTX_LOSSRATE,
  RTX_CWND
};

class SctpCMTAgent : public virtual SctpAgent 
{
public:

  SctpCMTAgent();
  void Timeout(SctpChunkType_E, SctpDest_S*);
  void SackGenTimerExpiration();
	
protected:

  void  delay_bind_init_all();
  int   delay_bind_dispatch(const char *varName, const char *localName,
				    TclObject *tracer);

  /* initialization stuff
   */
  void OptionReset();
  void Reset();
  
  /* tracing functions
   */
  void TraceAll();
  void TraceVar(const char*);

  /* sending functions
   */
  void      AddToSendBuffer(SctpDataChunkHdr_S *, int, u_int, SctpDest_S *);
  int       GenChunk(SctpChunkType_E, u_char *);
  void      SendBufferDequeueUpTo(u_int);
  void      FastRtx();
  Boolean_E AnyMarkedChunks();
  void      MarkChunkForRtx(SctpSendBufferNode_S *, MarkedForRtx_E);
  void      RtxMarkedChunks(SctpRtxLimit_E);
  void      recv(Packet *pkt, Handler*);
  void      SendMuch();
  
  /* processing functions
   */
  Boolean_E   ProcessGapAckBlocks(u_char *, Boolean_E);
  void        ProcessSackChunk(u_char *);
  int         ProcessChunk(u_char *, u_char **);
  u_int       GetHighestOutstandingTsn(SctpDest_S *);
  void        HeartbeatGenTimerExpiration(double, SctpDest_S*);

  /* new CMT functions
   */
  SctpDest_S* SelectRtxDest(SctpSendBufferNode_S*, SctpRtxLimit_E);
  void        SetSharedCCParams(SctpDest_S*);
  char*       PrintDestStatus(SctpDest_S*);


  /******* Variables *******/
  
  /* JRI-TODO: Take out options for SFR and CUC algos.
   */
  Boolean_E eUseCmtReordering;  // use CMT's reordering algo? (was CACC)
  Boolean_E eUseCmtCwnd;        // use CMT's cwnd growth algo?
  Boolean_E eUseCmtDelAck;      // use CMT's delayed ack algo?

  /* Variables for CMT delayed ack algo
   */
  u_int uiNumPacketsSacked;    // number of packets acked by SACK (sack->flags)
  u_int uiNumDestsSacked;      // number of dests acked by SACK (inferred)

  CmtRtxPolicy_E eCmtRtxPolicy;

  /* CMT-PF: Use the possibly failed state in RTX_SSTHRESH policy?
   * If using CMT-PF, always set eOneHeartbeatTimer to false.
   * Using one HB timer for the whole association will not work since
   * each PF dest needs its own HB timer.
   */
  Boolean_E eUseCmtPF;  

  /* CMT-PF: Value for Cwnd when PF path becomes ACTIVE again.
   */
  u_int uiCmtPFCwnd;

  /* CMT-PF: Track last adv rwnd
   */
  u_int uiArwnd;
};

#endif
