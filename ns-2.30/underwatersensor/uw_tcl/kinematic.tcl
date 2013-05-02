#$0 pkt interval; $1 bandwidth; $2 pkt length of cbr;  $3 stop time; $4 number of hops
set opt(chan)			Channel/UnderwaterChannel
set opt(prop)			Propagation/UnderwaterPropagation
set opt(netif)			Phy/UnderwaterPhy
set opt(mac)			Mac/UnderwaterMac/BroadcastMac
set opt(ifq)			Queue/DropTail
set opt(ll)				LL
set opt(energy)         EnergyModel
set opt(txpower)        0.6
set opt(rxpower)        0.3
set opt(initialenergy)  10000
set opt(idlepower)      0.01
set opt(ant)            Antenna/OmniAntenna  ;#we don't use it in underwater
set opt(filters)        GradientFilter    ;# options can be one or more of 
                                ;# TPP/OPP/Gear/Rmst/SourceRoute/Log/TagFilter

set opt(max_pkts)		300
set opt(interval_)	          0.2 ;# [lindex $argv 0] 
set opt(pkt_len)	          80;  #[lindex $argv 2] ;# pkt length of cbr

# the following parameters are set fot protocols
set opt(bit_rate)                         5.0e3 ;#[lindex $argv 1];#1.0e4    ;#bandwidth of the phy link
set opt(encoding_efficiency)          1
set opt(ND_window)                    1
set opt(ACKND_window)                 1
set opt(PhaseOne_window)              3
set opt(PhaseTwo_window)              1
set opt(PhaseTwo_interval)            0.5
set opt(IntervalPhase2Phase3)         1 
set opt(duration)                     0.1
set opt(PhyOverhead)                  8 
set opt(large_packet_size)            480 ;# 60 bytes
set opt(short_packet_size)            40  ;# 5 bytes
set opt(PhaseOne_cycle)               4 ;
set opt(PhaseTwo_cycle)               2 ;
set opt(PeriodInterval)               2
set opt(transmission_time_error)      0.0001; 

set opt(dz)                           	10
set opt(hop)				       7 ;#	[lindex $argv 4]
set opt(ifqlen)		              50	;# max packet in ifq
set opt(nn)	                		[expr $opt(hop)+1] ;#5	;# number of nodes in the network
set opt(layers)                         	1
set opt(x)	                		300	;# X dimension of the topography
set opt(y)	                        	300  ;# Y dimension of the topography
set opt(z)                      		10
set opt(seed)	                		648.88
set opt(stop)	                		1000 ;#[lindex $argv 3] ;#150	;# simulation time
set opt(prestop)                       	80     ;# time to prepare to stop
set opt(tr)	                		"kinematic.tr"	;# trace file
set opt(nam)                            	"kinematic.nam"  ;# nam file
set opt(adhocRouting)                 	Vectorbasedforward 
set opt(width)                           	20
set opt(adj)                             	10
set opt(interval)                        	0.001

set start_time				10
# ==================================================================

LL set mindelay_		50us
LL set delay_			25us
LL set bandwidth_		0	;# not used

#Queue/DropTail/PriQueue set Prefer_Routing_Protocols    1

# unity gain, omni-directional antennas
# set up the antennas to be centered in the node and 1.5 meters above it
Antenna/OmniAntenna set X_ 0
Antenna/OmniAntenna set Y_ 0
Antenna/OmniAntenna set Z_ 1.5
Antenna/OmniAntenna set Z_ 0.05
Antenna/OmniAntenna set Gt_ 1.0
Antenna/OmniAntenna set Gr_ 1.0



Mac/UnderwaterMac set bit_rate_  $opt(bit_rate)
Mac/UnderwaterMac set encoding_efficiency_  $opt(encoding_efficiency)
#Mac/UnderwaterMac/AlohaOverhear set  MaxResendInterval_ 0.2
#Mac/UnderwaterMac/AlohaOverhear set  DeltaDelay_ 1


Node/MobileNode/UnderwaterSensorNode set position_update_interval_ 1.0

# Initialize the SharedMedia interface with parameters to make
# it work like the 914MHz Lucent WaveLAN DSSS radio interface
Phy/UnderwaterPhy set CPThresh_ 100  ;#10.0
Phy/UnderwaterPhy set CSThresh_ 0  ;#1.559e-11
Phy/UnderwaterPhy set RXThresh_ 0   ;#3.652e-10
#Phy/UnderwaterPhy set Rb_ 2*1e6
Phy/UnderwaterPhy set Pt_ 0.2818
Phy/UnderwaterPhy set freq_ 25  ;#frequency range in khz 
Phy/UnderwaterPhy set K_ 2.0   ;#spherical spreading

# ==================================================================
# Main Program
# =================================================================

#
# Initialize Global Variables
# 
#set sink_ 1


#remove-all-packet-headers 

set ns_ [new Simulator]
set topo  [new Topography]

