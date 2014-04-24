#! /usr/bin/perl

use pdq;


$numIter 	= 100;
$numBank 	= 8;

$utilBus 	= 0;

@visitCnt 	= (0,0,0,0,0,0,0,0);
@utilBank 	= (0,0,0,0,0,0,0,0);
@servDemBank 	= (0,0,0,0,0,0,0,0);
@bankName	=("bnk0","bnk1","bnk2","bnk3","bnk4","bnk5","bnk6","bnk7");
@jobType	=("read","write"); 	#read, write
@thinkTimes 	= (1500.00,1800);	#read, write
@numJobs	= (32,32);		#read, write
@thruput 	= (0,0);

$qTime 		= 2;
$servTime 	= 8;

pdq::Init("Bank Contention Algo");

for( my $i = 0; $i <$numBank; $i++){
	pdq::CreateNode($bankName[$i],$pdq::CEN,$pdq::FCFS);
}
for( my $job = 0;$job<=1;$job++){

	@utilBank 	= (0,0,0,0,0,0,0,0);
	@servDemBank 	= (0,0,0,0,0,0,0,0);
	$thruput[$job] = 0.00;

	if($job == 0){	#read
		@visitCnt 	= (1,2,3,4,5,6,7,8);
	}else{	#write
		@visitCnt 	= (11,121,13,14,15,16,171,18);
	}
	pdq::CreateClosed($jobType[$job], $pdq::TERM,$numJobs[$job],$thinkTimes[$job]);
	for( my $i = 0; $i <$numIter; $i++){

		$utilBus =0.00;
		for( my $j =0; $j < $numBank; $j++){
			$utilBank[$j] = $thruput[$job] * $visitCnt[$j];
			$utilBus += $utilBank[$j];
		}	
	
		for( my $j =0; $j < $numBank; $j++){
			$servDemBank[$j] = $visitCnt[$j]*($qTime+$servTime);
			$servDemBank[$j] = $servDemBank[$j]*(1.00 - $utilBank[$j]); 
			$servDemBank[$j] = $servDemBank[$j]/(1.00 - $utilBus); 
			pdq::SetDemand($bankName[$j],$jobType[$job],$servDemBank[$j]);
			#pdq::SetDemand("bnk0","rad",0.20);
		}	
		pdq::Solve($pdq::EXACT);
		$thruput[$job] = pdq::GetThruput($pdq::TERM, $jobType[$job]);
		print "[" . $job . "]" . $thruput[$job] . "\n"
	}

}

print "--------------------------------\nread:\t" . $thruput[0] . "\twrite:\t" . $thruput[1] ."\n";
