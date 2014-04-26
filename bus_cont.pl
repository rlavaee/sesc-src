#! /usr/bin/perl

use pdq;

$numIter 	= 50;
$numBank 	= 8;
$numPhases = 0;
$phaseNum = $ARGV[0];


$utilBus 	= 0;
@rdTimeArr;
@wrTimeArr;
@wrThinkTimeArr;
@rdThinkTimeArr;
@rdVisitCntArr;
@wrVisitCntArr;
@rdServTimeArr;
@wrServTimeArr;

@visitCnt 	= (0,0,0,0,0,0,0,0);
@tempRdVisitCnt 	= (0,0,0,0,0,0,0,0);
@tempWrVisitCnt 	= (0,0,0,0,0,0,0,0);
@utilBank 	= (0,0,0,0,0,0,0,0);
@servDemBank 	= (0,0,0,0,0,0,0,0);
@bankName	=("bnk0","bnk1","bnk2","bnk3","bnk4","bnk5","bnk6","bnk7");
@jobType	=("read","write"); 	#read, write
@thinkTimes 	= (1500.00,1800);	#read, write
@numJobs	= (32,32);		#read, write
@thruput 	= (0,0);

$qTime 		= 2;
$servTime 	= 8;
$rowIdx =0;
pdq::Init("Bank Contention Algo");

for( my $i = 0; $i <$numBank; $i++){
	pdq::CreateNode($bankName[$i],$pdq::CEN,$pdq::FCFS);
}


for( my $job = 0;$job<=1;$job++){

	for(my $bankIdx =0 ; $bankIdx <$numBank; $bankIdx++){
		$rowIdx = 0;
		
		if($job == 0){ #read	
			$fileName = "rdCntBank" . $bankIdx . ".log";
#			print "reads.... " . $fileName . "\n";
			$rowIdx = 0;
			open (MYFILE, $fileName); 
			while(<MYFILE>){
				chomp;	
				$rdTimeArr[$bankIdx][$rowIdx] = $_;
#				print $rdTimeArr[$bankIdx][$rowIdx];
				$rowIdx +=1;
			}
			close (MYFILE);
		}
		if($job == 1){ #write	
			$fileName = "wrCntBank" . $bankIdx . ".log";
#			print "writes.... " . $fileName . "\n";
			$rowIdx = 0;
			open (MYFILE, $fileName); 
			while(<MYFILE>){
				chomp;
				$wrTimeArr[$bankIdx][$rowIdx] = $_;
#				print $wrTimeArr[$bankIdx][$rowIdx];
				$rowIdx +=1;
			}
			close (MYFILE);
		}
	}
}
#reading think and serv times 
$numPhases = $rowIdx;
$rowIdx = 0;
open(MYFILE, "wrThinkTime.log");
while(<MYFILE>){
	chomp;
	$wrThinkTimeArr[$rowIdx] = $_;
	$rowIdx +=1;
}
$rowIdx = 0;
open(MYFILE, "rdThinkTime.log");
while(<MYFILE>){
	chomp;
	$rdThinkTimeArr[$rowIdx] = $_;
	$rowIdx +=1;
}
$rowIdx = 0;
open(MYFILE, "rdServTime.log");
while(<MYFILE>){
	chomp;
	$rdServTimeArr[$rowIdx] = $_;
	$rowIdx +=1;
}
$rowIdx = 0;
open(MYFILE, "wrServTime.log");
while(<MYFILE>){
	chomp;
	$wrServTimeArr[$rowIdx] = $_;
	$rowIdx +=1;
}
$rowIdx = 0;


############################################################3
#calculating visit counts (by averages)
for ($rowIdx = 0; $rowIdx < $numPhases; $rowIdx++){
	my $totalVisits = 0;
	for(my $bnkI = 0; $bnkI <$numBank ; $bnkI ++){
		$totalVisits+=$rdTimeArr[$bnkI][$rowIdx];
	}
	for(my $bnkI = 0; $bnkI <$numBank ; $bnkI ++){
		$rdVisitCntArr[$bnkI][$rowIdx] = $rdTimeArr[$bnkI][$rowIdx]/$totalVisits;
		#$rdVisitCntArr[$bnkI][$rowIdx] = $rdTimeArr[$bnkI][$rowIdx];
	}
}

