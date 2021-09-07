#require("Hmisc")
require("ggplot2")
require("plyr")
require("reshape2")
require("ggpubr")

dir_r_graphs <- paste0("./Rgraphs/", version, "/")


MyOptTHEME  <- theme( plot.title=element_text(size=14), 
                          axis.text.x = element_text(angle=0,  size=11, colour="black"), 
                          axis.text.y = element_text(angle=0,size=11, colour="black", hjust=0.9 ) , 
                          strip.text.x=element_text(size=10), 
                          strip.text.y=element_text(size=10), 
                          axis.ticks=element_line(),
                          panel.background = element_rect(fill = "white", colour = NA), 
                          panel.grid.minor = element_line(linetype = "dotted", colour="grey85", size=0.2 ),
                          panel.grid.major = element_line(linetype = "dotted", colour="grey85", size=0.2 ),
                          axis.line=element_line(),
                          axis.title.x=element_text(size=12),
                          axis.title.y=element_text(size=12, angle=90),
                          legend.position="none"
)

MyOptTHEME_grids  <- theme( plot.title=element_text(size=14), 
                      axis.text.x = element_text(angle=30,  size=10, colour="black"), 
                      axis.text.y = element_text(angle=0,size=10, colour="black", hjust=0.9 ) , 
                      strip.text.x=element_text(size=10), 
                      strip.text.y=element_text(size=10), 
                      axis.ticks=element_line(),
                      panel.background = element_rect(fill = "white", colour = NA), 
                      panel.grid.minor = element_line(linetype = "dotted", colour="grey", size=0.3 ),
                      panel.grid.major = element_line(linetype = "dotted", colour="grey", size=0.3 ),
                      axis.line=element_line(),
                      axis.title.x=element_text(size=12),
                      axis.title.y=element_text(size=12, angle=90),
                      legend.position="bottom"
)

MyOptTHEME_x90  <- theme( plot.title=element_text(size=14), 
                      axis.text.x = element_text(angle=0,  size=12, colour="black"), 
                      axis.text.y = element_text(angle=0,size=12, colour="black", hjust=0.9 ) , 
                      strip.text.x=element_text(size=10), 
                      strip.text.y=element_text(size=10), 
                      axis.ticks=element_line(),
                      panel.background = element_rect(fill = "white", colour = NA), 
                      panel.grid.minor = element_line(linetype = "dotted", colour="grey85", size=0.2 ),
                      panel.grid.major = element_line(linetype = "dotted", colour="grey85", size=0.2 ),
                      axis.line=element_line(),
                      axis.title.x=element_text(size=13),
                      axis.title.y=element_text(size=13, angle=90),
                      legend.position="bottom"
)

MyOptTHEME_x90_90  <- theme( plot.title=element_text(size=14), 
                          axis.text.x = element_text(angle=90,  size=11, colour="black"), 
                          axis.text.y = element_text(angle=0,size=11, colour="black", hjust=0.9 ) , 
                          strip.text.x=element_text(size=10), 
                          strip.text.y=element_text(size=10), 
                          axis.ticks=element_line(),
                          panel.background = element_rect(fill = "white", colour = NA), 
                          panel.grid.minor = element_line(linetype = "dotted", colour="grey85", size=0.2 ),
                          panel.grid.major = element_line(linetype = "dotted", colour="grey85", size=0.2 ),
                          axis.line=element_line(),
                          axis.title.x=element_text(size=12),
                          axis.title.y=element_text(size=12, angle=90),
                          legend.position="none"
)

MyOptTHEME_x90_90leg  <- theme( plot.title=element_text(size=14), 
                             axis.text.x = element_text(angle=90,  size=11, colour="black"), 
                             axis.text.y = element_text(angle=0,size=11, colour="black", hjust=0.9 ) , 
                             strip.text.x=element_text(size=10), 
                             strip.text.y=element_text(size=10), 
                             axis.ticks=element_line(),
                             panel.background = element_rect(fill = "white", colour = NA), 
                             panel.grid.minor = element_line(linetype = "dotted", colour="grey85", size=0.2 ),
                             panel.grid.major = element_line(linetype = "dotted", colour="grey85", size=0.2 ),
                             axis.line=element_line(),
                             axis.title.x=element_text(size=12),
                             axis.title.y=element_text(size=12, angle=90),
                             legend.position="bottom"
)

#axis.text.x = element_text(angle=90,  size=12, colour="black", face="bold"), 
#panel.grid.minor = element_line(linetype = "dotted", colour="grey85", size=0.2 ),
#panel.grid.major = element_line(linetype = "dotted", colour="grey85", size=0.2 ),
MyOptTHEME_x90Border  <- theme( plot.title=element_text(size=16, face="bold"), 
                          axis.text.x = element_text(angle=0,  size=12, colour="black", face="bold"), 
                          axis.text.y = element_text(angle=0,size=10, colour="black", hjust=0.9 ) , 
                          strip.text.x=element_text(size=10, face="bold"), 
                          strip.text.y=element_text(size=10, face="bold"), 
                          axis.ticks=element_line(),
                          panel.background = element_rect(fill = "white", colour = NA), 
                          panel.grid.minor = element_blank(),
                          panel.grid.major = element_blank(),
                          panel.border = element_rect(colour="grey", fill=NA, size=2),
                          axis.line=element_line(),
                          axis.title.x=element_text(size=12, face="bold"),
                          axis.title.y=element_text(size=12, angle=90, face="bold"),
                          legend.position="none"
)


my_y_scale <- function(val, perc=c(0,1), n_breaks=10, digits=DIGITS_GRAPHS){
  aux_lim <- quantile(val, perc)
  aux_breaks <- seq(from=aux_lim[1], to=aux_lim[2], by=(aux_lim[2]-aux_lim[1]) / n_breaks)
  return ( scale_y_continuous(limits = aux_lim,breaks = aux_breaks, labels=function(n){format(n, scientific = FALSE, digits = digits)}  ) )
}


my_y_log_scale <- function(val, perc=c(0,1), n_breaks=10, digits=DIGITS_GRAPHS){
  aux_lim <- quantile(val, perc)
  aux_breaks <- seq(from=aux_lim[1], to=aux_lim[2], by=(aux_lim[2]-aux_lim[1]) / n_breaks)
  return ( scale_y_log10(limits = aux_lim,breaks = aux_breaks, labels=function(n){format(n, scientific = FALSE, digits = digits)}  ) )
}


getOptPathBen <- function(iIter){
  aux <- dfBen[dfBen$iter==iIter,]
  
  # Get Set/Path with higher 
  # only firt three, since we have three sets from a prefered path.
  aMax <- aux[order(-aux$avPthCap)[1:3], "pIDs"]
  
  return(aMax)
  
}

