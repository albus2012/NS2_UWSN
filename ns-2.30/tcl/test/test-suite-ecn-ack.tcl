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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-ecn-ack.tcl,v 1.26 2006/07/23 20:46:02 sallyfloyd Exp $
#
# To run all tests: test-all-ecn-ack
set dir [pwd]
catch "cd tcl/test"
catch "cd $dir"
source misc_simple.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP RTP ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:

set flowfile fairflow.tr; # file where flow data is written
set flowgraphfile fairflow.xgr; # file given to graph tool 

Trace set show_tcphdr_ 1

set wrap 90
set wrap1 [expr 90 * 512 + 40]

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

Class Topology/net2A-lossy -superclass Topology
Topology/net2A-lossy instproc init ns {
    $self instvar node_
    $self makenodes $ns
    $self createlinks $ns

    $self instvar lossylink_
    set lossylink_ [$ns link $node_(r2) $node_(r1)]
    set em [new ErrorModule Fid]
    set errmodel [new ErrorModel/Periodic]
    $errmodel unit pkt
    $lossylink_ errormodule $em
    $em insert $errmodel
    $em bind $errmodel 0
    $em default pass
}

TestSuite instproc finish file {
	global quiet wrap wrap1 PERL
	$self instvar ns_ tchan_ testName_ cwnd_chan_
	if {$file == "ecn_ack_fulltcp"} { 
	   ## tracing forward-path data packets
           exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
	     $PERL ../../bin/raw2xg -aecx -s 0.01 -m $wrap -t $file > temp.rands
  	   exec $PERL ../../bin/getrc -s 3 -d 2 all.tr | \
	     $PERL ../../bin/raw2xg -aefcx -s 0.01 -m $wrap -t $file > temp1.rands
	} elseif {$wrap == $wrap1} {
	   ## tracing reverse-path data packets
           exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
	     $PERL ../../bin/raw2xg -aefcx -s 0.01 -m $wrap -t $file > temp.rands
  	   exec $PERL ../../bin/getrc -s 3 -d 2 all.tr | \
	     $PERL ../../bin/raw2xg -aecx -s 0.01 -m $wrap -t $file > temp1.rands
	} else {
           exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
	     $PERL ../../bin/raw2xg -aecx -s 0.01 -m $wrap -t $file > temp.rands
  	   exec $PERL ../../bin/getrc -s 3 -d 2 all.tr | \
	     $PERL ../../bin/raw2xg -aecx -s 0.01 -m $wrap -t $file > temp1.rands
	}
	if {$quiet == "false"} {
        	exec xgraph -bb -tk -nl -m -x time -y packets temp.rands \
                     temp1.rands &
	}
        ## now use default graphing tool to make a data file
        ## if so desired
        if { [info exists cwnd_chan_] && $quiet == "false" } {
                $self plot_cwnd
        }
        if { [info exists tchan_] && $quiet == "false" } {
                $self plotQueue $testName_
        }
	# exec csh gnuplotC2.com temp.rands temp1.rands $file eps
	$ns_ halt
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

TestSuite instproc ecnsetup { tcptype { tcp1fid 0 } } {
    $self instvar ns_ node_ testName_ net_
    global wrap wrap1

    set delay 10ms
    $ns_ delay $node_(r1) $node_(r2) $delay
    $ns_ delay $node_(r2) $node_(r1) $delay

    set stoptime 2.0
    set redq [[$ns_ link $node_(r1) $node_(r2)] queue]
    $redq set setbit_ true
    $redq set limit_ 50

    set fid 1
    if {$tcptype == "Tahoe"} {
      set tcp1 [$ns_ create-connection TCP $node_(s1) \
	  TCPSink $node_(s3) $tcp1fid]
    } elseif {$tcptype == "Sack1"} {
      set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) \
	  TCPSink/Sack1  $node_(s3) $tcp1fid]
    } elseif {$tcptype == "FullTcpSack1"} {
          set wrap $wrap1
          set tcp1 [new Agent/TCP/FullTcp/Sack]
          set sink [new Agent/TCP/FullTcp/Sack]
          $ns_ attach-agent $node_(s1) $tcp1
          $ns_ attach-agent $node_(s3) $sink
          $tcp1 set fid_ $fid
          $sink set fid_ $fid
          $ns_ connect $tcp1 $sink
          # set up TCP-level connections
          $sink listen ; # will figure out who its peer is
	  $sink set window_ 35
	  $sink set ecn_ 1
    } else {
      set tcp1 [$ns_ create-connection TCP/$tcptype $node_(s1) \
	  TCPSink $node_(s3) $tcp1fid]
    }
    $tcp1 set window_ 35
    $tcp1 set ecn_ 1
    set ftp1 [$tcp1 attach-app FTP]
    $self enable_tracecwnd $ns_ $tcp1
        
    $ns_ at 0.0 "$ftp1 start"
        
    $self tcpDump $tcp1 5.0
        
    # trace only the bottleneck link
    ##$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"
}

