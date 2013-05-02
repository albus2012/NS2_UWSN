#
# Copyright (c) 1995-1997 The Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by the Computer Systems
#	Engineering Group at Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-ecn-full.tcl,v 1.21 2006/01/24 23:00:06 sallyfloyd Exp $
#
# To run all tests: test-all-ecn-full

##To run tests:
## set test=ecn_twoecn_reno
## ../../ns test-suite-ecn-full.tcl $test'_full'
## ../../ns test-suite-ecn.tcl $test

set dir [pwd]
catch "cd tcl/test"
source misc_simple.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Queue/RED set bytes_ false              
# default changed on 10/11/2004.
Queue/RED set queue_in_bytes_ false
# default changed on 10/11/2004.
Queue/RED set q_weight_ 0.002
Queue/RED set thresh_ 5 
Queue/RED set maxthresh_ 15
# The RED parameter defaults are being changed for automatic configuration.
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1
Agent/TCP set minrto_ 0
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
catch "cd $dir"
set scale 0.00001
set wrap [expr 90 * 1000 + 40]
Agent/TCP/FullTcp set segsize_ 960

set flowfile fairflow.tr; # file where flow data is written
set flowgraphfile fairflow.xgr; # file given to graph tool 

Class Topology

Topology instproc node? num {
    $self instvar node_
    return $node_($num)
}

Topology instproc makenodes ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(r2) [$ns node]
    set node_(s3) [$ns node]
    set node_(s4) [$ns node]
}

Topology instproc createlinks ns {  
    $self instvar node_
    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
    $ns queue-limit $node_(r1) $node_(r2) 25
    $ns queue-limit $node_(r2) $node_(r1) 25
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail

    $ns duplex-link-op $node_(s1) $node_(r1) orient right-down
    $ns duplex-link-op $node_(s2) $node_(r1) orient right-up
    $ns duplex-link-op $node_(r1) $node_(r2) orient right
    $ns duplex-link-op $node_(r1) $node_(r2) queuePos 0
    $ns duplex-link-op $node_(r2) $node_(r1) queuePos 0
    $ns duplex-link-op $node_(s3) $node_(r2) orient left-down
    $ns duplex-link-op $node_(s4) $node_(r2) orient left-up
}

Class Topology/net2 -superclass Topology
Topology/net2 instproc init ns {
    $self instvar node_
    $self makenodes $ns
    $self createlinks $ns
}

Class Topology/net2-lossy -superclass Topology
Topology/net2-lossy instproc init ns {
    $self instvar node_
    $self makenodes $ns
    $self createlinks $ns
 
    $self instvar lossylink_
    set lossylink_ [$ns link $node_(r1) $node_(r2)]
    set em [new ErrorModule Fid]
    set errmodel [new ErrorModel/Periodic]
    $errmodel unit pkt
    $lossylink_ errormodule $em
    $em insert $errmodel
    $em bind $errmodel 0
    $em default pass
} 

TestSuite instproc finish file {
	global quiet PERL wrap scale
	$self instvar ns_ tchan_ testName_ cwnd_chan_
        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
	   $PERL ../../bin/raw2xg -a -e -s $scale -m $wrap -t $file > temp.rands
	exec $PERL ../../bin/getrc -s 3 -d 2 all.tr | \
	  $PERL ../../bin/raw2xg -a -e -s $scale -m $wrap -t $file > temp1.rands
	if {$quiet == "false"} {
		exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
# The line below plots both data and ack packets.
#        	exec xgraph -bb -tk -nl -m -x time -y packets temp.rands \
#                     temp1.rands &
	}
        ## now use default graphing tool to make a data file
        ## if so desired

	if { [info exists tchan_] && $quiet == "false" } {
		$self plotQueue $testName_
	}
	if { [info exists cwnd_chan_] && $quiet == "false" } {
		$self plot_cwnd 
	}
	$ns_ halt
}

TestSuite instproc enable_tracequeue ns {
	$self instvar tchan_ node_
	set redq [[$ns link $node_(r1) $node_(r2)] queue]
	set tchan_ [open all.q w]
	$redq trace curq_
	$redq trace ave_
	$redq attach $tchan_
}

