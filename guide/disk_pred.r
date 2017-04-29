 newdata <- read.table("new.rdata",header=TRUE,colClasses="character")
 ## Missing value code is NA
 ## Change file name if needed
 ## Predicting least-squares mean
 predicted <- function(){
 if(!is.na(wr) & wr <=    20.000000000000000      ){
 nodeid <-            2
 predict <-    15.773333333333333     
 } else {
 if(is.na(wr) | wr <=    90.000000000000000      ){
 if(!is.na(rrnd) & rrnd <=    30.000000000000000      ){
 nodeid <-           12
 predict <-   0.42249999999999999     
 } else {
 nodeid <-           13
 predict <-    1.0800000000000001     
 }
 } else {
 nodeid <-            7
 predict <-    1.1366666666666667     
 }
 }
 return(c(nodeid,predict))
 }
 ###z <- read.table("disk_fit.txt",header=TRUE)
 ###noerr <- TRUE
 for(i in 1:dim(newdata)[1]){
 wr <- as.numeric(newdata$wr[i])
 rrnd <- as.numeric(newdata$rrnd[i])
 tmp <- predicted()
 node <- tmp[1]
 rpred <- tmp[2]
 ###gnode <- z$node[i]
 ###gpred <- z$predicted[i]
 ###if(sum(abs(rpred-gpred))>0.001*sum(abs(rpred)+abs(gpred))
 ### | is.na(rpred+gpred)){
 ###print(c("Case ",i,node,gnode,rpred,gpred))
 ###noerr <- FALSE}
 }
 ###if(noerr == TRUE) print("No errors")
