set opt(chan)			Channel/UnderwaterChannel
set opt(prop)			Propagation/UnderwaterPropagation
set opt(netif)			Phy/UnderwaterPhy
set opt(mac)			Mac/UnderwaterMac/DMac
set opt(ifq)			Queue/DropTail/PriQueue
set opt(ll)				LL
set opt(energy)         EnergyModel
set opt(txpower)        10;#not here
set opt(rxpower)        1; #not here
set opt(initialenergy)  10000
set opt(idlepower)      0.5 ;#set idlepower 
set opt(ant)            Antenna/OmniAntenna  ;#we don't use it in underwater
set opt(filters)        GradientFilter    ;# options can be one or more of 
                                ;# TPP/OPP/Gear/Rmst/SourceRoute/Log/TagFilter
Phy/UnderwaterPhy set tranp  10; # set transmission power here
Phy/UnderwaterPhy set recvp  1  ; #here
Phy/UnderwaterPhy set idlep  0.5  ;#not here

#set rate [lindex $argv 0];
set rate 50;
set opt(data_rate_) [expr 0.001*$rate];#  [lindex $argv 0]  ;#0.02


# the following parameters are set fot protocols
set opt(bit_rate)                     1.0e4
set opt(encoding_efficiency)          1

set opt(transmission_time_error)      0.0001; 

# the following parameters are set fot protocols


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
set opt(PeriodInterval)               1 



set opt(dz)                           10
set opt(ifqlen)		              50	;# max packet in ifq
set opt(nn)	                	7;# number of nodes
set opt(layers)                         1
set opt(x)	                	100	;# X dimension of the topography
set opt(y)	                        100  ;# Y dimension of the topography
set opt(z)                              [expr ($opt(layers)-1)*$opt(dz)]
set opt(seed)	                	113;#[lindex $argv 1]
set opt(stop)	                	1000	;# simulation time
set opt(prestop)                        20     ;# time to prepare to stop
set opt(tr)	                	"~/NS2/ns-2.30/result/dmac.tr"	;# trace file
set opt(nam)                            "~/NS2/ns-2.30/result/dmac.nam"  ;# nam file
set opt(datafile)	                "~/NS2/ns-2.30/result/dmac.data"
set opt(adhocRouting)                   Vectorbasedforward ;#SillyRouting
set opt(width)                           20
set opt(adj)                             10
set opt(interval)                        0.001
#set opt(traf)	                	"diffusion-traf.tcl"      ;# traffic file

# ==================================================================

LL set mindelay_		50us
LL set delay_			25us
LL set bandwidth_		0	;# not used

#Queue/DropTail/PriQueue set Prefer_Routing_Protocols    

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


Mac/UnderwaterMac/DMac set nodeCount 7;
Mac/UnderwaterMac/DMac set sendInterval 1.9;
Mac/UnderwaterMac/DMac set baseTime 0.2;
Mac/UnderwaterMac/DMac set guardTime 0.01;
Mac/UnderwaterMac/DMac set dataSize 500;
Mac/UnderwaterMac/DMac set controlSize 20;



Mac/UnderwaterMac/RMac set ND_window_  $opt(ND_window)
Mac/UnderwaterMac/RMac set ACKND_window_ $opt(ACKND_window)
Mac/UnderwaterMac/RMac set PhaseOne_window_ $opt(PhaseOne_window)
Mac/UnderwaterMac/RMac set PhaseTwo_window_ $opt(PhaseTwo_window)
Mac/UnderwaterMac/RMac set PhaseTwo_interval_ $opt(PhaseTwo_interval)
Mac/UnderwaterMac/RMac set IntervalPhase2Phase3_ $opt(IntervalPhase2Phase3)

Mac/UnderwaterMac/RMac set duration_ $opt(duration)
Mac/UnderwaterMac/RMac set PhyOverhead_ $opt(PhyOverhead)
Mac/UnderwaterMac/RMac set large_packet_size_  $opt(large_packet_size) 
Mac/UnderwaterMac/RMac set short_packet_size_  $opt(short_packet_size)
Mac/UnderwaterMac/RMac set PhaseOne_cycle_   $opt(PhaseOne_cycle)
Mac/UnderwaterMac/RMac set PhaseTwo_cycle_   $opt(PhaseTwo_cycle)
Mac/UnderwaterMac/RMac set PeriodInterval_   $opt(PeriodInterval)
Mac/UnderwaterMac/RMac set transmission_time_error_ $opt(transmission_time_error) 