$topo load_cubicgrid $opt(x) $opt(y) $opt(z)
#$ns_ use-newtrace
set tracefd	[open $opt(tr) w]
$ns_ trace-all $tracefd

set nf [open $opt(nam) w]
$ns_ namtrace-all-wireless $nf $opt(x) $opt(y)


set total_number [expr $opt(nn)-1]
set god_ [create-god $opt(nn)]

set chan_1_ [new $opt(chan)]


global defaultRNG

$defaultRNG seed $opt(seed)

$ns_ node-config -adhocRouting $opt(adhocRouting) \
		 -llType $opt(ll) \
		 -macType $opt(mac) \
		 -ifqType $opt(ifq) \
		 -ifqLen $opt(ifqlen) \
		 -antType $opt(ant) \
		 -propType $opt(prop) \
		 -phyType $opt(netif) \
		 #-channelType $opt(chan) \
		 -agentTrace ON \
         -routerTrace ON \
         -macTrace ON \
         -movementTrace ON \
         -topoInstance $topo\
         -energyModel $opt(energy)\
         -txpower $opt(txpower)\
         -rxpower $opt(rxpower)\
         -initialEnergy $opt(initialenergy)\
         -idlePower $opt(idlepower)\
         -channel $chan_1_


set node_(0) [$ns_  node 0]
#$node_(0) set sinkStatus_ 1
#$node_(0) set passive 1
    
$god_ new_node $node_(0)
$node_(0) set X_  5
$node_(0) set Y_  5
$node_(0) set Z_  0.0
$node_(0) set passive 1
set a_(0) [new Agent/Null]
$node_(0) set-mobilitypattern kinematic
$ns_ attach-agent $node_(0) $a_(0)



for {set i 1} {$i<$total_number} {incr i} {

set node_($i) [$ns_  node $i]
$node_($i) set sinkStatus_ 1
$god_ new_node $node_($i)
$node_($i) set X_  [expr $i*20]
$node_($i) set Y_  [expr $i*20]
$node_($i) set Z_  0.0
$node_($i) set-cx   50
$node_($i) set-cy   50
$node_($i) set-cz   0
$node_($i) set_next_hop [expr $i-1] ;# target is node 0 
$node_($i) set-mobilitypattern  kinematic

}



#puts "the total number is $total_number"
set node_($total_number) [$ns_  node $total_number]
$god_ new_node $node_($total_number)
$node_($total_number) set X_  [expr $total_number*20]
$node_($total_number) set Y_  [expr $total_number*20]
$node_($total_number) set Z_  0.0
$node_($total_number) set-cx  50
$node_($total_number) set-cy  50
$node_($total_number) set-cz  0
$node_($total_number) set_next_hop  [expr $total_number-1] ;# target is node 0 
$node_($total_number) set-mobilitypattern  kinematic

set a_($total_number) [new Agent/UDP]
$ns_ attach-agent $node_($total_number) $a_($total_number)
$ns_ connect $a_($total_number) $a_(0)

set cbr_(0) [new Application/Traffic/CBR]
$cbr_(0) set packetSize $opt(pkt_len)   ;#80
$cbr_(0) set interval_ $opt(interval_)
$cbr_(0) set random 1
$cbr_(0) set maxpkts_  $opt(max_pkts)
$cbr_(0) attach-agent $a_($total_number)



for {set i 0} { $i < $opt(nn)} {incr i} {
  $ns_ initial_node_pos $node_($i) 2
  $node_($i) setPositionUpdateInterval 0.01
  $node_($i) random-motion 0
  $ns_ at 5.0 "$node_($i) start-mobility-pattern"
}


$ns_ at $start_time "$cbr_(0) start"


#$ns_ at 15 "$a_($total_number) cbr-start"
#$ns_ at $start_time "$a_($total_number) exp-start"
#$ns_ at 4 "$a_(0) cbr-start"
#$ns_ at 2.0003 "$a_(2) cbr-start"
#$ns_ at 0.1 "$a_(0) announce"


puts "+++++++AFTER ANNOUNCE++++++++++++++"





;#$ns_ at $opt(stop).001 "$a_(0) terminate"


;#$ns_ at $opt(stop).002 "$a_($total_number) terminate"

for {set i 1} {$i<$total_number} {incr i} {
#;$ns_ at $opt(stop).002 "$a_($i) terminate"
	$ns_ at $opt(stop).002 "$node_($i) reset"
}

$ns_ at $opt(stop).003  "$god_ compute_energy"
$ns_ at $opt(stop).004  "$ns_ nam-end-wireless $opt(stop)"
$ns_ at $opt(stop).005 "puts \"NS EXISTING...\"; $ns_ halt"

 puts $tracefd "vectorbased"
 puts $tracefd "M 0.0 nn $opt(nn) x $opt(x) y $opt(y) z $opt(z)"
 puts $tracefd "M 0.0 prop $opt(prop) ant $opt(ant)"
 puts "starting Simulation..."
 $ns_ run
 