getOptPathBen_pri <- function(iIter){
  aux <- dfBen[dfBen$iter==iIter,]
  
  # Get Set/Path with higher 
  # only first as it is only one IEEE 802.11n path
  aMax <- aux[order(-aux$avPthCap)[1], "pIDs"]
  
  return(aMax)
}

# Function to be called by ddply
# ddply(dfCost, .(iter), .fun=getCRR_THE)
#
getCRR_THE <- function(indf){
  aIter <- unique(indf$iter)
  
  auxPIDs <- indf[order(indf$ipdv, indf$loss, indf$reorder, indf$owd ),"pIDs"]
  aIDBenMax <- getOptPathBen(aIter)
  
  if (auxPIDs[1]==aIDBenMax[1]){
    return(data.frame(iter=aIter, pref1=auxPIDs[1], pref2=auxPIDs[2], pref3=auxPIDs[3], pref4=auxPIDs[4], pref5=auxPIDs[5], pref6=auxPIDs[6] ))
  }else{
    auxPIDsben <-auxPIDs[auxPIDs %in% aIDBenMax] 
    auxPIDscost <- auxPIDs[!auxPIDs %in% aIDBenMax]
    return(data.frame(iter=aIter, pref1=auxPIDsben[1], pref2=auxPIDsben[2], pref3=auxPIDsben[3], pref4=auxPIDscost[1], pref5=auxPIDscost[2], pref6=auxPIDscost[3] ))
  }
  
}


# Functions to save tables
#
mySaveTable<-function(inDf, namefile, iCap, iLabel){
  
  latex(inDf, rowname=NULL, file=namefile, caption=iCap, caption.loc="bottom", 
        multicol=FALSE,   label=iLabel , center="none")
  
}


# Function to save graphs
#
mySaveGraph <- function(inG,  namefile){
  
  fil_to_save <- paste(namefile, ".pdf", sep="")
  pdf( file=fil_to_save, onefile=FALSE, height=6.5, width=8, pointsize=10, paper="special", color=TRUE)
  print(inG)
  dev.off()
  
  #fil_to_save <- paste(namefile, ".png", sep="")
  #png( file=fil_to_save, height=480, width=600, pointsize=10,  color=TRUE)
  #print(inG)
  #dev.off()
}


mySaveGraphPNG <- function(inG,  namefile){
  
  fil_to_save <- paste(namefile, ".png", sep="")
  png( file=fil_to_save ,width=800,height=600)
  print(inG)
  dev.off()
}


myG2_geom_bar <- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA){
ggPer <- ggplot(indf, aes(x=method,y=Per))
ggPer <- ggPer + geom_bar(position="dodge", size=1)
ggPer <- ggPer + geom_text(aes(label=sprintf("%.2f%%",Per), size=5, y=Per+2, x=method))
ggPer <- ggPer + inOpt
ggPer <- ggPer + iTit
ggPer <- ggPer + iXlab
ggPer <- ggPer + iYlab
if (length(iYlim) > 1  ){
  ggPer <- ggPer + iYlim
}
return (ggPer)
}

myG2_geom_barEvents <- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA){
  ggPer <- ggplot(indf, aes(x=method, y=(rate/tot_method)*100, fill=path))
  ggPer <- ggPer + geom_bar(position="dodge", size=1)
  ggPer <- ggPer + geom_text(aes(label=sprintf("%.2f%%",(rate/tot_method)*100), size=5, y=(rate/tot_method)*100 +2, x=method, colour=path),position = position_dodge(width=1))
  ggPer <- ggPer + inOpt
  ggPer <- ggPer + iTit
  ggPer <- ggPer + iXlab
  ggPer <- ggPer + iYlab
  ggPer <- ggPer + theme(  legend.position="bottom")
  if (length(iYlim) > 1  ){
    ggPer <- ggPer + iYlim
  }
  return (ggPer)
}


auxStatsYmaxYmin<-function(x)
{
  result=c(mean(x)-sd(x),mean(x)+sd(x))
  names(result)=c("ymin","ymax")
  result
}

auxStatsMean<-function(x)
{
  result=sprintf("%.2f%%", mean(x))
  names(result)="mean"
  result
}

myG2_geom_bar_errorByCoS<- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA, doGrid=TRUE){

  ggPer <- ggplot(indf)
  ggPer <- ggPer + geom_bar(aes(x=method, y=mean, fill=cos), position=position_dodge()) 
  #ggPer <- ggPer + geom_bar( position=position_dodge(), size=0.4  )
  ggPer <- ggPer + geom_errorbar(aes(x=method, ymin=(mean-err), ymax=(mean+err), colour=cos), 
                               width=0.2, size=0.3, 
                               position=position_dodge(0.9)) 
  ggPer <- ggPer + geom_text(aes(label=sprintf("%.2f%%",mean), size=5, y=75, x=method, colour=cos),
                             position=position_dodge(0.9))
  ggPer <- ggPer +scale_fill_grey()
  ggPer <- ggPer +scale_colour_grey()
  ggPer <- ggPer + inOpt
  ggPer <- ggPer + theme(  legend.position="bottom")
  ggPer <- ggPer + iTit
  ggPer <- ggPer + iXlab
  ggPer <- ggPer + iYlab
  if (doGrid){
    ggPer<- ggPer + facet_grid(meth ~ .)  
  }
  if (length(iYlim) > 1  ){
    ggPer <- ggPer + iYlim
  }
  return (ggPer)
}


myG2_geom_boxplot_ByCoS<- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA){
ggPer <- ggplot(indf, aes(x=method,y=Per))
#ggPer <- ggPer + geom_boxplot(position="dodge")
ggPer <- ggPer + geom_line(aes(x=method, y=50))
ggPer <- ggPer + inOpt
ggPer <- ggPer + iTit
ggPer <- ggPer + iXlab
ggPer <- ggPer + iYlab
ggPer<- ggPer + facet_grid(cos ~ .)  
if (length(iYlim) > 1  ){
  ggPer <- ggPer + iYlim
}
return (ggPer)
}


myG2_geom_boxplot<- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA){
  ggPer <- ggplot(indf, aes(x=method_codec,y=CRR_rate))
  ggPer <- ggPer + geom_boxplot(position="dodge")
  
  ggPer <- ggPer + inOpt
  ggPer <- ggPer + iTit
  ggPer <- ggPer + iXlab
  ggPer <- ggPer + iYlab
  
  if (length(iYlim) > 1  ){
    ggPer <- ggPer + iYlim
  }
  return (ggPer)
}

myG2_geom_boxplotRHR<- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA){
  ggPer <- ggplot(indf, aes(x=method_codec,y=RHR))
  ggPer <- ggPer + geom_boxplot(position="dodge")
  ggPer <- ggPer + geom_hline(aes(y=mean(RHR_THE), x=method_codec))
  ggPer <- ggPer + inOpt
  ggPer <- ggPer + iTit
  ggPer <- ggPer + iXlab
  ggPer <- ggPer + iYlab
  
  if (length(iYlim) > 1  ){
    ggPer <- ggPer + iYlim
  }
  return (ggPer)
}

