 newdata <- read.table("new.rdata",header=TRUE,colClasses="character")
 ## Missing value code is NA
 ## Change file name if needed
 ## Predicting least-squares mean
 predicted <- function(){
 if(!is.na(qdep) & qdep <=    3.0000000000000000      ){
 if(!is.na(wr) & wr <=    30.000000000000000      ){
 if(is.na(rdsz) | rdsz <=    232.00000000000000      ){
 if(!is.na(qdep) & qdep <=    1.5000000000000000      ){
 if(!is.na(wr) & wr <=    10.000000000000000      ){
 nodeid <-           32
 predict <-    240.27000000000001     
 } else {
 nodeid <-           33
 predict <-    128.60333333333332     
 }
 } else {
 if(is.na(wrnd) | wrnd <=    90.000000000000000      ){
 if(is.na(wrsz) | wrsz <=    207.00000000000000      ){
 nodeid <-           68
 predict <-    236.80250000000001     
 } else {
 nodeid <-           69
 predict <-    284.05499999999995     
 }
 } else {
 nodeid <-           35
 predict <-    189.35333333333332     
 }
 }
 } else {
 nodeid <-            9
 predict <-    281.69000000000000     
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    113.50000000000000      ){
 if(is.na(wr) | wr <=    70.000000000000000      ){
 if(!is.na(rdsz) & rdsz <=    96.000000000000000      ){
 nodeid <-           40
 predict <-    136.75666666666666     
 } else {
 nodeid <-           41
 predict <-    199.18750000000000     
 }
 } else {
 nodeid <-           21
 predict <-    148.87750000000000     
 }
 } else {
 if(is.na(wrnd) | wrnd <=    90.000000000000000      ){
 if(is.na(rrnd) | rrnd <=    70.000000000000000      ){
 if(!is.na(rdsz) & rdsz <=    178.00000000000000      ){
 nodeid <-           88
 predict <-    143.79333333333332     
 } else {
 nodeid <-           89
 predict <-    165.11000000000001     
 }
 } else {
 nodeid <-           45
 predict <-    147.04999999999998     
 }
 } else {
 nodeid <-           23
 predict <-    164.44999999999999     
 }
 }
 }
 } else {
 if(!is.na(wr) & wr <=    30.000000000000000      ){
 if(!is.na(wr) & wr <=    10.000000000000000      ){
 if(!is.na(rdsz) & rdsz <=    7.0000000000000000      ){
 nodeid <-           24
 predict <-    139.07999999999998     
 } else {
 if(is.na(qdep) | qdep <=    48.000000000000000      ){
 if(is.na(qdep) | qdep <=    24.000000000000000      ){
 if(!is.na(rrnd) & rrnd <=    30.000000000000000      ){
 nodeid <-          200
 predict <-    473.15499999999997     
 } else {
 if(!is.na(wrsz) & wrsz <=    48.000000000000000      ){
 nodeid <-          402
 predict <-    289.16500000000002     
 } else {
 nodeid <-          403
 predict <-    360.96499999999997     
 }
 }
 } else {
 if(!is.na(wrnd) & wrnd <=    10.000000000000000      ){
 nodeid <-          202
 predict <-    484.13499999999999     
 } else {
 nodeid <-          203
 predict <-    507.47666666666669     
 }
 }
 } else {
 nodeid <-           51
 predict <-    473.11999999999995     
 }
 }
 } else {
 if(is.na(qdep) | qdep <=    48.000000000000000      ){
 if(!is.na(rdsz) & rdsz <=    42.000000000000000      ){
 nodeid <-           52
 predict <-    192.39666666666668     
 } else {
 if(!is.na(rdsz) & rdsz <=    124.50000000000000      ){
 if(!is.na(wrnd) & wrnd <=    20.000000000000000      ){
 nodeid <-          212
 predict <-    375.02999999999997     
 } else {
 nodeid <-          213
 predict <-    251.34750000000003     
 }
 } else {
 if(!is.na(qdep) & qdep <=    10.000000000000000      ){
 nodeid <-          214
 predict <-    337.44000000000000     
 } else {
 nodeid <-          215
 predict <-    381.05500000000006     
 }
 }
 }
 } else {
 nodeid <-           27
 predict <-    314.31500000000000     
 }
 }
 } else {
 if(!is.na(wr) & wr <=    70.000000000000000      ){
 if(!is.na(wrsz) & wrsz <=    73.500000000000000      ){
 if(!is.na(rrnd) & rrnd <=    50.000000000000000      ){
 if(!is.na(rdsz) & rdsz <=    136.50000000000000      ){
 nodeid <-          112
 predict <-    234.93500000000000     
 } else {
 nodeid <-          113
 predict <-    282.28666666666669     
 }
 } else {
 if(!is.na(rrnd) & rrnd <=    70.000000000000000      ){
 nodeid <-          114
 predict <-    362.26333333333332     
 } else {
 nodeid <-          115
 predict <-    294.21000000000004     
 }
 }
 } else {
 if(!is.na(wrnd) & wrnd <=    10.000000000000000      ){
 nodeid <-           58
 predict <-    199.47500000000002     
 } else {
 if(!is.na(wrnd) & wrnd <=    30.000000000000000      ){
 nodeid <-          118
 predict <-    223.44999999999999     
 } else {
 if(is.na(wrsz) | wrsz <=    216.50000000000000      ){
 if(!is.na(wrnd) & wrnd <=    70.000000000000000      ){
 if(!is.na(wr) & wr <=    50.000000000000000      ){
 nodeid <-          952
 predict <-    304.48000000000002     
 } else {
 nodeid <-          953
 predict <-    213.56000000000003     
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    144.50000000000000      ){
 nodeid <-          954
 predict <-    293.05000000000001     
 } else {
 nodeid <-          955
 predict <-    204.63999999999999     
 }
 }
 } else {
 nodeid <-          239
 predict <-    203.57499999999999     
 }
 }
 }
 }
 } else {
 if(!is.na(wrnd) & wrnd <=    50.000000000000000      ){
 if(!is.na(qdep) & qdep <=    6.0000000000000000      ){
 nodeid <-           60
 predict <-    157.28500000000000     
 } else {
 if(!is.na(wrnd) & wrnd <=    10.000000000000000      ){
 nodeid <-          122
 predict <-    174.59500000000000     
 } else {
 if(!is.na(wr) & wr <=    90.000000000000000      ){
 nodeid <-          246
 predict <-    201.60666666666665     
 } else {
 nodeid <-          247
 predict <-    162.75333333333333     
 }
 }
 }
 } else {
 if(is.na(rrnd) | rrnd <=    90.000000000000000      ){
 if(!is.na(rdsz) & rdsz <=    27.500000000000000      ){
 nodeid <-          124
 predict <-    157.67250000000001     
 } else {
 if(is.na(qdep) | qdep <=    40.000000000000000      ){
 if(!is.na(wr) & wr <=    90.000000000000000      ){
 nodeid <-          500
 predict <-    183.73666666666668     
 } else {
 nodeid <-          501
 predict <-    134.75333333333333     
 }
 } else {
 if(!is.na(wr) & wr <=    90.000000000000000      ){
 nodeid <-          502
 predict <-    178.43500000000000     
 } else {
 nodeid <-          503
 predict <-    159.97666666666666     
 }
 }
 }
 } else {
 nodeid <-           63
 predict <-    160.19333333333330     
 }
 }
 }
 }
 }
 return(c(nodeid,predict))
 }
 ###z <- read.table("disk_fit.txt",header=TRUE)
 ###noerr <- TRUE
 for(i in 1:dim(newdata)[1]){
 qdep <- as.numeric(newdata$qdep[i])
 wr <- as.numeric(newdata$wr[i])
 wrnd <- as.numeric(newdata$wrnd[i])
 rrnd <- as.numeric(newdata$rrnd[i])
 wrsz <- as.numeric(newdata$wrsz[i])
 rdsz <- as.numeric(newdata$rdsz[i])
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
