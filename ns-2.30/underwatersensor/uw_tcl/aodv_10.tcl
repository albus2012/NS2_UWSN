#==============================================
#Define options
#==============================================
set val(chan)    Channel/WirelessChannel;
set val(prop)    Propagation/TwoRayGround;
set val(netif)   Phy/WirelessPhy;
set val(mac)     Mac/802_11;
set val(ifq)     Queue/DropTail/PriQueue;
set val(ll)      LL;
set val(ant)     Antenna/OmniAntenna;
set val(ifqlen)  50;
set val(nn)      3;
set val(rp)      AODV;
set val(x)       1000;
set val(y)       1000;
set val(stoptime) 300;

#===============================
#Main Program
#===============================

#��ʼ��ģ�����͸��ٶ���
set ns_ [new Simulator]
set tracefd [open result/out_100.tr w]

#�¸�ʽ
$ns_ use-newtrace
$ns_ trace-all $tracefd

set namtracefd [open result/out_100.nam w]
$ns_ namtrace-all-wireless $namtracefd $val(x) $val(y)
proc finish {} {
     global ns_ tracefd namtracefd
     $ns_ flush-trace
     close $tracefd
     close $namtracefd
     exec nam out_10.nam &
     exit 0 
     }

#�����ƶ�����
set topo [new Topography]

#�����ƶ�����
$topo load_flatgrid $val(x) $val(y)
set god_ [create-god $val(nn)]

set chan_1_ [new $val(chan)]
#���ýڵ�����
$ns_ node-config -adhocRouting  AODV\
            -llType $val(ll)\
            -macType $val(mac)\
            -ifqType $val(ifq)\
            -ifqLen $val(ifqlen)\
            -antType $val(ant)\
            -propType $val(prop)\
            -phyType $val(netif)\
            -channel $chan_1_\
            -topoInstance $topo\
            -agentTrace ON\
            -routerTrace ON\
            -macTrace OFF\
            -movementTrace OFF
            
              
            
#�����ƶ��ڵ�  
     
 for {set i 0} {$i < $val(nn) } {incr i} {
			set node_($i) [$ns_ node]
			$node_($i) set X_ 100+i 
			$node_($i) set Y_ 100+20*i
			$node_($i) set Z_ 0
		}
#$node_(1) setOFF		

$ns_ at 4.5768388786897245 "$node_(2) setOFF"

$ns_ at 12.6768388786897245 "$node_(2) setON"

#���س����ļ�
set udp_(0) [new Agent/UDP]
$ns_ attach-agent $node_(1) $udp_(0)
set null_(0) [new Agent/Null]
$ns_ attach-agent $node_(2) $null_(0)
set cbr_(0) [new Application/Traffic/CBR]
$cbr_(0) set packetSize_ 512
$cbr_(0) set interval_ 1
$cbr_(0) set random_ 1
$cbr_(0) set maxpkts_ 10000
$cbr_(0) attach-agent $udp_(0)
$ns_ connect $udp_(0) $null_(0)
$ns_ at 2.5568388786897245 "$cbr_(0) start"
 
#ģ����������ڵ�
#$ns_ at $val(stoptime) "$node_(2) NodeOn"
for {set i 0} {$i < $val(nn) } {incr i} {
	$ns_ at $val(stoptime) "$node_($i) reset";
}
proc finish {} {
	global ns nf nd
	$ns flush-trace
	close $nf
	close $nd 
	exec nam result/out_100.nam &
	exit 0
}
$ns_ at $val(stoptime) "$ns_ nam-end-wireless $val(stoptime)"
$ns_ at $val(stoptime) "$ns_ halt"
$ns_ run
