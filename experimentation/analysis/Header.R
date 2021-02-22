
CONF_INTERVAL <- 0.95




f_aux_read_csv <- function(a_fil, header, cols){
  aux_ <- read.csv2(file=a_fil, header=header, sep=",", col.names = cols, dec=".", stringsAsFactors = FALSE)
  return(aux_)
}


summaryfun <- function(xraw){
  x <- as.numeric(xraw)
  return(list(N=length(x),
              Mean=mean(x,na.rm = TRUE),
              Median=median(x,na.rm = TRUE),
              SD=sd(x,na.rm = TRUE),
              Min=min(x,na.rm = TRUE),
              Max=max(x,na.rm = TRUE),
              q0=quantile(x,na.rm = TRUE)[1],
              q2=quantile(x,na.rm = TRUE)[2],
              q3=quantile(x,na.rm = TRUE)[3],
              q4=quantile(x,na.rm = TRUE)[4],
              q5=quantile(x,na.rm = TRUE)[5] ))
}

get_id_from_str <- function(is){
  matches <- regmatches(is, gregexpr("[[:digit:]]+", is))
  return (unlist(matches))
}


