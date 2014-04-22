require('pdq')

wrJobCounts = read.table("wrJobCount.log",col.names=c("wrJobCount"))
wrThinkTimes = read.table("wrThinkTime.log",col.names=c("wrThinkTime"))
rdThinkTimes = read.table("rdThinkTime.log",col.names=c("rdThinkTime"))
wrServTimes = read.table("wrServTime.log",col.names=c("wrServTime"))
rdServTimes = read.table("rdServTime.log",col.names=c("rdServTime"))

rdServTimes = read.table("rdServTime.log",col.names=c("rdServTime"))
interval_data = data.frame(wrJobCounts,rdThinkTimes,wrThinkTimes,rdServTimes,wrServTimes)

analyze_interval <- function( params ){
        wrJobCount <- params[1][[1]]
        rdThinkTime <- params[2][[1]]
        wrThinkTime <- params[3][[1]]
        rdServTime <- params[4][[1]]
        wrServTime <- params[5][[1]]
        Init("dram queue")
        CreateClosed("read",TERM,32,rdThinkTime)
        CreateClosed("write",TERM,wrJobCount,wrThinkTime)
        CreateNode("dram",CEN,FCFS)
        SetDemand("dram","read",rdServTime)
        SetDemand("dram","write",wrServTime)
        Solve(EXACT)
        res <- c(GetResponse(TERM,"read"),GetResponse(TERM,"write"),Get)
        return(res)
}

rdRespTimes <- vector("list",nrow(interval_data))
wrRespTimes <- vector("list",nrow(interval_data))
for (index in 1:nrow(interval_data)){
        params <- interval_data[index,]
        res <- analyze_interval(params)
        rdRespTimes[[index]] <- res[1]
        wrRespTimes[[index]] <- res[2]
}
rdRespTime <- apply(data.frame(unlist(rdRespTimes)),2,mean)
wrRespTime <- apply(data.frame(unlist(wrRespTimes)),2,mean)
