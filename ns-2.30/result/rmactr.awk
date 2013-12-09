BEGIN{
	
  	highest_packet_id=0;
	sends=0;
	receives=0;
	first_received_time=0;
	first=0;
	end=0;
	total=0;
	loss=0;
        eiall = 0;
	etall = 0;
	erall = 0;
}

{
	action = $1;
	time = $2;
	node_id = $3;
	layer = $4;
	packet_id = $6;
	type = $7;
	pkt =$8;
	
	if(action=="s" || action== "r" || action=="f" )
	{  
            ei[node_id] = $15;
	    et[node_id] = $19;
	    er[node_id] = $21;
	    if(action=="r"&&layer=="RTR"&&type=="vectorbasedforward")
            	 {sends++;}
    	  
     	   if(packet_id > highest_packet_id) 
            	{highest_packet_id = packet_id;}
     	   if(node_id > highest_node_id) 
            	{highest_node_id = node_id;}

       
           if(start_time[packet_id] == 0) 
                {start_time[packet_id] = time;}
                           
	   if (action =="r" && layer== "RTR" && type== "DATA")
	    {
	    	if(first==0)
		{  first_received_time= time;
	    	   first=1;}
		receives++;
		total += pkt *8 ;
		end_time[packet_id] = time;
		end= time;
	    }
	    #else 
	    #	end_time[packet_id] = -1;
  	}
}

END{
	printf("highest_packet_id= %d \n", highest_packet_id);
	packet_duration=0;
	end_to_end_delay=0;
        for (packet_id = 0; packet_id < highest_packet_id ; packet_id++) 
	{
          packet_duration = end_time[packet_id] - start_time[packet_id];
	  if(packet_duration < 0)
           ;# printf("packet_duration< 0 id=%f\n", packet_id);
          if (packet_duration >= 0.0) 
          {
	    end_to_end_delay = end_to_end_delay + packet_duration;
          }
	}
	for( node_id = 0; node_id < highest_node_id; node_id++)
	{
          eiall += ei[node_id];
	  etall += et[node_id];
   	  erall += er[node_id];
         }

   	avg_end_to_end_delay = end_to_end_delay / receives;	
	#printf("total througput: %f bps \n", total);
 	total = total / (end - first_received_time);         
        loss = (sends-receives)/(sends)*100;				
         printf("Total packet sends : %d \n", sends);
         printf("Total packet receives : %d \n", receives);
         printf("Packet delivery loss : %s \n", loss);
         printf("Average End-to-End delay : %f  \n" , avg_end_to_end_delay);
        # printf("avg througput:%f bps/s\n", total); 
         printf("total ei : %f \n", eiall); 
	 printf("total et : %f \n", etall); 
	 printf("total er : %f \n", erall); 
         printf("______________________\n\n\n");
}