# Drop the specified packet.
TestSuite instproc drop_pkt { number } {
    $self instvar ns_ lossmodel
    set lossmodel [$self setloss]
    $lossmodel set offset_ $number
    $lossmodel set period_ 10000
}

# Mark the specified packet.
TestSuite instproc mark_pkt { number } {
    $self instvar ns_ lossmodel
    set lossmodel [$self setloss]
    $lossmodel set offset_ $number
    $lossmodel set period_ 10000
    $lossmodel set markecn_ true
}

TestSuite instproc drop_pkts pkts {
    $self instvar ns_
    set emod [$self emod]
    set errmodel1 [new ErrorModel/List]
    $errmodel1 droplist $pkts
    $emod insert $errmodel1
    $emod bind $errmodel1 1
}

#######################################################################
# SACK Test #
#######################################################################

Class Test/ecn_ack -superclass TestSuite
Test/ecn_ack instproc init {} {
        $self instvar net_ test_
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set bugFix_ true
        set test_	ecn_ack
        $self next
}
Test/ecn_ack instproc run {} {
	$self instvar ns_
	$self setTopo
	$self ecnsetup Sack1
	$self drop_pkt 20000
	$ns_ run
}

Class Test/ecn_ack_fulltcp -superclass TestSuite
Test/ecn_ack_fulltcp instproc init {} {
        $self instvar net_ test_
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set bugFix_ true
        set test_	ecn_ack_fulltcp
        $self next
}
Test/ecn_ack_fulltcp instproc run {} {
        global quiet wrap wrap1
	$self instvar ns_
	$self setTopo
        set wrap $wrap1
	$self ecnsetup FullTcpSack1
	$self drop_pkt 20000
	$ns_ run
}

#######################################################################
# SYN/ACK Packets #
#######################################################################

# No SYN packets dropped or marked.
Class Test/synack -superclass TestSuite
Test/synack instproc init {} {
        $self instvar net_ test_ guide_ 
        set net_        net2-lossy
        set test_       synack_
        set guide_      "No SYN packets dropped or marked."
        Agent/TCPSink set ecn_syn_ false
        $self next pktTraceFile
}
Test/synack instproc run {} {
        global quiet
        $self instvar ns_ guide_ node_ guide_ testName_ 
        puts "Guide: $guide_"
        Agent/TCP set ecn_ 1
	Agent/TCP set window_ 8
        $self setTopo

        # Set up forward TCP connection
        set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(s4) 0]
	$tcp1 set window_ 8
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.00 "$ftp1 produce 20"

        $self drop_pkt 1000
        $self tcpDump $tcp1 5.0
        $ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}

# Full TCP, no SYN packets dropped or marked.
Class Test/synack_fulltcp -superclass TestSuite
Test/synack_fulltcp instproc init {} {
        $self instvar net_ test_ guide_ 
        set net_        net2-lossy
        set test_       synack_fulltcp_
        set guide_      "Full TCP, no SYN packets dropped or marked."
        Agent/TCPSink set ecn_syn_ false
        $self next pktTraceFile
}
Test/synack_fulltcp instproc run {} {
        global quiet wrap wrap1
        $self instvar ns_ guide_ node_ guide_ testName_ 
        puts "Guide: $guide_"
        Agent/TCP set ecn_ 1
	Agent/TCP set window_ 8
        $self setTopo

        # Set up forward TCP connection
        set wrap $wrap1
        set tcp1 [new Agent/TCP/FullTcp/Sack]
        set sink [new Agent/TCP/FullTcp/Sack]
        $ns_ attach-agent $node_(s1) $tcp1
        $ns_ attach-agent $node_(s4) $sink
        $tcp1 set fid_ 0
        $sink set fid_ 0
        $ns_ connect $tcp1 $sink
        $sink listen ; # will figure out who its peer is
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.00 "$ftp1 produce 5"
        set ftp2 [$sink attach-app FTP]
        $ns_ at 0.03 "$ftp2 produce 20"

        $self drop_pkt 1000
        $self tcpDump $tcp1 5.0
        $ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}