for ($rowIdx = 0; $rowIdx < $numPhases; $rowIdx++){
	my $totalVisits = 0;
	for(my $bnkI = 0; $bnkI <$numBank ; $bnkI ++){
		$totalVisits+=$wrTimeArr[$bnkI][$rowIdx];
	}
	for(my $bnkI = 0; $bnkI <$numBank ; $bnkI ++){
		$wrVisitCntArr[$bnkI][$rowIdx] = $wrTimeArr[$bnkI][$rowIdx]/$totalVisits;
		#$wrVisitCntArr[$bnkI][$rowIdx] = $wrTimeArr[$bnkI][$rowIdx];
	}
}

#	for(my $test=0; $test<$rowIdx; $test++){
#		print $wrVisitCntArr[4][$test] . "\n"; 
#	
#	}


#for(my $phIdx = 0; $phIdx <$numPhases; $phIdx++){

	$phIdx = $phaseNum;

	@thinkTimes 	= ($rdThinkTimeArr[$phIdx],$wrThinkTimeArr[$phIdx]);	#read, write
	
	for( my $job = 0;$job<=1;$job++){
	
		@utilBank 	= (0,0,0,0,0,0,0,0);
		@servDemBank 	= (0,0,0,0,0,0,0,0);
		$thruput[$job] = 0.00;
	
		if($job == 0){	#read
			for(my $bnkI =0; $bnkI <$numBank; $bnkI++){
				$visitCnt[$bnkI] = $rdVisitCntArr[$bnkI][$phIdx];
			}
			$servTime = $rdServTimeArr[$phIdx];
		}else{	#write
			for(my $bnkI =0; $bnkI <$numBank; $bnkI++){
				$visitCnt[$bnkI] = $wrVisitCntArr[$bnkI][$phIdx];
			}
			$servTime = $wrServTimeArr[$phIdx];
		}

		pdq::CreateClosed($jobType[$job], $pdq::TERM,$numJobs[$job],$thinkTimes[$job]);

		print "Iterating for Phase: " . $phIdx .".......\n Parameters: Job: " . $job . " servTime: " . $servTime . " thinkTime: " . $thinkTimes[$job] . " visitCounts: ";
		for(my $temp = 0; $temp <$numBank; $temp++){
			if($job==0){
				print $rdVisitCntArr[$temp][$phIdx] . ", ";
			}else{
				print $wrVisitCntArr[$temp][$phIdx] . ", ";
			}
		}
		print "\n";
		for( my $i = 0; $i <$numIter; $i++){
	
			$utilBus =0.00;
			for( my $j =0; $j < $numBank; $j++){
				$utilBank[$j] = $thruput[$job] * $visitCnt[$j];
				$utilBus += $utilBank[$j];
			}	
		
			for( my $j =0; $j < $numBank; $j++){
#				$servDemBank[$j] = $visitCnt[$j]*($qTime+$servTime);
				$servDemBank[$j] = $visitCnt[$j]*($servTime);
				$servDemBank[$j] = $servDemBank[$j]*(1.00 - $utilBank[$j]); 
				$servDemBank[$j] = $servDemBank[$j]/(1.00 - $utilBus); 
				pdq::SetDemand($bankName[$j],$jobType[$job],$servDemBank[$j]);
				#print "serveDem: " .$servDemBank[$j] . " utilBus: " .$utilBus ."\n";
			}	
			pdq::Solve($pdq::EXACT);
			$thruput[$job] = pdq::GetThruput($pdq::TERM, $jobType[$job]);
			#print "[" . $job . "]" . $thruput[$job] . "\n"
		}
			print "--Utilization-- $job --\n";
			for(my $I = 0 ; $I <$numBank; $I++){
				print "$I : $utilBank[$I]\n";
			}
	
	}
	
	print "--------------------------------\nThruput:\t read: " . $thruput[0] . "\twrite: " . $thruput[1] ."\n";
#}