myG2_geom_boxplotMOS<- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA){
  nAlter <- length(levels(indf$method_codec)) - 3 # ignore at head zZRscore
  
  nAlter <- nAlter -3
  ggPer <- ggplot(indf[indf$method!="zZRscore",], aes(x=method_codec,y=MOS))
  ggPer <- ggPer + geom_boxplot(position="dodge")
  ggPer <- ggPer + geom_hline(data=indf[indf$method=="zZRscore",], aes(yintercept=MOS, linetype=codec))
  ggPer <- ggPer + geom_text(data=indf[indf$method=="zZRscore",], aes(x=15, y=MOS, label=method_codec), hjust=0, vjust=0, size=2)
  ggPer <- ggPer + inOpt
  ggPer <- ggPer + iTit
  ggPer <- ggPer + iXlab
  ggPer <- ggPer + iYlab
  
  if (length(iYlim) > 1  ){
    ggPer <- ggPer + iYlim
  }
  return (ggPer)
}

myG2_geom_boxplotCRRGlobal<- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA, optBY=""){
  
  indf <- indf[indf$method!="zZRscore",]
  #indf$method_codec <- gsub("METHODICAL", "EVA",indf$method_codec)
  
  #df2 <- ddply(indf, .(method_codec, JB), summarise, med=mean(CRR_rate) )
  if (optBY=="byJB"){
    df2 <- ddply(indf, .(method_codec, JB), summarise, med=mean(CRR_rate) )
  }else{
    df2 <- ddply(indf, .(method_codec, lossRA), summarise, med=mean(CRR_rate) )
  }
  
  
  ggPer <- ggplot(indf, aes(x=method_codec,y=CRR_rate))
  ggPer <- ggPer + geom_boxplot(position="dodge")
  #ggPer <- ggPer + stat_summary(fun.data=give.n, geom="text", size=2, position="dodge")
  #ggPer <- ggPer + geom_text(data=df2, aes(x=method_codec, y=med + 10, label=sprintf("%.2f%%",med) ), size=2, angle=90, position="dodge")
  ggPer <- ggPer + inOpt
  ggPer <- ggPer + iTit
  ggPer <- ggPer + iXlab
  ggPer <- ggPer + iYlab
  
  if (length(iYlim) > 1  ){
    ggPer <- ggPer + iYlim
  }
  
  if (optBY=="byJB"){
    ggPer <- ggPer + facet_grid(. ~ JB)  
  }else{
    ggPer <- ggPer + facet_grid(. ~ lossRA)  
  }
  
  return (ggPer)
}

myG2_geom_boxplotMOSGlobal<- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA, optBY=""){
  nAlter <- length(levels(indf$method_codec)) - 3 # ignore at head zZRscore
  
  #first order by day
  indf[with(indf, order(time) ),]
  
  indf <- indf[indf$method!="zZRscore",]
  #indf$method_codec <- gsub("METHODICAL", "EVA",indf$method_codec)
  
  if (optBY=="byJB"){
    df2 <- ddply(indf, .(method_codec, JB), summarise, med=mean(MOS) )
  }else{
    df2 <- ddply(indf, .(method_codec, lossRA), summarise, med=mean(MOS) )
  }
  
  nAlter <- nAlter -3
  ggPer <- ggplot(indf, aes(x=method_codec,y=MOS))
  ggPer <- ggPer + geom_boxplot(position="dodge")
  #ggPer <- ggPer + geom_text(data=df2, aes(x=method_codec, y=med + 0.5, label=sprintf("%.2f",med) ), size=2, angle=90, position="dodge")
  #ggPer <- ggPer + geom_hline(data=indf[indf$method=="zZRscore",], aes(yintercept=MOS, linetype=codec))
  
  ggPer <- ggPer + inOpt
  ggPer <- ggPer + iTit
  ggPer <- ggPer + iXlab
  ggPer <- ggPer + iYlab
  
  if (length(iYlim) > 1  ){
    ggPer <- ggPer + iYlim
  }
  if (optBY=="byJB"){
    ggPer <- ggPer + facet_grid(. ~ JB)  
  }else{
    ggPer <- ggPer + facet_grid(. ~ lossRA)  
  }
  
  return (ggPer)
}


myG2_geom_errorbarMOS<- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA, optBY="", aggr=FALSE){
  nAlter <- length(levels(indf$method_codec)) - 3 # ignore at head zZRscore
  
  #first order by day
  indf[with(indf, order(time) ),]
  
  indf <- indf[indf$method!="zZRscore",]
  fauxSummarise <- function(indf){
    #print(indf$RHR)
    MOS <- mean(indf$MOS)
    sdMOS <- sd(indf$MOS)
    #print(sdRHR)
    #RHR_THE <- mean(indf$RHR_THE)
    codec <- unique(indf$codec)
    method <- unique(indf$method)
    lossRA <- unique(indf$lossRA)
    return(data.frame(MOS=MOS, sdMOS=sdMOS, codec=codec, method=method, lossRA=lossRA))
  }
  fauxSummarise2 <- function(indf){
    #print(indf$RHR)
    MOS <- mean(indf$MOS)
    sdMOS <- sd(indf$MOS)
    #print(sdRHR)
    #RHR_THE <- mean(indf$RHR_THE)
    codec <- unique(indf$codec)
    method <- unique(indf$method)
    JB <- unique(indf$JB)
    return(data.frame(MOS=MOS, sdMOS=sdMOS, codec=codec, method=method, JB=JB))
  }
  
  if (optBY=="byJB"){
    df2 <- ddply(indf, .(method_codec, JB), .fun=fauxSummarise2) 
    df2$JB <- factor(df2$JB, labels=c("1-20ms","2-40ms","4-80ms"))
  }else{
    df2 <- ddply(indf, .(method_codec, lossRA),.fun=fauxSummarise) 
    df2$lossRA <- factor(df2$lossRA, labels=c("5%","10%","20%"))
  }
  
  nAlter <- nAlter -3
  
  
  
  
  #print(summary(df2))
  #dfCHECK <<- df2
  ggPer <- ggplot(df2, aes(x=method,y=MOS, fill=method))
  ggPer <- ggPer + geom_bar(aes(x=method,y=MOS), stat="identity")
  ggPer <- ggPer + scale_fill_brewer(palette="Dark2") # check values on R coobok page 273 pdf
  ggPer <- ggPer + geom_errorbar(aes(ymax=MOS+sdMOS, ymin=MOS-sdMOS), width=0.4)
  ggPer <- ggPer + geom_hline(yintercept=1, linetype="dashed", size=0.3) 
  ggPer <- ggPer + geom_hline(yintercept=2, linetype="dashed", size=0.3) 
  ggPer <- ggPer + geom_hline(yintercept=3, linetype="dashed", size=0.3)  
  ggPer <- ggPer + geom_hline(yintercept=4, linetype="dashed", size=0.3)   
  ggPer <- ggPer + inOpt
  ggPer <- ggPer + iTit
  ggPer <- ggPer + iXlab
  ggPer <- ggPer + iYlab
  #ggPer <- ggPer + theme(legend.position="bottom")
  
  #if (length(iYlim) > 1  ){
  #  ggPer <- ggPer + iYlim
  #}
  if (optBY=="byJB"){
    if (aggr){
      ggPer <- ggPer + facet_grid( JB~ codec)    
    }else{
      ggPer <- ggPer + facet_grid(. ~ JB)  
    }
  }else{
    #
    if (aggr){
      ggPer <- ggPer + facet_grid( lossRA~ codec)    
    }else{
      ggPer <- ggPer + facet_grid(. ~ lossRA)   
    }
  }
  
  return (ggPer)
}