TestSuite instproc plotQueue file {
	global quiet
	$self instvar tchan_
	#
	# Plot the queue size and average queue size, for RED gateways.
	#
	set awkCode {
		{
			if ($1 == "Q" && NF>2) {
				print $2, $3 >> "temp.q";
				set end $2
			}
			else if ($1 == "a" && NF>2)
				print $2, $3 >> "temp.a";
		}
	}
	set f [open temp.queue w]
	puts $f "TitleText: $file"
	puts $f "Device: Postscript"

	if { [info exists tchan_] } {
		close $tchan_
	}
	exec rm -f temp.q temp.a 
	exec touch temp.a temp.q

	exec awk $awkCode all.q

	puts $f \"queue
	exec cat temp.q >@ $f  
	puts $f \n\"ave_queue
	exec cat temp.a >@ $f
	###puts $f \n"thresh
	###puts $f 0 [[ns link $r1 $r2] get thresh]
	###puts $f $end [[ns link $r1 $r2] get thresh]
	close $f
	if {$quiet == "false"} {
		exec xgraph -bb -tk -x time -y queue temp.queue &
	}
}

TestSuite instproc tcpDumpAll { tcpSrc interval label } {
    global quiet
    $self instvar dump_inst_ ns_
    if ![info exists dump_inst_($tcpSrc)] {
	set dump_inst_($tcpSrc) 1
	set report $label/window=[$tcpSrc set window_]/packetSize=[$tcpSrc set packetSize_]
	if {$quiet == "false"} {
		puts $report
	}
	$ns_ at 0.0 "$self tcpDumpAll $tcpSrc $interval $label"
	return
    }
    $ns_ at [expr [$ns_ now] + $interval] "$self tcpDumpAll $tcpSrc $interval $label"
    set report time=[$ns_ now]/class=$label/ack=[$tcpSrc set ack_]/packets_resent=[$tcpSrc set nrexmitpack_]
    if {$quiet == "false"} {
    	puts $report
    }
}       

TestSuite instproc emod {} {
	$self instvar topo_
	$topo_ instvar lossylink_
        set errmodule [$lossylink_ errormodule]
	return $errmodule
}

TestSuite instproc setloss {} {
	$self instvar topo_
	$topo_ instvar lossylink_
        set errmodule [$lossylink_ errormodule]
        set errmodel [$errmodule errormodels]
        if { [llength $errmodel] > 1 } {
                puts "droppedfin: confused by >1 err models..abort"
                exit 1
        }
	return $errmodel
}

TestSuite instproc ecnsetup { tcptype {stoptime 3.0} { tcp1fid 0 } { delack 0 }} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
##
##  Agent/TCP set maxburst_ 4
##
    set delay 10ms
    $ns_ delay $node_(r1) $node_(r2) $delay
    $ns_ delay $node_(r2) $node_(r1) $delay

    set redq [[$ns_ link $node_(r1) $node_(r2)] queue]
    $redq set setbit_ true
    $self tcpconnection $tcptype $tcp1fid $delack 1

    #$self enable_tracequeue $ns_
        
    # trace only the bottleneck link
    ##$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"
}

TestSuite instproc tcpconnection { tcptype tcpfid delack dump } {
    $self instvar ns_ node_
    if {$tcptype == "Reno" } {
      set tcp [$ns_ create-connection-list TCP/FullTcp $node_(s1) \
	  TCP/FullTcp $node_(s3) $tcpfid]
    } else {
      set tcp [$ns_ create-connection-list TCP/FullTcp/$tcptype $node_(s1) \
	  TCP/FullTcp/$tcptype $node_(s3) $tcpfid]
    }
    set tcp1 [lindex $tcp 0]
    set sink [lindex $tcp 1]
    $sink listen
    $sink set ecn_ 1
 
    if {$delack == 1} {
	$sink set interval_ 100ms
    }
    $tcp1 set window_ 30
    $tcp1 set ecn_ 1

    set ftp1 [$tcp1 attach-app FTP]
    $ns_ at 0.0 "$ftp1 start"
    if { $dump == 1 } {
	$self tcpDump $tcp1 5.0
        $self enable_tracecwnd $ns_ $tcp1
    }
}

TestSuite instproc second_tcp { tcptype starttime } {
    $self instvar ns_ node_
    if {$tcptype == "Tahoe"} {
      set tcp [$ns_ create-connection TCP $node_(s1) \
         TCPSink $node_(s3) 2]
    } elseif {$tcptype == "Sack1"} {
      set tcp [$ns_ create-connection TCP/Sack1 $node_(s1) \
          TCPSink/Sack1  $node_(s3) 2]
    } else {
      set tcp [$ns_ create-connection TCP/$tcptype $node_(s1) \
         TCPSink $node_(s3) 2]
    }
    $tcp set window_ 30
    $tcp set ecn_ 1
    set ftp [$tcp attach-app FTP]
    $ns_ at $starttime "$ftp start"
}

# TestSuite instproc second_tcp { tcptype starttime } {
#     $self tcpconnection $tcptype 2 0 0 
# }

