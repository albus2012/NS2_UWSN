#分析不同路由協定的效能的AWK程式
BEGIN{
	
  	highest_packet_id=0;
        sendoutnum=0;
	recvnum=0;
	sendnum=0;
	sendackdata=0;
	recvackdata=0;
}

{
	val2 = $2;
	val3 = $3;
	val4 = $4;
        val5 = $5;
	if(val2 == "NO")
          NO = val4;
	if(val2 == "rate")
	{  
            rate = val4;
	    no[rate] = NO+1;
  	}
        if (val3 == "sends")
          {
          
          send[rate] += val5;
          }

        if (val3 == "receives")
          {
          
          recv[rate] += val5;
          }
        if (val3 == "loss")
          {
          
          loss[rate] += val5;
          }
       if (val3 == "delay")
          {
          delay[rate] += val5;
          }

       if (val2 == "ei")
          {
          ei[rate] += val4;
          }
        if (val2 == "et")
          {
          et[rate] += val4;
          }
        if (val2 == "er")
          {
          er[rate] += val4;
          }
       


 }

END{
	for(rate =2; rate < 12; rate++)
	{
	printf("rate is %d\n", rate);
	printf("er is %d\n", er[rate]/no[rate]);
	printf("ei is %f\n", ei[rate]/no[rate]);
	printf("et is %f\n", et[rate]/no[rate]);

	printf("send is %f\n", send[rate]/no[rate]);
	printf("recv is %f\n", recv[rate]/no[rate]);
	printf("loss is %f\n", loss[rate]/no[rate]);
	printf("delay is %f\n", delay[rate]/no[rate]);
        printf("\n");
	}
}