myG2_geom_errorbarMOSpoint<- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA, optBY="", aggr=FALSE){
  nAlter <- length(levels(indf$method_codec)) - 3 # ignore at head zZRscore
  
  #first order by day
  indf[with(indf, order(time) ),]
  
  indf <- indf[indf$method!="zZRscore",]
  fauxSummarise <- function(indf){
    #print(indf$RHR)
    MOS <- mean(indf$MOS)
    sdMOS <- sd(indf$MOS)
    #print(sdRHR)
    #RHR_THE <- mean(indf$RHR_THE)
    codec <- unique(indf$codec)
    method <- unique(indf$method)
    lossRA <- unique(indf$lossRA)
    return(data.frame(MOS=MOS, sdMOS=sdMOS, codec=codec, method=method, lossRA=lossRA))
  }
  fauxSummarise2 <- function(indf){
    #print(indf$RHR)
    MOS <- mean(indf$MOS)
    sdMOS <- sd(indf$MOS)
    #print(sdRHR)
    #RHR_THE <- mean(indf$RHR_THE)
    codec <- unique(indf$codec)
    method <- unique(indf$method)
    JB <- unique(indf$JB)
    return(data.frame(MOS=MOS, sdMOS=sdMOS, codec=codec, method=method, JB=JB))
  }
  
  if (optBY=="byJB"){
    df2 <- ddply(indf, .(method_codec, JB), .fun=fauxSummarise2) 
    df2$JB <- factor(df2$JB, labels=c("JB = 1  (20ms)","JB = 2  (40ms)","JB = 4  (80ms)"))
  }else{
    df2 <- ddply(indf, .(method_codec, lossRA),.fun=fauxSummarise) 
    df2$lossRA <- factor(df2$lossRA, labels=c("FP = 5%","FP = 10%","FP = 20%"))
  }
  
  nAlter <- nAlter -3
  
  
  
  ggPer <- ggplot(df2, aes(x=method,y=MOS, colour=codec, shape=codec, fill=codec))
  ggPer <- ggPer + geom_path(aes(group=codec, linetype=codec), size=0.6)
  #ggPer <- ggPer + geom_path(position=position_dodge(0.2), size=0.8) 
  ggPer <- ggPer + geom_point(aes(x=method,y=MOS, group=codec), size=2.5)
  #ggPer <- ggPer + geom_point(aes(x=method,y=MOS), position=position_dodge(0.2), shape=22, size=2)
  ggPer <- ggPer + scale_colour_brewer(palette="Dark2") # check values on R coobok page 273 pdf
  ggPer <- ggPer + geom_errorbar(aes(ymax=MOS+sdMOS, ymin=MOS-sdMOS, group=1), width=0.2)
  
  #ggPer <- ggPer + geom_hline(yintercept=1, linetype="dotted", size=0.2) 
  #ggPer <- ggPer + geom_hline(yintercept=2, linetype="dotted", size=0.2) 
  #ggPer <- ggPer + geom_hline(yintercept=3, linetype="dotted", size=0.2)  
  #ggPer <- ggPer + geom_hline(yintercept=4, linetype="dotted", size=0.2)   
  ggPer <- ggPer + inOpt
  ggPer <- ggPer + iTit
  ggPer <- ggPer + iXlab
  ggPer <- ggPer + iYlab
  #ggPer <- ggPer + scale_y_log10()
  ggPer <- ggPer + theme(legend.position="bottom")
  
  #if (length(iYlim) > 1  ){
  #  ggPer <- ggPer + iYlim
  #}
  if (optBY=="byJB"){
    if (aggr){
      ggPer <- ggPer + facet_grid( JB~ codec)    
    }else{
      #ggPer <- ggPer + facet_grid(. ~ JB)  
      #ggPer <- ggPer + facet_grid( JB ~. , scales="free_y", space="free")  
      #ggPer <- ggPer + facet_wrap( ~JB ,nrow=3, scales="free_y")  
      ggPer <- ggPer + facet_wrap( ~JB ,nrow=3)  
    }
  }else{
    #
    if (aggr){
      ggPer <- ggPer + facet_grid( lossRA~ codec)    
    }else{
      #ggPer <- ggPer + facet_grid(. ~ lossRA)   
      #ggPer <- ggPer + facet_grid(lossRA ~ ., scales="free_y",  space="free")  
      ggPer <- ggPer + facet_wrap( ~lossRA ,nrow=3)#, scales="free_y")  
    }
  }
  
  return (ggPer)
}