# SYN packet dropped.
Class Test/synack0 -superclass TestSuite
Test/synack0 instproc init {} {
        $self instvar net_ test_ guide_ 
        set net_        net2-lossy
        set test_       synack0_
        set guide_      "SYN packet dropped."
        Agent/TCPSink set ecn_syn_ false
        $self next pktTraceFile
}
Test/synack0 instproc run {} {
        global quiet
        $self instvar ns_ guide_ node_ guide_ testName_ 
        puts "Guide: $guide_"
        Agent/TCP set ecn_ 1
	Agent/TCP set window_ 8
        $self setTopo

        # Set up forward TCP connection
        set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(s4) 0]
	$tcp1 set window_ 8
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.00 "$ftp1 produce 20"

        $self drop_pkt 1
        $self tcpDump $tcp1 5.0
        $ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}

# SYN packet dropped, old parameters for RTO.
Class Test/synack0A -superclass TestSuite
Test/synack0A instproc init {} {
        $self instvar net_ test_ guide_ 
        set net_        net2-lossy
        set test_       synack0A_
        set guide_      "SYN packet dropped, old parameters for RTO."
        Agent/TCPSink set ecn_syn_ false
	Agent/TCP set rtxcur_init_ 6.0 
	Agent/TCP set updated_rttvar_ false 
	Test/synack0A instproc run {} [Test/synack0 info instbody run ]
        $self next pktTraceFile
}

# SYN/ACK packet dropped.
Class Test/synack1 -superclass TestSuite
Test/synack1 instproc init {} {
        $self instvar net_ test_ guide_ 
        set net_        net2A-lossy
        set test_       synack1_
        set guide_      "SYN/ACK packet dropped."
        Agent/TCPSink set ecn_syn_ false
        $self next pktTraceFile
}
Test/synack1 instproc run {} {
        global quiet
        $self instvar ns_ guide_ node_ guide_ testName_
        puts "Guide: $guide_"
        Agent/TCP set ecn_ 1
        $self setTopo

        # Set up forward TCP connection
        set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(s4) 0]
        $tcp1 set window_ 8
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.00 "$ftp1 produce 20"

        $self drop_pkt 1
        $self tcpDump $tcp1 5.0
        $ns_ at 5.0 "$self cleanupAll $testName_"
        $ns_ run
}

# SYN/ACK packet dropped, FullTCP
Class Test/synack1_fulltcp -superclass TestSuite
Test/synack1_fulltcp instproc init {} {
        $self instvar net_ test_ guide_ 
        set net_        net2A-lossy
        set test_       synack1_fulltcp_
        set guide_      "SYN/ACK packet dropped, FullTCP."
        Agent/TCPSink set ecn_syn_ false
        $self next pktTraceFile
}
Test/synack1_fulltcp instproc run {} {
        global quiet wrap wrap1
        $self instvar ns_ guide_ node_ guide_ testName_
        puts "Guide: $guide_"
        Agent/TCP set ecn_ 1
        Agent/TCP set window_ 8
        $self setTopo

        # Set up forward TCP connection
        set wrap $wrap1
        set tcp1 [new Agent/TCP/FullTcp/Sack]
        set sink [new Agent/TCP/FullTcp/Sack]
        $ns_ attach-agent $node_(s1) $tcp1
        $ns_ attach-agent $node_(s4) $sink
        $tcp1 set fid_ 0
        $sink set fid_ 0
        $ns_ connect $tcp1 $sink
        $sink listen ; # will figure out who its peer is
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.00 "$ftp1 produce 5"
        set ftp2 [$sink attach-app FTP]
        $ns_ at 0.03 "$ftp2 produce 20"

        $self drop_pkt 1
        $self tcpDump $tcp1 5.0
        $ns_ at 5.0 "$self cleanupAll $testName_"
        $ns_ run
}

