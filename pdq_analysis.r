require('pdq')
Interval <- 10000*1000

wrJobCounts = read.table("wrJobCount.log",col.names=c("wrJobCount"))
rdJobCounts = read.table("rdJobCount.log",col.names=c("rdJobCount"))
#rdJobCounts = rep(32,30)
#rdThinkTimes = read.table("rdThinkTime.log",col.names=c("rdThinkTime"))
#wrThinkTimes = read.table("wrThinkTime.log",col.names=c("wrThinkTime"))
wrServTimes = read.table("wrServTime.log",col.names=c("wrServTime"))
rdServTimes = read.table("rdServTime.log",col.names=c("rdServTime"))
rdServTimes = read.table("rdServTime.log",col.names=c("rdServTime"))

rdAccessTimes = read.table("rdAccessTime.log",col.names=c("rdAccessTime"))
wrAccessTimes = read.table("wrAccessTime.log",col.names=c("wrAccessTime"))


rdNums = read.table("rdNum.log",col.names=c("rdNum"))
wrNums = read.table("wrNum.log",col.names=c("wrNum"))

rdThinkTimes = (rdJobCounts*Interval - rdNums * rdAccessTimes)/rdNums
wrThinkTimes = (wrJobCounts*Interval - wrNums * rdAccessTimes)/rdNums

interval_data=data.frame(rdJobCounts,wrJobCounts,rdThinkTimes,wrThinkTimes,rdServTimes,wrServTimes)

analyze_interval <- function( params ){
	rdJobCount <- params[1][[1]]
	wrJobCount <- params[2][[1]]
	rdThinkTime <- params[3][[1]]
	wrThinkTime <- params[4][[1]]
	rdServTime <- params[5][[1]]
	wrServTime <- params[6][[1]]
	show(wrThinkTime)
	Init("dram queue")
	CreateClosed("read",TERM,rdJobCount,as.numeric(rdThinkTime))
	CreateClosed("write",TERM,wrJobCount,as.numeric(wrThinkTime))
	CreateNode("dram",CEN,FCFS)
	SetDemand("dram","read",rdServTime)
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



rdRespTime <- colSums(rdRespTimes*rdNums) / colSums(rdNums)
wrRespTime <- colSums(wrRespTimes*wrNums) / colSums(wrNums)
rdQOcc <- apply(rdQOccs,2,mean)
wrQOcc <- apply(wrQOccs,2,mean)
QOcc <- rdQOcc + wrQOcc

rdThru <- apply(rdThrus,2,mean)
wrThru <- apply(wrThrus,2,mean)
Thru <- rdThru + wrThru

rdUtil <- apply(rdUtils,2,mean)
wrUtil <- apply(wrUtils,2,mean)
Util <- rdUtil + wrUtil

wrRespTimesCmp <- data.frame(wrRespTimes,wrAccessTimes)
rdRespTimesCmp <- data.frame(rdRespTimes,rdAccessTimes)
show(rdRespTimesCmp)
show(wrRespTimesCmp)
show(rdThrus)
show(wrThrus)
show(rdUtils)
show(wrUtils)
show(rdThru)
show(wrThru)
show(rdUtil)
show(wrUtil)