BKPmyG2_geom_errorbarMOS<- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA, optBY="", aggr=FALSE){
  nAlter <- length(levels(indf$method_codec)) - 3 # ignore at head zZRscore
  
  #first order by day
  indf[with(indf, order(time) ),]
  
  indf <- indf[indf$method!="zZRscore",]
  fauxSummarise <- function(indf){
    #print(indf$RHR)
    MOS <- mean(indf$MOS)
    sdMOS <- sd(indf$MOS)
    #print(sdRHR)
    #RHR_THE <- mean(indf$RHR_THE)
    codec <- unique(indf$codec)
    method <- unique(indf$method)
    lossRA <- unique(indf$lossRA)
    return(data.frame(MOS=MOS, sdMOS=sdMOS, codec=codec, method=method, lossRA=lossRA))
  }
  
  if (optBY=="byJB"){
    df2 <- ddply(indf, .(method_codec, JB), .fun=fauxSummarise) 
  }else{
    df2 <- ddply(indf, .(method_codec, lossRA),.fun=fauxSummarise) 
  }
  
  nAlter <- nAlter -3
  if (aggr){
    #ggPer <- ggplot(df2, aes(x=method,y=MOS, shape=codec))
    ggPer <- ggplot(df2, aes(x=method,y=MOS, fill=codec))
  }else{
    ggPer <- ggplot(df2, aes(x=method_codec,y=MOS, shape=codec))  
  }
  print(summary(df2))
  dfCHECK <<- df2
  #ggPer <- ggPer + geom_bar(position="dodge", stat="identity")
  #ggPer <- ggPer + stat_summary(fun.data="mean_cl_boot", geom="bar") 
  ##ggPer <- ggPer + stat_summary(fun.data="mean_cl_boot", geom="errorbar", width=0.6) 
  #ggPer <- ggPer + stat_summary(fun.y="mean", geom="point", size=3) 
  
  #ggPer <- ggPer + scale_shape_manual(values=c(17,1,15))
  ggPer <- ggPer + geom_bar(aes(x=method,y=MOS), stat="identity")
  ggPer <- ggPer + scale_fill_brewer(palette="Set1") # check values on R coobok page 273 pdf
  ggPer <- ggPer + geom_errorbar(aes(ymax=MOS+sdMOS, ymin=MOS-sdMOS), width=0.4)
  
  ggPer <- ggPer + geom_hline(yintercept=2, linetype="dashed", size=0.3) 
  ggPer <- ggPer + geom_hline(yintercept=3, linetype="dashed", size=0.3)  
  ggPer <- ggPer + geom_hline(yintercept=4, linetype="dashed", size=0.3)   
  #ggPer <- ggPer + inOpt
  ggPer <- ggPer + iTit
  ggPer <- ggPer + iXlab
  ggPer <- ggPer + iYlab
  ggPer <- ggPer + theme(legend.position="bottom")
  
  if (length(iYlim) > 1  ){
    ggPer <- ggPer + iYlim
  }
  if (optBY=="byJB"){
    ggPer <- ggPer + facet_grid(. ~ JB)  
  }else{
    #
    if (aggr){
      ggPer <- ggPer + facet_grid( lossRA~ codec)    
    }else{
      ggPer <- ggPer + facet_grid(. ~ lossRA)   
      
    }
  }
  
  return (ggPer)
}


my_Table_Latex_MOSQualGlobal<- function(indf, inDesc, fileSav,inLabel, optBY=""){
  #first order by day
  indf[with(indf, order(time) ),]
  
  indf <- indf[indf$method!="zZRscore",]
  
  fauxSummarise <- function(indf){
    #print(indf$RHR)
    mosQual <- mean(indf$mosQual)
    sdMOSQual <- sd(indf$mosQual)
    #print(sdRHR)
    #RHR_THE <- mean(indf$RHR_THE)
    codec <- unique(indf$codec)
    
    return(data.frame(mosQual=mosQual, sdMOSQual=sdMOSQual, codec=codec))
  }
  
  if (optBY=="byJB"){
    df2 <- ddply(indf, .(method_codec, JB), .fun=fauxSummarise) 
    #df2 <- df2[df2$JB==1,]
  }else{
    df2 <- ddply(indf, .(method_codec, lossRA),.fun=fauxSummarise) 
    #df2 <- df2[df2$lossRA==0.05, ]
  }
  #require("xtable")
  
  #toLatex <- xtable(df2, caption=inDesc, label="tab01:")
  mySaveTable(df2, fileSav, inDesc, inLabel )
  #return(toLatex)
}


myG2_geom_errorbarMOSQualGlobal<- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA, optBY="", optLine=TRUE){
  nAlter <- length(levels(indf$method_codec)) - 3 # ignore at head zZRscore
  
  #first order by day
  indf[with(indf, order(time) ),]
  
  indf <- indf[indf$method!="zZRscore",]
  
  fauxSummarise <- function(indf){
    #print(indf$RHR)
    mosQual <- mean(indf$mosQual)
    sdMOSQual <- sd(indf$mosQual)
    #print(sdRHR)
    #RHR_THE <- mean(indf$RHR_THE)
    codec <- unique(indf$codec)
    
    return(data.frame(mosQual=mosQual, sdMOSQual=sdMOSQual, codec=codec))
  }
  
  if (optBY=="byJB"){
    df2 <- ddply(indf, .(method_codec, JB), .fun=fauxSummarise) 
  }else{
    df2 <- ddply(indf, .(method_codec, lossRA),.fun=fauxSummarise) 
  }
  
#   
#   
#   nAlter <- nAlter -3
#   
#   ggPer <- ggplot(df2, aes(x=method_codec,y=mosQual, fill=codec))
#   
#   ggPer <- ggPer + geom_bar(position="dodge", stat="identity")
#   ggPer <- ggPer + scale_fill_brewer(palette="Set1") # check values on R coobok page 273 pdf
#   
#   ggPer <- ggPer + geom_errorbar(aes(ymax=mosQual+sdMOSQual, ymin=mosQual-sdMOSQual), width=0.4)
# #   if (optLine){
# #     ggPer <- ggPer + geom_hline(aes(yintercept=RHR_THE, x=method_codec))
# #   }else{
# #     ggPer <- ggPer + stat_smooth(aes(y=RHR_THE, x=method_codec, fill=codec, group=1 ), formula=y~x, method="lm")
# #   }
#   
#   ggPer <- ggPer + inOpt
#   ggPer <- ggPer + iTit
#   ggPer <- ggPer + iXlab
#   ggPer <- ggPer + iYlab
#   ggPer <- ggPer + theme(legend.position="bottom")
#   
#   if (length(iYlim) > 1  ){
#     ggPer <- ggPer + iYlim
#   }
#   
#   if (optBY=="byJB"){
#     ggPer <- ggPer + facet_grid(. ~ JB)  
#   }else{
#     ggPer <- ggPer + facet_grid(. ~ lossRA)  
#   }
#   
#   
#   return (ggPer)
}


