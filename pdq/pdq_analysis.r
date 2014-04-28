args <- commandArgs(TRUE)
dir <- args[1]
Interval <- strtoi(args[2])
setwd(dir)
require('pdq')
require('methods')

rdJobCounts = read.table("rdJobCount.log",col.names=c("rdJobCount"))
rdJobCounts = max(rdJobCounts,rep(1,nrow(rdJobCounts)))
wrJobCounts = read.table("wrJobCount.log",col.names=c("wrJobCount"))
wrJobCounts = max(wrJobCounts,rep(1,nrow(wrJobCounts)))
rdServTimes = read.table("rdServTime.log",col.names=c("rdServTime"))
wrServTimes = read.table("wrServTime.log",col.names=c("wrServTime"))

rdAccessTimes = read.table("rdAccessTime.log",col.names=c("rdAccessTime"))
wrAccessTimes = read.table("wrAccessTime.log",col.names=c("wrAccessTime"))

rdNums = read.table("rdNum.log",col.names=c("rdNum"))
wrNums = read.table("wrNum.log",col.names=c("wrNum"))

rdThinkTimes = (rdJobCounts*Interval - rdNums * rdAccessTimes)/rdNums
colnames(rdThinkTimes)=c("rdThinkTimes")
wrThinkTimes = (wrJobCounts*Interval - wrNums * wrAccessTimes)/wrNums
colnames(wrThinkTimes)=c("wrThinkTimes")

interval_data=data.frame(rdJobCounts,wrJobCounts,rdServTimes,wrServTimes,rdThinkTimes,wrThinkTimes)

analyze_interval <- function( params ){
	rdJobCount <- params[1][[1]]
	wrJobCount <- params[2][[1]]
	rdServTime <- params[3][[1]]
	wrServTime <- params[4][[1]]
	rdThinkTime <- params[5][[1]]
	wrThinkTime <- params[6][[1]]
	Init("dram queue")
	CreateNode("dram",CEN,FCFS)
	CreateClosed("read",TERM,max(rdJobCount,1),rdThinkTime)
	SetDemand("dram","read",rdServTime)
	CreateClosed("write",TERM,max(wrJobCount,1),wrThinkTime)
	SetDemand("dram","write",wrServTime)
	Solve(EXACT)
	res <- c(GetResponse(TERM,"read"),GetResponse(TERM,"write"),GetQueueLength("dram","read",TERM),GetQueueLength("dram","write",TERM),GetThruput(TERM,"read"),GetThruput(TERM,"write"),GetUtilization("dram","read",TERM),GetUtilization("dram","write",TERM))
	return(res)

}

rdRespTimes <- vector("list",nrow(interval_data))
wrRespTimes <- vector("list",nrow(interval_data))
rdQOccs <- vector("list",nrow(interval_data))
wrQOccs <- vector("list",nrow(interval_data))
rdThrus <- vector("list",nrow(interval_data))
wrThrus <- vector("list",nrow(interval_data))
rdUtils <- vector("list",nrow(interval_data))
wrUtils <- vector("list",nrow(interval_data))
for (index in 1:nrow(interval_data)){
	params <- interval_data[index,]
	res <- analyze_interval(params)
	rdRespTimes[[index]] <- res[1]
	wrRespTimes[[index]] <- res[2]
	rdQOccs[[index]] <- res[3]
	wrQOccs[[index]] <- res[4]
  rdThrus[[index]] <- res[5]
  wrThrus[[index]] <- res[6]
  rdUtils[[index]] <- res[7]
  wrUtils[[index]] <- res[8]
}
rdRespTimes <- data.frame(unlist(rdRespTimes))
wrRespTimes <- data.frame(unlist(wrRespTimes))
rdQOccs <- data.frame(unlist(rdQOccs))
wrQOccs <- data.frame(unlist(wrQOccs))
rdThrus <- data.frame(unlist(rdThrus))
wrThrus <- data.frame(unlist(wrThrus))
rdUtils <- data.frame(unlist(rdUtils))
wrUtils <- data.frame(unlist(wrUtils))

perf_params <- data.frame(rdThrus,wrThrus,rdUtils,wrUtils,rdQOccs,wrQOccs,rdRespTimes,wrRespTimes)
colnames(perf_params) <- c("pdq_rdThru","pdq_wrThru","pdq_rdUtil","pdq_wrUtil","pdq_rdQOcc","pdq_wrQOcc","pdq_rdResp","pdq_wrResp")
print(perf_params)

rdRespTimesCmp=data.frame(rdRespTimes,rdAccessTimes)
wrRespTimesCmp=data.frame(wrRespTimes,wrAccessTimes)
