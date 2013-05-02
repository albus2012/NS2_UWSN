# ======================================================================
# Default Script Options
# ======================================================================

set opt(ragent)		Agent/rtProto/AODV
set opt(pos)		NONE

if { $opt(pos) != "NONE" } {
	puts "*** WARNING: AODV using $opt(pos) position configuration..."
}

# ======================================================================
Agent instproc init args {
        $self next $args
}       
Agent/rtProto instproc init args {
        $self next $args
}       
Agent/rtProto/AODV instproc init args {
        $self next $args
}       

Agent/rtProto/AODV set sport_	0
Agent/rtProto/AODV set dport_	0

# ======================================================================

proc create-routing-agent { node id } {
	global ns_ ragent_ tracefd opt

	#
	#  Create the Routing Agent and attach it to port 255.
	#
	set ragent_($id) [new $opt(ragent) $id]
	set ragent $ragent_($id)
	$node attach $ragent 255

	$ragent if-queue [$node set ifq_(0)]	;# ifq between LL and MAC
	$ns_ at 0.$id "$ragent_($id) start"	;# start BEACON/HELLO Messages

	#
	# Drop Target (always on regardless of other tracing)
	#
	set drpT [cmu-trace Drop "RTR" $node]
	$ragent drop-target $drpT
	
	#
	# Log Target
	#
	set T [new Trace/Generic]
	$T target [$ns_ set nullAgent_]
	$T attach $tracefd
	$T set src_ $id
	$ragent log-target $T
}


proc create-mobile-node { id } {
	global ns_ chan prop topo tracefd opt node_
	global chan prop tracefd topo opt

	set node_($id) [new MobileNode]

	set node $node_($id)
	$node random-motion 0		;# disable random motion
	$node topography $topo

	#
	# This Trace Target is used to log changes in direction
	# and velocity for the mobile node.
	#
	set T [new Trace/Generic]
	$T target [$ns_ set nullAgent_]
	$T attach $tracefd
	$T set src_ $id
	$node log-target $T

	$node add-interface $chan $prop $opt(ll) $opt(mac)	\
		$opt(ifq) $opt(ifqlen) $opt(netif) $opt(ant)

	#
	# Create a Routing Agent for the Node
	#
	create-routing-agent $node $id

	# ============================================================

	if { $opt(pos) == "Box" } {

            set spacing 200
            set maxrow 3
            set col [expr ($id - 1) % $maxrow]
            set row [expr ($id - 1) / $maxrow]
            $node set X_ [expr $col * $spacing]
            $node set Y_ [expr $row * $spacing]
            $node set Z_ 0.0
            $node set speed_ 0.0

            $ns_ at 0.0 "$node_($id) start"

	} elseif { $opt(pos) == "Random" } {

            $node random-motion 1

            $ns_ at 0.0 "$node_($id) start"
        }
        
}