myG2_geom_errorbarRHRGlobal_hline<- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA, optBY="", optLine=TRUE){
  nAlter <- length(levels(indf$method_codec)) - 3 # ignore at head zZRscore
  
  #first order by day
  indf[with(indf, order(time) ),]
  
  indf <- indf[indf$method!="zZRscore",]

  fauxSummarise <- function(indf){
	  #print(indf$RHR)
	  RHR <- mean(indf$RHR)
	  sdRHR <- sd(indf$RHR)
	  #print(sdRHR)
	  RHR_THE <- mean(indf$RHR_THE)
	  codec <- unique(indf$codec)

	  return(data.frame(RHR=RHR, sdRHR=sdRHR, RHR_THE=RHR_THE, codec=codec))
  }

  if (optBY=="byJB"){
    df2 <- ddply(indf, .(method_codec, JB), .fun=fauxSummarise) 
  }else{
    df2 <- ddply(indf, .(method_codec, lossRA),.fun=fauxSummarise) 
  }

  dfCHECK <<- df2
  #print(summary(df2))
  
  nAlter <- nAlter -3
  #ggPer <- ggplot(indf, aes(x=method_codec,y=RHR, fill=codec))
  ggPer <- ggplot(df2, aes(x=method_codec,y=RHR, fill=codec))
  #ggPer <- ggPer + stat_summary(fun.y="mean", geom="bar" ) 
  ggPer <- ggPer + geom_bar(position="dodge", stat="identity")
  ggPer <- ggPer + scale_fill_brewer(palette="Set1") # check values on R coobok page 273 pdf
  #ggPer <- ggPer + stat_summary(fun.data="mean_cl_boot", geom="errorbar", width=0.4) 
  ggPer <- ggPer + geom_errorbar(aes(ymax=RHR+sdRHR, ymin=RHR-sdRHR), width=0.4)
  if (optLine){
  ggPer <- ggPer + geom_hline(aes(yintercept=RHR_THE, x=method_codec))
  }else{
  ggPer <- ggPer + stat_smooth(aes(y=RHR_THE, x=method_codec, fill=codec, group=1 ), formula=y~x, method="lm")
  }

  ggPer <- ggPer + inOpt
  ggPer <- ggPer + iTit
  ggPer <- ggPer + iXlab
  ggPer <- ggPer + iYlab
  ggPer <- ggPer + theme(legend.position="bottom")
  
  if (length(iYlim) > 1  ){
    ggPer <- ggPer + iYlim
  }
  
  if (optBY=="byJB"){
    ggPer <- ggPer + facet_grid(. ~ JB)  
  }else{
    ggPer <- ggPer + facet_grid(. ~ lossRA)  
  }
  
  
  return (ggPer)
}






myG2_geom_boxplotRHRGlobal<- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA, optBY=""){
  nAlter <- length(levels(indf$method_codec)) - 3 # ignore at head zZRscore
  
  
  #first order by day
  indf[with(indf, order(time) ),]
  
  indf <- indf[indf$method!="zZRscore",]
  #indf$method_codec <- gsub("METHODICAL", "EVA",indf$method_codec)
  
  if (optBY=="byJB"){
    df2 <- ddply(indf, .(method_codec, JB), summarise, med=mean(RHR) )
  }else{
    df2 <- ddply(indf, .(method_codec, lossRA), summarise, med=mean(RHR) )
  }
  
  nAlter <- nAlter -3
  ggPer <- ggplot(indf, aes(x=method_codec,y=RHR))
  ggPer <- ggPer + geom_boxplot(position="dodge")
  #ggPer <- ggPer + geom_hline(aes(y=mean(RHR_THE), x=method_codec))
  #ggPer <- ggPer + geom_hline(aes(yintercept=RHR_THE, x=method_codec))
  ggPer <- ggPer + stat_smooth(aes(y=RHR_THE, x=method_codec, group=1))
  #ggPer <- ggPer + geom_text(data=df2, aes(x=method_codec, y=med + 10, label=sprintf("%.2f%%",med) ), size=2, angle=90, position="dodge")
  ggPer <- ggPer + inOpt
  ggPer <- ggPer + iTit
  ggPer <- ggPer + iXlab
  ggPer <- ggPer + iYlab
  
  if (length(iYlim) > 1  ){
    ggPer <- ggPer + iYlim
  }
  if (optBY=="byJB"){
    ggPer <- ggPer + facet_grid(. ~ JB)  
  }else{
    ggPer <- ggPer + facet_grid(. ~ lossRA)  
  }
  
  return (ggPer)
}

myG2_geom_boxplotRHRGlobal_hline<- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA){
  nAlter <- length(levels(indf$method_codec)) - 3 # ignore at head zZRscore
  
  
  #first order by day
  indf[with(indf, order(time) ),]
  
  indf <- indf[indf$method!="zZRscore",]
  #indf$method_codec <- gsub("METHODICAL", "EVA",indf$method_codec)
  
    df2 <- ddply(indf, .(method_codec, JB), summarise, med=mean(RHR) )
  
  
  nAlter <- nAlter -3
  ggPer <- ggplot(indf, aes(x=method_codec,y=RHR))
  ggPer <- ggPer + geom_boxplot(position="dodge")
  #ggPer <- ggPer + geom_hline(aes(y=mean(RHR_THE), x=method_codec))
  ggPer <- ggPer + geom_hline(aes(yintercept=RHR_THE, x=method_codec))
  #ggPer <- ggPer + stat_smooth(aes(y=RHR_THE, x=method_codec, group=1))
  #ggPer <- ggPer + geom_text(data=df2, aes(x=method_codec, y=med + 10, label=sprintf("%.2f%%",med) ), size=2, angle=90, position="dodge")
  ggPer <- ggPer + inOpt
  ggPer <- ggPer + iTit
  ggPer <- ggPer + iXlab
  ggPer <- ggPer + iYlab
  
  if (length(iYlim) > 1  ){
    ggPer <- ggPer + iYlim
  }
  
  ggPer <- ggPer + facet_grid(. ~ JB)  
  
  
  return (ggPer)
}

myG2_geom_lines_text<- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA){
  myFormat <- function(x){
    lab <- format(x, scientific=FALSE)  
  }
  
  #indf$time <- indf$time - 20121011
  
  ggIP1 <- ggplot(indf, aes(x=timeData, y=medianOWD))
  ggIP1 <- ggIP1 + geom_line(aes(x=timeData, y=100), linetype="dotted")
  ggIP1 <- ggIP1 + geom_line(aes(x=timeData, y=50) ,linetype="dotted")
  ggIP1 <- ggIP1 + geom_line(aes(x=timeData, y=jitter, colour="jitter"))
  ggIP1 <- ggIP1 + geom_line(aes(x=timeData, y=medianOWD, colour="OWD"))
  ggIP1 <- ggIP1 + geom_line(aes(x=timeData, y=loss_rate, colour="Loss Ratio"))
  ggIP1 <- ggIP1 + geom_text(aes(x=timeData, y=250, label=METHODICAL), hjust=0, vjust=0, size=2)
  ggIP1 <- ggIP1 + geom_text(aes(x=timeData, y=300, label=METHODICAL_20), hjust=0, vjust=0, size=2)
  ggIP1 <- ggIP1 + geom_text(aes(x=timeData, y=180, label=TOPSIS), hjust=0, vjust=0, size=2)
  ggIP1 <- ggIP1 + geom_text(aes(x=timeData, y=100, label=DIA), hjust=0, vjust=0, size=2)
  ggIP1 <- ggIP1 + facet_grid(ip_source ~ .)  
  ggIP1 <- ggIP1 + scale_x_continuous(label=myFormat)
  ggIP1 <- ggIP1 + inOpt
  ggIP1 <- ggIP1 + iTit
  ggIP1 <- ggIP1 + iXlab
  ggIP1 <- ggIP1 + iYlab
  if (length(iYlim) > 1  ){
    ggIP1 <- ggIP1 + iYlim
  }
  return(ggIP1)
}




