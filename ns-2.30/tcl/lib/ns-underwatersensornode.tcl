# This program is copied from ns-mobilenode.tcl. I modified part of codes 
#specially for underwater sensor node


# The following setups up link layer, mac layer, network interface
# and physical layer structures for the mobile node.
#
Node/MobileNode/UnderwaterSensorNode instproc add-interface { channel pmodel lltype mactype \
		qtype qlen iftype anttype topo inerrproc outerrproc fecproc} {
	$self instvar arptable_ nifs_ netif_ mac_ ifq_ ll_ imep_ inerr_ outerr_ fec_
	
	set ns [Simulator instance]
	set imepflag [$ns imep-support]
	set t $nifs_
	incr nifs_

	set netif_($t)	[new $iftype]		;# interface
	set mac_($t)	[new $mactype]		;# mac layer
	set ifq_($t)	[new $qtype]		;# interface queue
	set ll_($t)	[new $lltype]		;# link layer
        set ant_($t)    [new $anttype]
       

        # puts "this is a underwater sensor node"
	$ns mac-type $mactype
	set inerr_($t) ""
	if {$inerrproc != ""} {
		set inerr_($t) [$inerrproc]
	}
	set outerr_($t) ""
	if {$outerrproc != ""} {
		set outerr_($t) [$outerrproc]
	}
	set fec_($t) ""
	if {$fecproc != ""} {
		set fec_($t) [$fecproc]
	}

	set namfp [$ns get-nam-traceall]
        if {$imepflag == "ON" } {              
		# IMEP layer
		set imep_($t) [new Agent/IMEP [$self id]]
		set imep $imep_($t)
		set drpT [$self mobility-trace Drop "RTR"]
		if { $namfp != "" } {
			$drpT namattach $namfp
		}
		$imep drop-target $drpT
		$ns at 0.[$self id] "$imep_($t) start"   ;# start beacon timer
        }
	#
	# Local Variables
	#
	set nullAgent_ [$ns set nullAgent_]
	set netif $netif_($t)
	set mac $mac_($t)
	set ifq $ifq_($t)
	set ll $ll_($t)

	set inerr $inerr_($t)
	set outerr $outerr_($t)
	set fec $fec_($t)

	#
	# Initialize ARP table only once.
	#
	if { $arptable_ == "" } {
		set arptable_ [new ARPTable $self $mac]
		# FOR backward compatibility sake, hack only
		if {$imepflag != ""} {
			set drpT [$self mobility-trace Drop "IFQ"]
		} else {
			set drpT [cmu-trace Drop "IFQ" $self]
		}
		$arptable_ drop-target $drpT
		if { $namfp != "" } {
			$drpT namattach $namfp
		}
        }
	#
	# Link Layer
	#
	$ll arptable $arptable_
	$ll mac $mac

	$ll down-target $ifq

	if {$imepflag == "ON" } {
		$imep recvtarget [$self entry]
		$imep sendtarget $ll
		$ll up-target $imep
        } else {
		
                $ll up-target [$self entry]
                #puts "ns-mobile: setup uptarget..."
               # puts "the uptarget is [$self enrty]" 	
}
	#
	# Interface Queue
	#
	$ifq target $mac
	$ifq set limit_ $qlen
	if {$imepflag != ""} {
		set drpT [$self mobility-trace Drop "IFQ"]
	} else {
		set drpT [cmu-trace Drop "IFQ" $self]
        }
	$ifq drop-target $drpT
	if { $namfp != "" } {
		$drpT namattach $namfp
	}
	#
	# Mac Layer
	#


	$mac netif $netif
	$mac up-target $ll
        

        #added by Peng Xie       

 #       puts "before put on node\n"
 
          $mac node_on $self

  #      puts "after put on node\n"

	if {$outerr == "" && $fec == ""} {
		$mac down-target $netif
	} elseif {$outerr != "" && $fec == ""} {
		$mac down-target $outerr
		$outerr target $netif
	} elseif {$outerr == "" && $fec != ""} {
		$mac down-target $fec
		$fec down-target $netif
	} else {
		$mac down-target $fec
		$fec down-target $outerr
		$err target $netif
	}

	set god_ [God instance]
        if {$mactype == "Mac/802_11"} {
		$mac nodes [$god_ num_nodes]
	}
       

	#
	# Network Interface
	#
	#if {$fec == ""} {
        #		$netif up-target $mac
	#} else {
        #		$netif up-target $fec
	#	$fec up-target $mac
	#}

	$netif channel $channel
	if {$inerr == "" && $fec == ""} {
		$netif up-target $mac
	} elseif {$inerr != "" && $fec == ""} {
		$netif up-target $inerr
		$inerr target $mac
	} elseif {$err == "" && $fec != ""} {
		$netif up-target $fec
		$fec up-target $mac
	} else {
		$netif up-target $inerr
		$inerr target $fec
		$fec up-target $mac
	}

	$netif propagation $pmodel	;# Propagation Model
	$netif node $self		;# Bind node <---> interface

       
         #wireless phy doesn't need antenna anymore
	 $netif antenna $ant_($t)
        # puts " after put on antenna \n"   
	#
	# Physical Channel
	#
	$channel addif $netif
	
            

        # List-based improvement
	# For nodes talking to multiple channels this should
	# be called multiple times for each channel
	$channel add-node $self		

	# let topo keep handle of channel
	$topo channel $channel
	# ============================================================

	if { [Simulator set MacTrace_] == "ON" } {
		#
		# Trace RTS/CTS/ACK Packets
		#
		if {$imepflag != ""} {
			set rcvT [$self mobility-trace Recv "MAC"]
		} else {
			set rcvT [cmu-trace Recv "MAC" $self]
		}
		$mac log-target $rcvT
		if { $namfp != "" } {
			$rcvT namattach $namfp
		}
		#
		# Trace Sent Packets
		#
		if {$imepflag != ""} {
			set sndT [$self mobility-trace Send "MAC"]
		} else {
			set sndT [cmu-trace Send "MAC" $self]
		}
		$sndT target [$mac down-target]
		$mac down-target $sndT
		if { $namfp != "" } {
			$sndT namattach $namfp
		}
		#
		# Trace Received Packets
		#
		if {$imepflag != ""} {
			set rcvT [$self mobility-trace Recv "MAC"]
		} else {
			set rcvT [cmu-trace Recv "MAC" $self]
		}
		$rcvT target [$mac up-target]
		$mac up-target $rcvT
		if { $namfp != "" } {
			$rcvT namattach $namfp
		}
		#
		# Trace Dropped Packets
		#
		if {$imepflag != ""} {
			set drpT [$self mobility-trace Drop "MAC"]
		} else {
			set drpT [cmu-trace Drop "MAC" $self]
		}
		$mac drop-target $drpT
		if { $namfp != "" } {
			$drpT namattach $namfp
		}
	} else {
		$mac log-target [$ns set nullAgent_]
		$mac drop-target [$ns set nullAgent_]
	}

# change wrt Mike's code
       if { [Simulator set EotTrace_] == "ON" } {
               #
               # Also trace end of transmission time for packets
               #

               if {$imepflag != ""} {
                       set eotT [$self mobility-trace EOT "MAC"]
               } else {
                       set eoT [cmu-trace EOT "MAC" $self]
               }
               $mac eot-target $eotT
       }



	# ============================================================

	$self addif $netif
}