# SYN/ACK packet marked.
Class Test/synack2 -superclass TestSuite
Test/synack2 instproc init {} {
        $self instvar net_ test_ guide_ 
        set net_        net2A-lossy
        set test_       synack2_
        set guide_      "SYN/ACK packet marked."
        Agent/TCPSink set ecn_syn_ true
        $self next pktTraceFile
}
Test/synack2 instproc run {} {
        global quiet
        $self instvar ns_ guide_ node_ guide_ testName_
        puts "Guide: $guide_"
        Agent/TCP set ecn_ 1
	Agent/TCP set window_ 8
        $self setTopo

        # Set up forward TCP connection
        set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(s4) 0]
        $tcp1 set window_ 8

        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.00 "$ftp1 produce 20"

        $self mark_pkt 1
        # $self mark_pkt 10
        $self tcpDump $tcp1 5.0
        $ns_ at 2.0 "$self cleanupAll $testName_"
        $ns_ run
}

Class Test/synack2_fulltcp -superclass TestSuite
Test/synack2_fulltcp instproc init {} {
        $self instvar net_ test_ guide_ 
        set net_        net2A-lossy
        set test_       synack2_fulltcp_
        set guide_      "SYN/ACK packet marked, FullTCP."
        Agent/TCPSink set ecn_syn_ true
	Agent/TCP/FullTcp set ecn_syn_ true
        $self next pktTraceFile
}
Test/synack2_fulltcp instproc run {} {
        global quiet wrap wrap1
        $self instvar ns_ guide_ node_ guide_ testName_
        puts "Guide: $guide_"
        Agent/TCP set ecn_ 1
        $self setTopo

        # Set up forward TCP connection
        set wrap $wrap1
        set tcp1 [new Agent/TCP/FullTcp/Sack]
        set sink [new Agent/TCP/FullTcp/Sack]
        $ns_ attach-agent $node_(s1) $tcp1
        $ns_ attach-agent $node_(s4) $sink
        $tcp1 set fid_ 0
        $sink set fid_ 0
        $ns_ connect $tcp1 $sink
        $sink listen ; # will figure out who its peer is
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.00 "$ftp1 produce 5"
        set ftp2 [$sink attach-app FTP]
        $ns_ at 0.03 "$ftp2 produce 20"

        $self mark_pkt 1
        # $self mark_pkt 10
        $self tcpDump $tcp1 5.0
        $ns_ at 2.0 "$self cleanupAll $testName_"
        $ns_ run
}

Class Test/synack2a_fulltcp -superclass TestSuite
Test/synack2a_fulltcp instproc init {} {
        $self instvar net_ test_ guide_ 
        set net_        net2A-lossy
        set test_       synack2a_fulltcp_
        set guide_      "SYN/ACK packet marked, FullTCP, wait."
        Agent/TCPSink set ecn_syn_ true
	Agent/TCP/FullTcp set ecn_syn_ true
	Agent/TCP/FullTcp set ecn_syn_wait_ true
        $self next pktTraceFile
}
Test/synack2a_fulltcp instproc run {} {
        global quiet wrap wrap1
        $self instvar ns_ guide_ node_ guide_ testName_
        puts "Guide: $guide_"
        Agent/TCP set ecn_ 1
        $self setTopo

        # Set up forward TCP connection
        set wrap $wrap1
        set tcp1 [new Agent/TCP/FullTcp/Sack]
        set sink [new Agent/TCP/FullTcp/Sack]
        $ns_ attach-agent $node_(s1) $tcp1
        $ns_ attach-agent $node_(s4) $sink
        $tcp1 set fid_ 0
        $sink set fid_ 0
        $ns_ connect $tcp1 $sink
        $sink listen ; # will figure out who its peer is
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.00 "$ftp1 produce 5"
        set ftp2 [$sink attach-app FTP]
        $ns_ at 0.03 "$ftp2 produce 20"

        $self mark_pkt 1
        # $self mark_pkt 10
        $self tcpDump $tcp1 5.0
        $ns_ at 2.0 "$self cleanupAll $testName_"
        $ns_ run
}
# time 0: SYN packet sent by A
# time 0.027277: SYN/ACK packet sent by B
# time 0.032309: SYN/ACK packet marked
# time 0.054555: ACK packet sent by A
# time 0.054555: two request data packets sent by A
# time 0.08558: ACK packets sent from B
# time 0.112858: three data packets sent by A
# time 0.146923: ACK packets sent by B
# time 0.231832: data packets sent by B

TestSuite runTest

# E: Congestion Experienced in IP header.
# N: ECN-Capable-Transport (ECT) in IP header.
# C: ECN-Echo in TCP header.
# A: Congestion Window Reduced (CWR) in TCP header. 