# Drop the specified packet.
TestSuite instproc drop_pkt { number } {
    $self instvar ns_ lossmodel
    set lossmodel [$self setloss]
    $lossmodel set offset_ $number
    $lossmodel set period_ 10000
}

TestSuite instproc drop_pkts pkts {
    $self instvar ns_ errmodel1
    set emod [$self emod]
    set errmodel1 [new ErrorModel/List]
    $errmodel1 droplist $pkts
    $emod insert $errmodel1
    $emod bind $errmodel1 1

}

#######################################################################
# Reno Tests #
#######################################################################

# Plain ECN
Class Test/ecn_nodrop_reno_full -superclass TestSuite
Test/ecn_nodrop_reno_full instproc init {} {
        $self instvar net_ test_
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set bugFix_ true
        set test_	ecn_nodrop_reno_full
        $self next pktTraceFile
}
Test/ecn_nodrop_reno_full instproc run {} {
	$self instvar ns_
	Agent/TCP set old_ecn_ 1
	$self ecnsetup Reno 3.0 
	$self drop_pkt 10000
	$ns_ run
}

# Two ECNs close together
Class Test/ecn_twoecn_reno_full -superclass TestSuite
Test/ecn_twoecn_reno_full instproc init {} {
        $self instvar net_ test_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set bugFix_ true
        set test_	ecn_twoecn_reno_full
        $self next pktTraceFile
}
Test/ecn_twoecn_reno_full instproc run {} {
	$self instvar ns_ lossmodel
	Agent/TCP set old_ecn_ 1
	$self ecnsetup Reno 3.0 
	$self drop_pkt 243
	$lossmodel set markecn_ true
	$ns_ run
}

# ECN followed by packet loss.
Class Test/ecn_drop_reno_full -superclass TestSuite
Test/ecn_drop_reno_full instproc init {} {
        $self instvar net_ test_
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set bugFix_ true
        set test_	ecn_drop_reno_full
        $self next pktTraceFile
}
Test/ecn_drop_reno_full instproc run {} {
	$self instvar ns_
	Agent/TCP set old_ecn_ 1
	$self ecnsetup Reno 3.0
	$self drop_pkt 243
	$ns_ run
}

# This shows ECN preceded by packet loss,
Class Test/ecn_drop1_reno_full -superclass TestSuite
Test/ecn_drop1_reno_full instproc init {} {
        $self instvar net_ test_
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set bugFix_ true
        set test_	ecn_drop1_reno_full
        $self next pktTraceFile
}
Test/ecn_drop1_reno_full instproc run {} {
	$self instvar ns_
	Agent/TCP set old_ecn_ 1
	$self ecnsetup Reno 3.0
	$self drop_pkt 237
	$ns_ run
}

# Packet loss only.
Class Test/ecn_noecn_reno_full -superclass TestSuite
Test/ecn_noecn_reno_full instproc init {} {
        $self instvar net_ test_
        Queue/RED set setbit_ true
	Queue/RED set thresh_ 1000
	Queue/RED set maxthresh_ 1000
        set net_	net2-lossy
	Agent/TCP set bugFix_ true
        set test_	ecn_noecn_reno_full
	Test/ecn_noecn_reno_full instproc run {} [Test/ecn_drop_reno_full info instbody run ]
        $self next pktTraceFile
}

# Multiple dup acks with bugFix_
Class Test/ecn_bursty_reno_full -superclass TestSuite
Test/ecn_bursty_reno_full instproc init {} {
        $self instvar net_ test_
        Queue/RED set setbit_ true
	Queue/RED set thresh_ 100
	Queue/RED set maxthresh_ 100
        set net_	net2-lossy
	Agent/TCP set bugFix_ true
        set test_	ecn_bursty_reno_full
        $self next pktTraceFile
}
Test/ecn_bursty_reno_full instproc run {} {
	$self instvar ns_

	$self ecnsetup Reno 3.0
        set lossmodel [$self setloss]
        $lossmodel set offset_ 245
	$lossmodel set burstlen_ 15
        $lossmodel set period_ 10000
	$ns_ run
}

# Multiple dup acks following ECN
Class Test/ecn_burstyEcn_reno_full -superclass TestSuite
Test/ecn_burstyEcn_reno_full instproc init {} {
        $self instvar net_ test_
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set bugFix_ true
        set test_	ecn_burstyEcn_reno_full
	Test/ecn_burstyEcn_reno_full instproc run {} [Test/ecn_bursty_reno_full info instbody run ]   
        $self next pktTraceFile
}