auxCnvFactor2NumType <- function(iCol, aS, in1, in2){
  if (is.factor(iCol)){
    #print(levels(iCol))
    #print(labels(iCol))
    iCol <- factor(iCol, labels=c("0", "1"))
    iCol <- as.numeric(levels(iCol)[iCol]) 
  }else{
    iCol[which(iCol==aS, arr.ind=TRUE)] <- "1"
    iCol[which(iCol!="1", arr.ind=TRUE)] <- 0
    
    iCol <- as.numeric(iCol)
    #print(iCol)
  }
  iCol <- (iCol * in1) + in2
  return (iCol)  
}


# Try to combine dfCost with dfRES
#ddply(dfCost, .(iter), .fun=getMeth)
getMethMultihoming <- function(idf){
  
  iter <- idf$iter
  idf$METHODICAL <- -1
  idf$METHODICAL2 <- -1
  idf$METHODICAL8 <- -1
  idf$METHODICAL_H6 <- -1
  idf$METHODICAL_H2 <- -1
  
  idf$TOPSIS <- -1
  idf$DiA <- -1  
  idf$time <- -1
  
  auxdf <- dfRES_prefMultihoming[dfRES_prefMultihoming$iter==iter,]
  auxdf$time <- as.numeric(auxdf$time[1]) 
  #print(auxdf$time)
  
  idf$time <- unique(auxdf$time)
  aMet <- auxdf[auxdf$method=="METHODICAL",]
  if (aMet$pref1==idf$pIDs[1]){
    idf[1,"METHODICAL"] <- 1
    idf[2,"METHODICAL"] <- 0
  }else{
    idf[1,"METHODICAL"] <- 0
    idf[2,"METHODICAL"] <- 1
  }
  
  aMet <- auxdf[auxdf$method=="METHODICAL_20,80",]
  if (aMet$pref1==idf$pIDs[1]){
    idf[1,"METHODICAL2"] <- 1
    idf[2,"METHODICAL2"] <- 0
  }else{
    idf[1,"METHODICAL2"] <- 0
    idf[2,"METHODICAL2"] <- 1
  } 
  
  aMet <- auxdf[auxdf$method=="METHODICAL_80,20",]
  if (aMet$pref1==idf$pIDs[1]){
    idf[1,"METHODICAL8"] <- 1
    idf[2,"METHODICAL8"] <- 0
  }else{
    idf[1,"METHODICAL8"] <- 0
    idf[2,"METHODICAL8"] <- 1
  }
  
  aMet <- auxdf[auxdf$method=="METHODICAL_H6",]
  if (aMet$pref1==idf$pIDs[1]){
    idf[1,"METHODICAL_H6"] <- 1
    idf[2,"METHODICAL_H6"] <- 0
  }else{
    idf[1,"METHODICAL_H6"] <- 0
    idf[2,"METHODICAL_H6"] <- 1
  }
  
  aMet <- auxdf[auxdf$method=="METHODICAL_H2",]
  if (aMet$pref1==idf$pIDs[1]){
    idf[1,"METHODICAL_H2"] <- 1
    idf[2,"METHODICAL_H2"] <- 0
  }else{
    idf[1,"METHODICAL_H2"] <- 0
    idf[2,"METHODICAL_H2"] <- 1
  }
  
  aMet <- auxdf[auxdf$method=="TOPSIS",]
  if (aMet$pref1==idf$pIDs[1]){
    idf[1,"TOPSIS"] <- 1
    idf[2,"TOPSIS"] <- 0
  }else{
    idf[1,"TOPSIS"] <- 0
    idf[2,"TOPSIS"] <- 1
  }
  
  aMet <- auxdf[auxdf$method=="DiA",]
  if (aMet$pref1==idf$pIDs[1]){
    idf[1,"DiA"] <- 1
    idf[2,"DiA"] <- 0
  }else{
    idf[1,"DiA"] <- 0
    idf[2,"DiA"] <- 1
  }
  
  #idf$time <- as.numeric(levels(idf$time)[idf$time])
  
  return(idf)
  
}

getMethTP <- function(idf){
  
  iter <- idf$iter
  idf$METHODICAL <- -1
  idf$METHODICAL2 <- -1
  idf$METHODICAL8 <- -1
  idf$METHODICAL_H6 <- -1
  idf$METHODICAL_H2 <- -1
  
  idf$TOPSIS <- -1
  idf$DiA <- -1  
  idf$time <- -1
  
  auxdf <- dfRES_prefTP[dfRES_prefTP$iter==iter,]
  auxdf$time <- as.numeric(auxdf$time[1])
  idf$time <- unique(auxdf$time)
  aMet <- auxdf[auxdf$method=="METHODICAL",]
  if (aMet$pref1==idf$pIDs[1]){
    idf[1,"METHODICAL"] <- 1
    idf[2,"METHODICAL"] <- 0
  }else{
    idf[1,"METHODICAL"] <- 0
    idf[2,"METHODICAL"] <- 1
  }
  
  aMet <- auxdf[auxdf$method=="METHODICAL_20,80",]
  if (aMet$pref1==idf$pIDs[1]){
    idf[1,"METHODICAL2"] <- 1
    idf[2,"METHODICAL2"] <- 0
  }else{
    idf[1,"METHODICAL2"] <- 0
    idf[2,"METHODICAL2"] <- 1
  } 
  
  aMet <- auxdf[auxdf$method=="METHODICAL_80,20",]
  if (aMet$pref1==idf$pIDs[1]){
    idf[1,"METHODICAL8"] <- 1
    idf[2,"METHODICAL8"] <- 0
  }else{
    idf[1,"METHODICAL8"] <- 0
    idf[2,"METHODICAL8"] <- 1
  }
  
  aMet <- auxdf[auxdf$method=="METHODICAL_H6",]
  if (aMet$pref1==idf$pIDs[1]){
    idf[1,"METHODICAL_H6"] <- 1
    idf[2,"METHODICAL_H6"] <- 0
  }else{
    idf[1,"METHODICAL_H6"] <- 0
    idf[2,"METHODICAL_H6"] <- 1
  }
  
  aMet <- auxdf[auxdf$method=="METHODICAL_H2",]
  if (aMet$pref1==idf$pIDs[1]){
    idf[1,"METHODICAL_H2"] <- 1
    idf[2,"METHODICAL_H2"] <- 0
  }else{
    idf[1,"METHODICAL_H2"] <- 0
    idf[2,"METHODICAL_H2"] <- 1
  }

  aMet <- auxdf[auxdf$method=="TOPSIS",]
  if (aMet$pref1==idf$pIDs[1]){
    idf[1,"TOPSIS"] <- 1
    idf[2,"TOPSIS"] <- 0
  }else{
    idf[1,"TOPSIS"] <- 0
    idf[2,"TOPSIS"] <- 1
  }
  
  aMet <- auxdf[auxdf$method=="DiA",]
  if (aMet$pref1==idf$pIDs[1]){
    idf[1,"DiA"] <- 1
    idf[2,"DiA"] <- 0
  }else{
    idf[1,"DiA"] <- 0
    idf[2,"DiA"] <- 1
  }
  
  #idf$time <- as.numeric(levels(idf$time)[idf$time])
  
  return(idf)
  
}

