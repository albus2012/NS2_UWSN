#分析不同路由協定的效能的AWK程式
BEGIN{
	
  	highest_packet_id=0;
	sends=0;
	receives=0;
	first_received_time=0;
	first=0;
	end=0;
	total=0;
	loss=0;
}

{
	action = $1;
	time = $2;
	trace = $4;
	packet_id = $6;
	type = $7;
	pkt =$8;
	
	if(action=="s" || action== "r" || action=="f" )
	{  
	    if(action=="s"&&trace=="AGT"&&type=="cbr")
            	 {sends++;}
    	  
     	   if(packet_id > highest_packet_id) 
            	{highest_packet_id = packet_id;}
       
           if(start_time[packet_id] == 0) 
                {start_time[packet_id] = time;}
                           
	   if (action =="r" && trace== "AGT" && type== "cbr")
	    {
	    	if(first==0)
		{  first_received_time= time;
	    	   first=1;}
		receives++;
		total += pkt *8 ;
		end_time[packet_id] = time;
		end= time;
	    }
	    else 
	    	end_time[packet_id] = -1;
  	}
}

END{
	printf("highest_packet_id=%d\n", highest_packet_id);
	packet_duration=0;
	end_to_end_delay=0;
         for (packet_id = 0; packet_id < highest_packet_id ; packet_id++) {
           packet_duration = end_time[packet_id] - start_time[packet_id];
          # printf("packet_duration=%f\n", packet_duration);
          if (packet_duration >= 0.0) 
           	{end_to_end_delay = end_to_end_delay + packet_duration;
          }
		#printf("end_to_end_delay=%f\n", end_to_end_delay);

         }
   	avg_end_to_end_delay = end_to_end_delay / receives;			      printf("total througput: %f bps \n", total);
 	total = total / (end - first_received_time);         
        loss = (sends-receives)/(sends)*100;				
         printf(" Total packet sends: %d \n", sends);
         printf(" Total packet receives: %d \n", receives);
         printf(" Packet delivery loss: %s \n", loss);
         printf(" Average End-to-End delay:%f s \n" , avg_end_to_end_delay);
         printf(" avg througput:%f bps/s\n", total);     
}