# Initialize the SharedMedia interface with parameters to make
# it work like the 914MHz Lucent WaveLAN DSSS radio interface
Phy/UnderwaterPhy set CPThresh_ 100  ;#10.0
Phy/UnderwaterPhy set CSThresh_ 0  ;#1.559e-11
Phy/UnderwaterPhy set RXThresh_ 0   ;#3.652e-10
#Phy/UnderwaterPhy set Rb_ 2*1e6
Phy/UnderwaterPhy set Pt_ 0.2818;
Phy/UnderwaterPhy set freq_ 25  ;#frequency range in khz 
Phy/UnderwaterPhy set K_ 2.0   ;#spherical spreading
Phy/UnderwaterPhy set sync_hdr_len 1;



# ==================================================================
# Main Program
# =================================================================

#
# Initialize Global Variables
# 
#set sink_ 1


remove-all-packet-headers 
#remove-packet-header AODV ARP TORA  IMEP TFRC
add-packet-header IP Mac LL  ARP  UWVB DMAC

set ns_ [new Simulator]
set topo  [new Topography]

$topo load_cubicgrid $opt(x) $opt(y) $opt(z)
#$ns_ use-newtrace
set tracefd	[open $opt(tr) w]
$ns_ trace-all $tracefd

set nf [open $opt(nam) w]
$ns_ namtrace-all-wireless $nf $opt(x) $opt(y)

set data [open $opt(datafile) w]


# DMAC
set start_time 1
#puts "the start time is $start_time"

#RMAC
#set phase1_time [expr $opt(PhaseOne_cycle)*$opt(PhaseOne_window)]
#set phase2_time [expr $opt(PhaseTwo_cycle)*($opt(PhaseTwo_window)+$opt(PhaseTwo_interval))]
#set start_time [expr $phase1_time+$phase2_time+$opt(IntervalPhase2Phase3)]

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
		 -agentTrace OFF \
                 -routerTrace ON \
                 -macTrace ON\
                 -topoInstance $topo\
                 -energyModel $opt(energy)\
                 -txpower $opt(txpower)\
                 -rxpower $opt(rxpower)\
                 -initialEnergy $opt(initialenergy)\
                 -idlePower $opt(idlepower)\
                 -channel $chan_1_



for {set i 0} {$i<$opt(nn)} {incr i} {

	set node_($i) [$ns_  node $i]
	$node_($i) set sinkStatus_ 1
	$node_($i) set passive 1
	$god_ new_node $node_($i)
	
	set a_($i) [new Agent/UWSink]
	$ns_ attach-agent $node_($i) $a_($i)
	$a_($i) attach-vectorbasedforward $opt(width)
	$a_($i) cmd set-range 200
	$a_($i) cmd set-filename $opt(datafile)
	$a_($i) set data_rate_ $opt(data_rate_)
	
}

#for {set ti 0} {$ti < $opt(stop) } {incr ti 120} {
#    for {set i 0} {$i < $opt(nn) } {incr i} { 
#
#	      ;
#	#$ns_ at $ti "$node_($i) setSoundSpeed 1400";
#   	$ns_ at [expr $ti+50] "$node_($i) setSoundSpeed 1400";
#
#      }
#  }


$node_(0) set X_  70
$node_(0) set Y_  70
$node_(0) set Z_  0
$a_(0) setTargetAddress 4
#$a_(0) cmd set-target-x   30
#$a_(0) cmd set-target-y   30
#$a_(0) cmd set-target-z   1
$node_(0) set_next_hop 4
$node_(0) set-cx 70
$node_(0) set-cy 70
$node_(0) set-cz 0

$node_(1) set X_  30
$node_(1) set Y_  30
$node_(1) set Z_  1
$node_(1) set_next_hop 5
$a_(1) setTargetAddress 5
#$a_(1) cmd set-target-x   120
#$a_(1) cmd set-target-y   200
#$a_(1) cmd set-target-z   0
$node_(1) set-cx 30
$node_(1) set-cy 30
$node_(1) set-cz 1

$node_(2) set X_  90
$node_(2) set Y_  50
$node_(2) set Z_  6
$node_(2) set_next_hop 3
$a_(2) setTargetAddress 3
#$a_(2) cmd set-target-x   30
#$a_(2) cmd set-target-y   100
#$a_(2) cmd set-target-z   0
$node_(2) set-cx 90
$node_(2) set-cy 50
$node_(2) set-cz 6

