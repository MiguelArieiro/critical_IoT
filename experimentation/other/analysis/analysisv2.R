library(data.table)
library(ggplot2)
library(Rmisc)

rm(list=ls())
source("Header.R")
source("Header_graphs.R")

version <- 1.1
max_STAs <- 100
max_APs <- 2
dir_res <- "./ns-allinone-3.32/ns-3.32/xres/"
dir_base <- "./ns-allinone-3.32/ns-3.32"
dir_exp <- "e"
namesColFlow <- c("FlowID", "Protocol", "SourceAddress", "OutputAddress", "TXBitrate", "RXBitrate", "MeanDelay", "PacketLossRatio")

files_signalDBM <- "FIsignal.csv"
files_noiseDBM <- "FInoise.csv"
files_energyConsumed <- "FIenergyConsumed.csv"
files_energyRemaining <- "FIenergyRemaining.csv"
file_flow <- "flow_output.txt"

data_signalDBM <- NULL
data_noiseDBM <- NULL
data_consumedEnergy <- NULL
data_remainEnergy <- NULL
data_flow <- NULL

for (i in seq(1, 167)){
  ie <- paste0('0',i)
  #idExp = dir_exp + '01'
  idExp <- paste0(dir_exp , ie)
  file_read_through_flow <- paste0( dir_res , idExp , '/' )
  print(file_read_through_flow)
  
  a_files_ <- list.files(file_read_through_flow)
  
  for (a_fil in a_files_){
    a_f <- paste0(file_read_through_flow, "/", a_fil)
    print(a_f)
    if (grepl(file_flow, a_f, fixed=TRUE)){
      df_results <- f_aux_read_csv(a_f, header=TRUE, cols=namesColFlow)
      dt_temp<-data.table(df_results)
      dt_temp$experiment <- idExp
      dt_temp$file <- a_fil
      data_flow <- rbind(data_flow, dt_temp)
      rm (df_results)
    }
    
    if (grepl(files_signalDBM, a_f, fixed = TRUE)){
      df_results <- f_aux_read_csv(a_f, header=FALSE,cols=c('time', 'nodeId', 'signalDBM'))
      dt_temp<-data.table(df_results)
      dt_aggr <- dt_temp[,summaryfun(signalDBM), by=nodeId]
      dt_aggr$experiment <- idExp
      dt_aggr$file <- a_fil
      data_signalDBM <- rbind(data_signalDBM, dt_aggr)
      rm(df_results)
    }
    
    if (grepl(files_noiseDBM, a_f, fixed = TRUE)){
      df_results <- f_aux_read_csv(a_f, header=FALSE,cols=c('time','nodeId', 'noiseDBM'))
      dt_temp<-data.table(df_results)
      dt_aggr <- dt_temp[,summaryfun(noiseDBM), by=nodeId]
      dt_aggr$experiment <- idExp
      dt_aggr$file <- a_fil
      
      data_noiseDBM <- rbind(data_noiseDBM, dt_aggr)
      rm(df_results)
    }
    
    if (grepl(files_energyConsumed, a_f, fixed = TRUE)){
      df_results <- f_aux_read_csv(a_f, header=FALSE,cols=c('time', 'nodeId', 'energyConsumed'))
      dt_temp<-data.table(df_results)
      dt_aggr <- dt_temp[,summaryfun(energyConsumed), by=nodeId]
      dt_aggr$experiment <- idExp
      dt_aggr$file <- a_fil
      
      data_consumedEnergy <- rbind(data_consumedEnergy, dt_aggr)
      rm(df_results)
    }
    
    if (grepl(files_energyRemaining, a_f, fixed = TRUE)){
      df_results <- f_aux_read_csv(a_f, header=FALSE,cols=c('time', 'nodeId', 'energyRemaining'))
      dt_temp<-data.table(df_results)
      dt_aggr <- dt_temp[,summaryfun(energyRemaining), by=nodeId]
      dt_aggr$experiment <- idExp
      dt_aggr$file <- a_fil
      
      data_remainEnergy <- rbind(data_remainEnergy, dt_aggr)
      rm(df_results)
    }
  }
}

save(list=ls(), file= paste0("resdata_", version, ".data"))