# Multiple dup acks without bugFix_
Class Test/ecn_noBugfix_reno_full -superclass TestSuite
Test/ecn_noBugfix_reno_full instproc init {} {
        $self instvar net_ test_
	Queue/RED set thresh_ 100 
	Queue/RED set maxthresh_ 100
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set bugFix_ false
        set test_	ecn_noBugfix_reno_full
	Test/ecn_noBugfix_reno_full instproc run {} [Test/ecn_bursty_reno_full info instbody run ]

        $self next pktTraceFile
}

# ECN followed by timeout.

Class Test/ecn_timeout_reno_full -superclass TestSuite
Test/ecn_timeout_reno_full instproc init {} {
        $self instvar net_ test_
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set bugFix_ true
        set test_	ecn_timeout_reno_full
        $self next pktTraceFile
}
Test/ecn_timeout_reno_full instproc run {} {
	$self instvar ns_
	$self ecnsetup Reno 3.0 1
	$self drop_pkts {242 243 244 245 246 247 248 249 250 251 252 253 254 255 256 257 258 259 260 261 262 263 264 265 266 267} 

	$ns_ run
}

# ECN followed by a timeout, followed by an ECN representing a
# new instance of congestion.

Class Test/ecn_timeout1_reno_full -superclass TestSuite
Test/ecn_timeout1_reno_full instproc init {} {
        $self instvar net_ test_
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set bugFix_ true
        set test_	ecn_timeout1_reno_full
        $self next pktTraceFile
}
Test/ecn_timeout1_reno_full instproc run {} {
	$self instvar ns_
	Agent/TCP set old_ecn_ 1
	$self ecnsetup Reno 3.0 1
	$self drop_pkts {245 246 247 248 249 250 251 252 253 254 255 256 257 258 259 260 261 262} 
	$self second_tcp Tahoe 1.2
	$ns_ run
}

# Packet drops with a window of one packet.
Class Test/ecn_smallwin_reno_full -superclass TestSuite
Test/ecn_smallwin_reno_full instproc init {} {
        $self instvar net_ test_
        Queue/RED set setbit_ true
	Agent/TCP set bugFix_ true
        set net_	net2-lossy
        set test_	ecn_smallwin_reno_full
        $self next pktTraceFile
}
Test/ecn_smallwin_reno_full instproc run {} {
	$self instvar ns_
	Agent/TCP set old_ecn_ 0
	$self ecnsetup Reno 6.0 1
	$self drop_pkts {4 8 9 10 11 100 115 118 119 121 122}

	$ns_ run
}

#ECN with a window of one packet.
Class Test/ecn_smallwinEcn_reno_full -superclass TestSuite
Test/ecn_smallwinEcn_reno_full instproc init {} {
        $self instvar net_ test_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set bugFix_ true
	Agent/TCP set rfc2988_ true
        set test_	ecn_smallwinEcn_reno_full
        $self next pktTraceFile
}
Test/ecn_smallwinEcn_reno_full instproc run {} {
	$self instvar ns_ errmodel1
	Agent/TCP set old_ecn_ 0
	$self ecnsetup Reno 10.0 1
	$self drop_pkts {4 8 9 10 11 100 115 118 119 121 122}
	#$self drop_pkts {6 10 11 13 14 15 122 137 145 150 152 153 154 155}
	$errmodel1 set markecn_ true
	$ns_ run
}

# Packet drops for the second packet.
Class Test/ecn_secondpkt_reno_full -superclass TestSuite
Test/ecn_secondpkt_reno_full instproc init {} {
        $self instvar net_ test_
        Queue/RED set setbit_ true
	Agent/TCP set bugFix_ true
        set net_	net2-lossy
        set test_	ecn_secondpkt_reno_full
        $self next pktTraceFile
}
Test/ecn_secondpkt_reno_full instproc run {} {
	$self instvar ns_
	Agent/TCP set old_ecn_ 0
	$self ecnsetup Reno 2.0 1
	$self drop_pkts {3 5} 

	$ns_ run
}

# ECN for the second packet.
Class Test/ecn_secondpktEcn_reno_full -superclass TestSuite
Test/ecn_secondpktEcn_reno_full instproc init {} {
        $self instvar net_ test_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set bugFix_ true
        set test_	ecn_secondpktEcn_reno_full
        $self next pktTraceFile
}
Test/ecn_secondpktEcn_reno_full instproc run {} {
	$self instvar ns_ errmodel1
	Agent/TCP set old_ecn_ 0
	$self ecnsetup Reno 2.0 1
	#$self drop_pkts {2 4} 
	$self drop_pkts {3 5}
	$errmodel1 set markecn_ true
	$ns_ run
}

TestSuite runTest