$node_(3) set X_  30
$node_(3) set Y_  100
$node_(3) set Z_  0
#$node_(3) set passive 1
$node_(3) set_next_hop 1
$a_(3) setTargetAddress 1
#$a_(3) cmd set-target-x   70
#$a_(3) cmd set-target-y   70
#$a_(3) cmd set-target-z   0
$node_(3) set-cx 30
$node_(3) set-cy 100
$node_(3) set-cz 0

$node_(4) set X_  110
$node_(4) set Y_  130
$node_(4) set Z_  3
$node_(4) set_next_hop 6
$a_(4) setTargetAddress 6
#$a_(4) cmd set-target-x   130
#$a_(4) cmd set-target-y   80
#$a_(4) cmd set-target-z   0
$node_(4) set-cx 110
$node_(4) set-cy 130
$node_(4) set-cz 3

$node_(5) set X_  120
$node_(5) set Y_  200
$node_(5) set Z_  0
$node_(5) set_next_hop 1
$a_(5) setTargetAddress 1
#$a_(5) cmd set-target-x   80
#$a_(5) cmd set-target-y   160
#$a_(5) cmd set-target-z   0
$node_(5) set-cx 120
$node_(5) set-cy 200
$node_(5) set-cz 0

$node_(6) set X_  130
$node_(6) set Y_  80
$node_(6) set Z_  0
$node_(6) set_next_hop 4
$a_(6) setTargetAddress 4
#$a_(6) cmd set-target-x   110
#$a_(6) cmd set-target-y   130
#$a_(6) cmd set-target-z   3
$node_(6) set-cx 130
$node_(6) set-cy 80
$node_(6) set-cz 0



#for { set i 0 } { $i < 8 } { incr i } {
#	set start_time [expr $start_time+0.69238]
#	$ns_ at $start_time "$a_($i) cbr-start"
#}

$ns_ at $start_time.11 "$a_(0) exp-start"
$ns_ at $start_time.11 "$a_(1) exp-start"
$ns_ at $start_time.40 "$a_(2) exp-start"
$ns_ at $start_time.70 "$a_(3) exp-start"
$ns_ at $start_time.99 "$a_(4) exp-start"
$ns_ at $start_time.79 "$a_(5) exp-start"
$ns_ at $start_time.44 "$a_(6) exp-start"
#$ns_ at $start_time.66 "$a_(7) exp-start"


set node_size 10
for {set k 0} { $k<$opt(nn) } { incr k } {
	$ns_ initial_node_pos $node_($k) $node_size
}
puts "+++++++AFTER ANNOUNCE++++++++++++++"


#$ns_ at [expr $opt(stop)-50] "$a_(0) terminate"
#$ns_ at [expr $opt(stop)-50] "$a_(1) terminate"
#$ns_ at [expr $opt(stop)-50] "$a_(2) terminate"
#$ns_ at [expr $opt(stop)-50] "$a_(3) terminate"
#$ns_ at [expr $opt(stop)-50] "$a_(4) terminate"
#$ns_ at [expr $opt(stop)-50] "$a_(5) terminate"
#$ns_ at [expr $opt(stop)-50] "$a_(6) terminate"
#$ns_ at [expr $opt(stop)-50] "$a_(7) terminate"

$ns_ at $opt(stop).001 "$a_(0) terminate"
$ns_ at $opt(stop).003 "$a_(1) terminate"
$ns_ at $opt(stop).004 "$a_(2) terminate"
$ns_ at $opt(stop).005 "$a_(3) terminate"
$ns_ at $opt(stop).006 "$a_(4) terminate"
$ns_ at $opt(stop).007 "$a_(5) terminate"
$ns_ at $opt(stop).008 "$a_(6) terminate"


$ns_ at $opt(stop).013  "$god_ compute_energy"
$ns_ at $opt(stop).014  "$ns_ nam-end-wireless $opt(stop)"
$ns_ at $opt(stop).015 "puts \"NS EXISTING...\"; $ns_ halt"

puts $data  "New simulation...."
puts $data "nodes  = $opt(nn), random_seed = $opt(seed), sending_interval_=$opt(interval), width=$opt(width)"
puts $data "mac is $opt(mac)"	
puts $data "x= $opt(x) y= $opt(y) z= $opt(z)"
close $data

 puts $tracefd "SillyRrouting"
 puts $tracefd "M 0.0 nn $opt(nn) x $opt(x) y $opt(y) z $opt(z)"
 puts $tracefd "M 0.0 prop $opt(prop) ant $opt(ant)"
 puts "starting Simulation..."
 $ns_ run
