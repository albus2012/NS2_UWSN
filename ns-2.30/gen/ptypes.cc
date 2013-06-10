static const char code[] = "\n\
global ptype pvals\n\
set ptype(error) -1\n\
set pvals(-1) error\n\
set ptype(tcp) 0\n\
set pvals(0) tcp\n\
set ptype(udp) 1\n\
set pvals(1) udp\n\
set ptype(cbr) 2\n\
set pvals(2) cbr\n\
set ptype(audio) 3\n\
set pvals(3) audio\n\
set ptype(video) 4\n\
set pvals(4) video\n\
set ptype(ack) 5\n\
set pvals(5) ack\n\
set ptype(start) 6\n\
set pvals(6) start\n\
set ptype(stop) 7\n\
set pvals(7) stop\n\
set ptype(prune) 8\n\
set pvals(8) prune\n\
set ptype(graft) 9\n\
set pvals(9) graft\n\
set ptype(graftack) 10\n\
set pvals(10) graftAck\n\
set ptype(join) 11\n\
set pvals(11) join\n\
set ptype(assert) 12\n\
set pvals(12) assert\n\
set ptype(message) 13\n\
set pvals(13) message\n\
set ptype(rtcp) 14\n\
set pvals(14) rtcp\n\
set ptype(rtp) 15\n\
set pvals(15) rtp\n\
set ptype(rtprotodv) 16\n\
set pvals(16) rtProtoDV\n\
set ptype(ctrmcast_encap) 17\n\
set pvals(17) CtrMcast_Encap\n\
set ptype(ctrmcast_decap) 18\n\
set pvals(18) CtrMcast_Decap\n\
set ptype(srm) 19\n\
set pvals(19) SRM\n\
set ptype(sa_req) 20\n\
set pvals(20) sa_req\n\
set ptype(sa_accept) 21\n\
set pvals(21) sa_accept\n\
set ptype(sa_conf) 22\n\
set pvals(22) sa_conf\n\
set ptype(sa_teardown) 23\n\
set pvals(23) sa_teardown\n\
set ptype(live) 24\n\
set pvals(24) live\n\
set ptype(sa_reject) 25\n\
set pvals(25) sa_reject\n\
set ptype(telnet) 26\n\
set pvals(26) telnet\n\
set ptype(ftp) 27\n\
set pvals(27) ftp\n\
set ptype(pareto) 28\n\
set pvals(28) pareto\n\
set ptype(exp) 29\n\
set pvals(29) exp\n\
set ptype(httpinval) 30\n\
set pvals(30) httpInval\n\
set ptype(http) 31\n\
set pvals(31) http\n\
set ptype(encap) 32\n\
set pvals(32) encap\n\
set ptype(mftp) 33\n\
set pvals(33) mftp\n\
set ptype(arp) 34\n\
set pvals(34) ARP\n\
set ptype(mac) 35\n\
set pvals(35) MAC\n\
set ptype(tora) 36\n\
set pvals(36) TORA\n\
set ptype(dsr) 37\n\
set pvals(37) DSR\n\
set ptype(aodv) 38\n\
set pvals(38) AODV\n\
set ptype(imep) 39\n\
set pvals(39) IMEP\n\
set ptype(rap_data) 40\n\
set pvals(40) rap_data\n\
set ptype(rap_ack) 41\n\
set pvals(41) rap_ack\n\
set ptype(tcpfriend) 42\n\
set pvals(42) tcpFriend\n\
set ptype(tcpfriendctl) 43\n\
set pvals(43) tcpFriendCtl\n\
set ptype(ping) 44\n\
set pvals(44) ping\n\
set ptype(diffusion) 45\n\
set pvals(45) diffusion\n\
set ptype(rtprotols) 46\n\
set pvals(46) rtProtoLS\n\
set ptype(ldp) 47\n\
set pvals(47) LDP\n\
set ptype(gaf) 48\n\
set pvals(48) gaf\n\
set ptype(ra) 49\n\
set pvals(49) ra\n\
set ptype(pushback) 50\n\
set pvals(50) pushback\n\
set ptype(pgm) 51\n\
set pvals(51) PGM\n\
set ptype(lms) 52\n\
set pvals(52) LMS\n\
set ptype(lms_setup) 53\n\
set pvals(53) LMS_SETUP\n\
set ptype(sctp) 54\n\
set pvals(54) sctp\n\
set ptype(sctp_app1) 55\n\
set pvals(55) sctp_app1\n\
set ptype(smac) 56\n\
set pvals(56) smac\n\
set ptype(xcp) 57\n\
set pvals(57) xcp\n\
set ptype(hdlc) 58\n\
set pvals(58) HDLC\n\
set ptype(belllabstrace) 59\n\
set pvals(59) BellLabsTrace\n\
set ptype(vectorbasedforward) 60\n\
set pvals(60) vectorbasedforward\n\
set ptype(vectorbasedvoidavoidance) 61\n\
set pvals(61) vectorbasedvoidavoidance\n\
set ptype(underwaterrmac) 62\n\
set pvals(62) UnderwaterRmac\n\
set ptype(underwatertmac) 63\n\
set pvals(63) UnderwaterTmac\n\
set ptype(dbr) 64\n\
set pvals(64) dbr\n\
set ptype(auv-sync) 65\n\
set pvals(65) auv-SYNC\n\
set ptype(auv-missinglist) 66\n\
set pvals(66) auv-missinglist\n\
set ptype(auv-hello) 67\n\
set pvals(67) auv-hello\n\
set ptype(uwan-sync) 68\n\
set pvals(68) uwan-SYNC\n\
set ptype(uwan-missinglist) 69\n\
set pvals(69) uwan-missinglist\n\
set ptype(uwan-hello) 70\n\
set pvals(70) uwan-hello\n\
set ptype(otman) 71\n\
set pvals(71) OTMAN\n\
set ptype(fama) 72\n\
set pvals(72) FAMA\n\
set ptype(slotted-fama) 73\n\
set pvals(73) Slotted-FAMA\n\
set ptype(uw-staticrouting) 74\n\
set pvals(74) UW-StaticRouting\n\
set ptype(uw-message) 75\n\
set pvals(75) UW-Message\n\
set ptype(uw-aloha-ack) 76\n\
set pvals(76) UW-Aloha-ACK\n\
set ptype(uw_drouting) 77\n\
set pvals(77) uw_drouting\n\
proc ptype2val {str} {\n\
global ptype\n\
set str [string tolower $str]\n\
if ![info exists ptype($str)] {\n\
set str error\n\
}\n\
set ptype($str)\n\
}\n\
proc pval2type {val} {\n\
global pvals\n\
if ![info exists pvals($val)] {\n\
set val -1\n\
}\n\
set pvals($val)\n\
}\n\
";
#include "config.h"
EmbeddedTcl et_ns_ptypes(code);