myG2_geom_lines_PathOnlyOneIP<- function(indf,  inOpt, iTit, iXlab, iYlab, iYlim=NA){
  # Prepare data
  #
  indf$METHODICAL <- indf$METHODICAL * 50 + 750
  indf$METHODICAL2 <- indf$METHODICAL2 * 50 + 650
  indf$METHODICAL8 <- indf$METHODICAL8 * 50 + 550
  indf$METHODICAL_H6 <- indf$METHODICAL_H6 * 50 + 450
  indf$METHODICAL_H2 <- indf$METHODICAL_H2 * 50 + 350
  
  indf$TOPSIS <- indf$TOPSIS * 50 + 250
  indf$DiA <- indf$DiA * 50 + 150
  
  #aday <- iDay * 10000
  
  x1 <- indf$time[1] + 100
  indf$x1 <- x1
  
  myFormat <- function(x){
    lab <- format(x, scientific=FALSE)  
  }
  
  #indf$time <- indf$time - aday
  
  ggIP1 <- ggplot(indf, aes(x=time, y=medianOWD))
  ggIP1 <- ggIP1 + geom_line(aes(x=time, y=100), linetype="dotted")
  ggIP1 <- ggIP1 + geom_line(aes(x=time, y=50) ,linetype="dotted")
  ggIP1 <- ggIP1 + geom_line(aes(x=time, y=ipdv, colour="IPDV"))
  ggIP1 <- ggIP1 + geom_line(aes(x=time, y=owd, colour="OWD"))
  ggIP1 <- ggIP1 + geom_line(aes(x=time, y=loss, colour="Loss Ratio"))
  ggIP1 <- ggIP1 + geom_step(aes(x=time, y=METHODICAL))
  ggIP1 <- ggIP1 + geom_text(aes(x=x1, y=max(METHODICAL)+20, label="_METHODICAL_"), hjust=0, vjust=0, size=2)
  ggIP1 <- ggIP1 + geom_step(aes(x=time, y=METHODICAL2))
  ggIP1 <- ggIP1 + geom_text(aes(x=x1, y=max(METHODICAL2)+20, label="_METHODICAL 20%,80%_"), hjust=0, vjust=0, size=2)
  ggIP1 <- ggIP1 + geom_step(aes(x=time, y=METHODICAL8))
  ggIP1 <- ggIP1 + geom_text(aes(x=x1, y=max(METHODICAL8)+20, label="_METHODICAL 80%,20%_"), hjust=0, vjust=0, size=2)
  ggIP1 <- ggIP1 + geom_step(aes(x=time, y=METHODICAL_H6))
  ggIP1 <- ggIP1 + geom_text(aes(x=x1, y=max(METHODICAL_H6)+20, label="_METHODICAL H6_"), hjust=0, vjust=0, size=2)
  ggIP1 <- ggIP1 + geom_step(aes(x=time, y=METHODICAL_H2))
  ggIP1 <- ggIP1 + geom_text(aes(x=x1, y=max(METHODICAL_H2)+20, label="_METHODICAL H2_"), hjust=0, vjust=0, size=2)
  
  ggIP1 <- ggIP1 + geom_step(aes(x=time, y=TOPSIS))
  ggIP1 <- ggIP1 + geom_text(aes(x=x1, y=max(TOPSIS)+20, label="_TOPSIS_"), hjust=0, vjust=0, size=2)
  ggIP1 <- ggIP1 + geom_step(aes(x=time, y=DiA))
  ggIP1 <- ggIP1 + geom_text(aes(x=x1, y=max(DiA)+20, label="_DiA_"), hjust=0, vjust=0, size=2)
  ggIP1 <- ggIP1 + facet_grid(pIDs ~ .)  
  ggIP1 <- ggIP1 + scale_x_continuous(label=myFormat)
  ggIP1 <- ggIP1 + inOpt
  ggIP1 <- ggIP1 + iTit
  ggIP1 <- ggIP1 + iXlab
  ggIP1 <- ggIP1 + iYlab
  if (length(iYlim) > 1  ){
    ggIP1 <- ggIP1 + iYlim
  }
  return(ggIP1)
}





myG2_geom_bar_CON <- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA){
  ggPer <- ggplot(indf, aes(x=Sets,y=Perc, fill=factor(method)))
  ggPer <- ggPer + geom_bar(position="dodge", size=1)
  ggPer <- ggPer + scale_x_discrete(limits=1:6)
  ggPer <- ggPer + facet_grid(Pref ~ method )  
  ggPer <- ggPer + inOpt
  ggPer <- ggPer + iTit
  ggPer <- ggPer + iXlab
  ggPer <- ggPer + iYlab
  if (length(iYlim)  > 1  ){
    ggPer <- ggPer + iYlim
  }
  return (ggPer)
}


myG2_geom_bar_CON_perPath <- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA){
  ggPer <- ggplot(indf, aes(x=Sets,y=Perc, fill=factor(PrefPath)))
  ggPer <- ggPer + geom_bar(position="dodge", size=1)
  ggPer <- ggPer + scale_x_discrete(limits=1:6)
  ggPer <- ggPer + facet_grid(Pref ~ method )  
  ggPer <- ggPer + inOpt
  ggPer <- ggPer + iTit
  ggPer <- ggPer + iXlab
  ggPer <- ggPer + iYlab
  ggPer <- ggPer + theme(legend.position="bottom")
  if (length(iYlim) > 1 ){
    ggPer <- ggPer + iYlim
  }
  return (ggPer)
}

myG2_geom_bar_MeTH_HEU <- function(indf, inOpt, iTit, iXlab, iYlab, iYlim=NA){
  ggPer <- ggplot(indf, aes(x=method1,y=Per))
  ggPer <- ggPer + geom_bar(position="dodge", size=1)
  ggPer <- ggPer + geom_text(aes(label=sprintf("%.2f%%",Per), size=5, y=Per+10, x=method1, angle=90))
  ggPer <- ggPer + inOpt
  ggPer <- ggPer + iTit
  ggPer <- ggPer + iXlab
  ggPer <- ggPer + iYlab
  if (!is.na(iYlim)  ){
    ggPer <- ggPer + iYlim
  }
  return (ggPer)
}


