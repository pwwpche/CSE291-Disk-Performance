 newdata <- read.table("new.rdata",header=TRUE,colClasses="character")
 ## Missing value code is NA
 ## Change file name if needed
 ## Predicting least-squares mean
 predicted <- function(){
 if(is.na(wrsz) | wrsz <=    1.2600000000000000E+02 ){
 if(is.na(wr) | wr <=   62.5000000000000000 ){
 if(is.na(rdsz) | rdsz <=    1.5000000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=   62.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   30.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   78.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   10.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    2.5000000000000000 ){
 if(!is.na(wr) & wr <=   12.5000000000000000 ){
 nodeid <-  512
 predict <-    0.2333333333333333
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   32.0000000000000000 ){
 nodeid <-  2052
 predict <-    0.1750000000000000
 } else {
 if(!is.na(wrsz) & wrsz <=   22.0000000000000000 ){
 nodeid <-  4106
 predict <-    0.3133333333333334
 } else {
 if(!is.na(wrsz) & wrsz <=   36.0000000000000000 ){
 nodeid <-  8214
 predict <-    0.3799999999999999
 } else {
 nodeid <-  8215
 predict <-    0.4000000000000000
 }
 }
 }
 } else {
 nodeid <-  1027
 predict <-    0.2600000000000000
 }
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  514
 predict <-    1.0249999999999999
 } else {
 if(!is.na(wrsz) & wrsz <=   30.0000000000000000 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  2060
 predict <-    0.4550000000000000
 } else {
 nodeid <-  2061
 predict <-    0.8025000000000000
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   66.0000000000000000 ){
 nodeid <-  2062
 predict <-    0.8200000000000000
 } else {
 nodeid <-  2063
 predict <-    1.4399999999999999
 }
 }
 }
 }
 } else {
 if(!is.na(wr) & wr <=   12.5000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    2.5000000000000000 ){
 nodeid <-  516
 predict <-    0.2150000000000000
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  1034
 predict <-    1.6324999999999998
 } else {
 if(!is.na(qdep) & qdep <=   18.0000000000000000 ){
 nodeid <-  2070
 predict <-    0.3833333333333334
 } else {
 nodeid <-  2071
 predict <-    2.1425000000000001
 }
 }
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   70.0000000000000000 ){
 if(!is.na(rrnd) & rrnd <=   75.0000000000000000 ){
 if(!is.na(wr) & wr <=   37.5000000000000000 ){
 nodeid <-  4144
 predict <-    1.1599999999999999
 } else {
 nodeid <-  4145
 predict <-    5.4233333333333329
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   40.0000000000000000 ){
 nodeid <-  4146
 predict <-    2.6999999999999997
 } else {
 nodeid <-  4147
 predict <-    2.1550000000000002
 }
 }
 } else {
 nodeid <-  1037
 predict <-    4.4800000000000004
 }
 } else {
 if(!is.na(qdep) & qdep <=   14.0000000000000000 ){
 nodeid <-  1038
 predict <-    1.9950000000000001
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  2078
 predict <-    3.3874999999999997
 } else {
 if(!is.na(wrsz) & wrsz <=   22.0000000000000000 ){
 nodeid <-  4158
 predict <-    1.4849999999999999
 } else {
 nodeid <-  4159
 predict <-    3.0825000000000000
 }
 }
 }
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.1400000000000000E+02 ){
 if(!is.na(qdep) & qdep <=   22.0000000000000000 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   54.0000000000000000 ){
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   18.0000000000000000 ){
 nodeid <-  4160
 predict <-    2.5366666666666666
 } else {
 nodeid <-  4161
 predict <-    3.9449999999999998
 }
 } else {
 nodeid <-  2081
 predict <-    1.7566666666666666
 }
 } else {
 nodeid <-  1041
 predict <-    0.9299999999999999
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   90.0000000000000000 ){
 nodeid <-  1042
 predict <-    2.2050000000000001
 } else {
 if(!is.na(wrsz) & wrsz <=   46.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   34.0000000000000000 ){
 nodeid <-  4172
 predict <-    3.2166666666666668
 } else {
 nodeid <-  4173
 predict <-    3.9050000000000002
 }
 } else {
 if(!is.na(qdep) & qdep <=   12.0000000000000000 ){
 nodeid <-  4174
 predict <-    1.0774999999999999
 } else {
 nodeid <-  4175
 predict <-    4.8599999999999994
 }
 }
 }
 }
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(wr) & wr <=   12.5000000000000000 ){
 nodeid <-  1044
 predict <-    4.8933333333333335
 } else {
 if(!is.na(qdep) & qdep <=   26.0000000000000000 ){
 nodeid <-  2090
 predict <-    6.1366666666666667
 } else {
 nodeid <-  2091
 predict <-    7.3533333333333326
 }
 }
 } else {
 nodeid <-  523
 predict <-    8.0449999999999999
 }
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   14.0000000000000000 ){
 nodeid <-  524
 predict <-    2.5100000000000002
 } else {
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 if(!is.na(qdep) & qdep <=   20.0000000000000000 ){
 if(is.na(rrnd) | rrnd <=   25.0000000000000000 ){
 nodeid <-  4200
 predict <-    4.2999999999999998
 } else {
 nodeid <-  4201
 predict <-    1.8599999999999999
 }
 } else {
 nodeid <-  2101
 predict <-    8.1699999999999999
 }
 } else {
 nodeid <-  1051
 predict <-    5.6500000000000004
 }
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   75.0000000000000000 ){
 if(!is.na(wr) & wr <=   12.5000000000000000 ){
 nodeid <-  1052
 predict <-    2.9450000000000003
 } else {
 if(!is.na(qdep) & qdep <=   18.0000000000000000 ){
 nodeid <-  2106
 predict <-    2.6799999999999997
 } else {
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 nodeid <-  4214
 predict <-    7.4500000000000002
 } else {
 nodeid <-  4215
 predict <-    5.2949999999999999
 }
 }
 }
 } else {
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   58.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   16.0000000000000000 ){
 nodeid <-  4216
 predict <-    1.6075000000000002
 } else {
 nodeid <-  4217
 predict <-    6.5366666666666662
 }
 } else {
 nodeid <-  2109
 predict <-    8.4550000000000001
 }
 } else {
 nodeid <-  1055
 predict <-    2.2349999999999999
 }
 }
 }
 }
 }
 } else {
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   70.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   46.0000000000000000 ){
 if(is.na(wr) | wr <=   12.5000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    6.0000000000000000 ){
 nodeid <-  1056
 predict <-    3.4133333333333336
 } else {
 if(is.na(qdep) | qdep <=   58.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   28.0000000000000000 ){
 nodeid <-  4228
 predict <-    3.2000000000000002
 } else {
 if(!is.na(rdsz) & rdsz <=   30.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   12.0000000000000000 ){
 nodeid <-  16916
 predict <-    0.7633333333333333
 } else {
 nodeid <-  16917
 predict <-    1.6899999999999999
 }
 } else {
 nodeid <-  8459
 predict <-    4.0199999999999996
 }
 }
 } else {
 nodeid <-  2115
 predict <-    2.7949999999999999
 }
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  1058
 predict <-    5.0475000000000003
 } else {
 if(!is.na(rdsz) & rdsz <=   18.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   32.0000000000000000 ){
 nodeid <-  4236
 predict <-    2.3999999999999999
 } else {
 nodeid <-  4237
 predict <-    4.3499999999999996
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   26.0000000000000000 ){
 nodeid <-  4238
 predict <-    3.7833333333333332
 } else {
 nodeid <-  4239
 predict <-    5.5825000000000005
 }
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   58.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   18.0000000000000000 ){
 nodeid <-  1060
 predict <-    4.7400000000000002
 } else {
 nodeid <-  1061
 predict <-    5.6299999999999999
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   12.0000000000000000 ){
 nodeid <-  1062
 predict <-    8.4250000000000007
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  4252
 predict <-    6.2550000000000008
 } else {
 nodeid <-  4253
 predict <-    4.7433333333333332
 }
 } else {
 nodeid <-  2127
 predict <-    7.9049999999999994
 }
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.0200000000000000E+02 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  532
 predict <-    7.0950000000000006
 } else {
 if(is.na(qdep) | qdep <=   46.0000000000000000 ){
 if(is.na(wr) | wr <=   12.5000000000000000 ){
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 nodeid <-  4264
 predict <-    5.4199999999999999
 } else {
 nodeid <-  4265
 predict <-    6.9600000000000000
 }
 } else {
 nodeid <-  2133
 predict <-    7.4775000000000009
 }
 } else {
 nodeid <-  1067
 predict <-    9.6933333333333334
 }
 }
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(wr) & wr <=   12.5000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   14.0000000000000000 ){
 nodeid <-  2136
 predict <-   10.2266666666666666
 } else {
 if(!is.na(rdsz) & rdsz <=    1.1400000000000000E+02 ){
 nodeid <-  4274
 predict <-    8.6000000000000014
 } else {
 if(!is.na(qdep) & qdep <=   46.0000000000000000 ){
 nodeid <-  8550
 predict <-   10.4899999999999984
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 nodeid <-  17102
 predict <-   13.2799999999999994
 } else {
 nodeid <-  17103
 predict <-   12.7549999999999990
 }
 }
 }
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=    1.3000000000000000E+02 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  8552
 predict <-   13.6699999999999999
 } else {
 nodeid <-  8553
 predict <-   16.4699999999999989
 }
 } else {
 nodeid <-  4277
 predict <-   18.6649999999999991
 }
 } else {
 if(is.na(qdep) | qdep <=   44.0000000000000000 ){
 nodeid <-  4278
 predict <-    9.6099999999999994
 } else {
 nodeid <-  4279
 predict <-   13.7249999999999996
 }
 }
 }
 } else {
 if(is.na(qdep) | qdep <=   52.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   42.0000000000000000 ){
 nodeid <-  2140
 predict <-    9.5633333333333326
 } else {
 nodeid <-  2141
 predict <-   12.3274999999999988
 }
 } else {
 nodeid <-  1071
 predict <-   15.8049999999999997
 }
 }
 }
 }
 } else {
 if(is.na(rdsz) | rdsz <=   98.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   18.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=   78.0000000000000000 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  1072
 predict <-    3.8999999999999999
 } else {
 if(!is.na(rrnd) & rrnd <=   75.0000000000000000 ){
 nodeid <-  2146
 predict <-    3.3399999999999999
 } else {
 if(!is.na(wrnd) & wrnd <=   75.0000000000000000 ){
 nodeid <-  4294
 predict <-    2.6766666666666672
 } else {
 nodeid <-  4295
 predict <-    3.9299999999999997
 }
 }
 }
 } else {
 nodeid <-  537
 predict <-    9.2149999999999999
 }
 } else {
 if(is.na(qdep) | qdep <=   44.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   14.0000000000000000 ){
 nodeid <-  2152
 predict <-    4.0499999999999998
 } else {
 nodeid <-  2153
 predict <-    6.0000000000000000
 }
 } else {
 if(is.na(qdep) | qdep <=   38.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   30.0000000000000000 ){
 nodeid <-  4308
 predict <-    6.2366666666666672
 } else {
 nodeid <-  4309
 predict <-    4.0850000000000000
 }
 } else {
 nodeid <-  2155
 predict <-    7.5750000000000002
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   54.0000000000000000 ){
 nodeid <-  1078
 predict <-    4.0000207661685376
 x <- ifelse(is.na(wrnd),  33.3333333333333357 ,wrnd)
 predict <- predict +   -2.4079921346112330E-03  * x
 x <- ifelse(is.na(rrnd),  33.3333333333333357 ,rrnd)
 predict <- predict +   -2.0889285913949819E-03  * x
 x <- ifelse(is.na(rdsz),  45.0000000000000000 ,rdsz)
 predict <- predict +    5.9180332672529284E-02  * x
 x <- ifelse(is.na(qdep),  51.1111111111111143 ,qdep)
 predict <- predict +   -5.5823058312475554E-02  * x
 x <- ifelse(is.na(wrsz),  40.0000000000000000 ,wrsz)
 predict <- predict +    0.1466651705934706  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  1079
 predict <-   12.1200000000000010
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.0200000000000000E+02 ){
 nodeid <-  270
 predict <-    8.3900000000000006
 } else {
 if(is.na(qdep) | qdep <=   52.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   22.0000000000000000 ){
 nodeid <-  1084
 predict <-    8.6933333333333334
 } else {
 if(!is.na(wrnd) & wrnd <=   75.0000000000000000 ){
 nodeid <-  2170
 predict <-   13.2899999999999991
 } else {
 if(!is.na(rdsz) & rdsz <=    1.1600000000000000E+02 ){
 nodeid <-  4342
 predict <-   11.0700000000000003
 } else {
 nodeid <-  4343
 predict <-   10.0033333333333339
 }
 }
 }
 } else {
 nodeid <-  543
 predict <-   14.6824999999999992
 }
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   26.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   10.0000000000000000 ){
 if(!is.na(wr) & wr <=   12.5000000000000000 ){
 if(is.na(wrsz) | wrsz <=   90.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    6.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    2.5000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   72.0000000000000000 ){
 nodeid <-  2176
 predict <-    0.2733333333333334
 } else {
 nodeid <-  2177
 predict <-    0.5575000000000000
 }
 } else {
 nodeid <-  1089
 predict <-    1.1350000000000000
 }
 } else {
 nodeid <-  545
 predict <-    0.4550000000000000
 }
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  1092
 predict <-    0.7766666666666665
 } else {
 if(is.na(rdsz) | rdsz <=   58.0000000000000000 ){
 nodeid <-  2186
 predict <-    0.3325000000000000
 } else {
 nodeid <-  2187
 predict <-    1.3550000000000000
 }
 }
 } else {
 nodeid <-  547
 predict <-    0.9199999999999999
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=    6.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    2.5000000000000000 ){
 if(is.na(wrnd) | wrnd <=   50.0000000000000000 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 nodeid <-  2192
 predict <-    0.5925000000000000
 } else {
 nodeid <-  2193
 predict <-    0.7500000000000000
 }
 } else {
 nodeid <-  1097
 predict <-    0.5166666666666666
 }
 } else {
 if(is.na(wrsz) | wrsz <=    1.0800000000000000E+02 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  2196
 predict <-    1.3033333333333335
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   76.0000000000000000 ){
 nodeid <-  8788
 predict <-    1.2675000000000001
 } else {
 nodeid <-  8789
 predict <-    1.4375000000000002
 }
 } else {
 nodeid <-  4395
 predict <-    1.5049999999999999
 }
 }
 } else {
 nodeid <-  1099
 predict <-    2.0000000000000000
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   26.0000000000000000 ){
 nodeid <-  550
 predict <-    1.5649999999999999
 } else {
 if(is.na(rdsz) | rdsz <=    1.3200000000000000E+02 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(is.na(wrnd) | wrnd <=   25.0000000000000000 ){
 nodeid <-  4408
 predict <-    3.1999999999999997
 } else {
 nodeid <-  4409
 predict <-    3.1299999999999999
 }
 } else {
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 nodeid <-  4410
 predict <-    2.1274999999999999
 } else {
 nodeid <-  4411
 predict <-    3.1966666666666668
 }
 }
 } else {
 nodeid <-  1103
 predict <-    3.5449999999999999
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   18.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=   98.0000000000000000 ){
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  1104
 predict <-    3.2324999999999999
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(is.na(rrnd) | rrnd <=   25.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=   32.0000000000000000 ){
 if(is.na(wr) | wr <=   12.5000000000000000 ){
 nodeid <-  17680
 predict <-    0.7033333333333333
 } else {
 nodeid <-  17681
 predict <-    2.1899999999999999
 }
 } else {
 nodeid <-  8841
 predict <-    3.4500000000000002
 }
 } else {
 nodeid <-  4421
 predict <-    3.4175000000000000
 }
 } else {
 nodeid <-  2211
 predict <-    2.1699999999999999
 }
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  2212
 predict <-    4.5200000000000005
 } else {
 nodeid <-  2213
 predict <-    5.0700000000000003
 }
 } else {
 nodeid <-  1107
 predict <-    4.0600000000000005
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   66.0000000000000000 ){
 nodeid <-  554
 predict <-    3.1250000000000000
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  4440
 predict <-    4.3266666666666671
 } else {
 nodeid <-  4441
 predict <-    5.5366666666666662
 }
 } else {
 nodeid <-  2221
 predict <-    7.0833333333333330
 }
 } else {
 nodeid <-  1111
 predict <-    3.4575000000000000
 }
 }
 }
 } else {
 if(is.na(rdsz) | rdsz <=    1.1800000000000000E+02 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 if(is.na(rdsz) | rdsz <=   66.0000000000000000 ){
 nodeid <-  2224
 predict <-    4.5674999999999999
 } else {
 nodeid <-  2225
 predict <-    6.0066666666666668
 }
 } else {
 if(!is.na(qdep) & qdep <=   22.0000000000000000 ){
 nodeid <-  2226
 predict <-    6.3433333333333337
 } else {
 nodeid <-  2227
 predict <-    8.8633333333333351
 }
 }
 } else {
 if(is.na(rdsz) | rdsz <=    1.1000000000000000E+02 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(is.na(wrsz) | wrsz <=   96.0000000000000000 ){
 nodeid <-  4456
 predict <-    3.8500000000000001
 } else {
 nodeid <-  4457
 predict <-    7.4950000000000001
 }
 } else {
 if(!is.na(wr) & wr <=   25.0000000000000000 ){
 nodeid <-  4458
 predict <-    1.2650000000000001
 } else {
 if(!is.na(rdsz) & rdsz <=   38.0000000000000000 ){
 nodeid <-  8918
 predict <-    6.8049999999999997
 } else {
 nodeid <-  8919
 predict <-    9.4074999999999989
 }
 }
 }
 } else {
 nodeid <-  1115
 predict <-    6.6099999999999994
 }
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  558
 predict <-    9.5650000000000013
 } else {
 if(!is.na(wr) & wr <=   12.5000000000000000 ){
 nodeid <-  1118
 predict <-    8.4533333333333331
 } else {
 if(!is.na(wrnd) & wrnd <=   75.0000000000000000 ){
 nodeid <-  2238
 predict <-    9.5499999999999989
 } else {
 if(!is.na(qdep) & qdep <=   22.0000000000000000 ){
 nodeid <-  4478
 predict <-    7.8099999999999996
 } else {
 nodeid <-  4479
 predict <-    9.9000000000000004
 }
 }
 }
 }
 }
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   94.0000000000000000 ){
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   34.0000000000000000 ){
 if(is.na(wr) | wr <=   12.5000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   14.0000000000000000 ){
 nodeid <-  1120
 predict <-    0.8966666666666665
 } else {
 if(!is.na(rdsz) & rdsz <=   18.0000000000000000 ){
 nodeid <-  2242
 predict <-    2.6699999999999999
 } else {
 nodeid <-  2243
 predict <-    1.8633333333333333
 }
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 nodeid <-  1122
 predict <-    7.5024999999999995
 } else {
 nodeid <-  1123
 predict <-   10.5099999999999998
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   42.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   30.0000000000000000 ){
 nodeid <-  1124
 predict <-    5.6633333333333331
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  2250
 predict <-    6.2466666666666670
 } else {
 if(!is.na(rdsz) & rdsz <=   76.0000000000000000 ){
 nodeid <-  4502
 predict <-    5.4375000000000000
 } else {
 if(is.na(rrnd) | rrnd <=   50.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=    1.4200000000000000E+02 ){
 nodeid <-  18012
 predict <-   10.4200000000000017
 } else {
 nodeid <-  18013
 predict <-   10.6150000000000002
 }
 } else {
 nodeid <-  9007
 predict <-    8.9533333333333331
 }
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.0000000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=   72.0000000000000000 ){
 nodeid <-  2252
 predict <-    8.6433333333333326
 } else {
 if(is.na(wr) | wr <=   12.5000000000000000 ){
 if(is.na(wrnd) | wrnd <=   25.0000000000000000 ){
 nodeid <-  9012
 predict <-    8.5633333333333326
 } else {
 nodeid <-  9013
 predict <-    9.5000000000000000
 }
 } else {
 nodeid <-  4507
 predict <-   14.0933333333333337
 }
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  2254
 predict <-   16.0549999999999997
 } else {
 if(is.na(wr) | wr <=   12.5000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.2400000000000000E+02 ){
 nodeid <-  9020
 predict <-    9.7699999999999996
 } else {
 nodeid <-  9021
 predict <-   14.8050000000000015
 }
 } else {
 nodeid <-  4511
 predict <-   16.8924999999999983
 }
 }
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   58.0000000000000000 ){
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   50.0000000000000000 ){
 if(is.na(qdep) | qdep <=   36.0000000000000000 ){
 nodeid <-  2256
 predict <-    8.2599999999999998
 } else {
 nodeid <-  2257
 predict <-   12.3499999999999996
 }
 } else {
 nodeid <-  1129
 predict <-   14.8899999999999988
 }
 } else {
 nodeid <-  565
 predict <-    9.5350000000000001
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   68.0000000000000000 ){
 nodeid <-  566
 predict <-   14.3966666666666665
 } else {
 if(!is.na(wrsz) & wrsz <=   74.0000000000000000 ){
 nodeid <-  1134
 predict <-   16.1550000000000011
 } else {
 if(!is.na(wrsz) & wrsz <=   78.0000000000000000 ){
 nodeid <-  2270
 predict <-   14.0549999999999997
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  4542
 predict <-   17.5933333333333337
 } else {
 if(!is.na(wrnd) & wrnd <=   75.0000000000000000 ){
 nodeid <-  9086
 predict <-   18.2766666666666673
 } else {
 nodeid <-  9087
 predict <-   18.1250000000000000
 }
 }
 }
 }
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   62.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   24.0000000000000000 ){
 if(!is.na(wr) & wr <=   12.5000000000000000 ){
 nodeid <-  568
 predict <-    1.4633333333333336
 } else {
 if(!is.na(rdsz) & rdsz <=    6.0000000000000000 ){
 nodeid <-  1138
 predict <-    8.9224999999999994
 } else {
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 nodeid <-  2278
 predict <-   10.0733333333333324
 } else {
 nodeid <-  2279
 predict <-   15.1550000000000011
 }
 }
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(wr) & wr <=   12.5000000000000000 ){
 if(is.na(qdep) | qdep <=   48.0000000000000000 ){
 nodeid <-  2280
 predict <-    3.8766666666666665
 } else {
 nodeid <-  2281
 predict <-    5.5549999999999997
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.1000000000000000E+02 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  4564
 predict <-   12.8700000000000010
 } else {
 nodeid <-  4565
 predict <-    9.4566666666666670
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  4566
 predict <-   19.4400000000000013
 } else {
 if(!is.na(wr) & wr <=   37.5000000000000000 ){
 nodeid <-  9134
 predict <-   13.0099999999999998
 } else {
 nodeid <-  9135
 predict <-   17.9699999999999989
 }
 }
 }
 }
 } else {
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 if(is.na(wr) | wr <=   12.5000000000000000 ){
 nodeid <-  2284
 predict <-    5.8700000000000001
 } else {
 nodeid <-  2285
 predict <-   10.5250000000000004
 }
 } else {
 nodeid <-  1143
 predict <-   18.5533333333333310
 }
 }
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(!is.na(wr) & wr <=   12.5000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  1144
 predict <-    9.4533333333333331
 } else {
 nodeid <-  1145
 predict <-   10.2733333333333334
 }
 } else {
 if(is.na(rdsz) | rdsz <=    1.4600000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=   86.0000000000000000 ){
 nodeid <-  2292
 predict <-   13.4649999999999999
 } else {
 if(is.na(qdep) | qdep <=   50.0000000000000000 ){
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 nodeid <-  9172
 predict <-   14.5024999999999995
 } else {
 nodeid <-  9173
 predict <-   17.4366666666666674
 }
 } else {
 nodeid <-  4587
 predict <-   23.8025000000000020
 }
 }
 } else {
 nodeid <-  1147
 predict <-   23.8149999999999977
 }
 }
 } else {
 if(is.na(rdsz) | rdsz <=    1.2200000000000000E+02 ){
 if(!is.na(wr) & wr <=   12.5000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   98.0000000000000000 ){
 nodeid <-  2296
 predict <-    6.9925000000000006
 } else {
 if(is.na(qdep) | qdep <=   56.0000000000000000 ){
 nodeid <-  4594
 predict <-    9.4833333333333325
 } else {
 nodeid <-  4595
 predict <-   13.6750000000000007
 }
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.0400000000000000E+02 ){
 nodeid <-  4596
 predict <-   11.8849999999999998
 } else {
 if(!is.na(wr) & wr <=   37.5000000000000000 ){
 if(is.na(wrsz) | wrsz <=    1.1800000000000000E+02 ){
 nodeid <-  18388
 predict <-   16.8766666666666652
 } else {
 nodeid <-  18389
 predict <-   10.2250000000000014
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   75.0000000000000000 ){
 nodeid <-  18390
 predict <-   21.6733333333333356
 } else {
 nodeid <-  18391
 predict <-   12.2666666666666657
 }
 }
 }
 } else {
 if(is.na(rdsz) | rdsz <=    1.1400000000000000E+02 ){
 nodeid <-  4598
 predict <-   12.4875000000000007
 } else {
 nodeid <-  4599
 predict <-   16.0199999999999996
 }
 }
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(is.na(wrsz) | wrsz <=    1.1800000000000000E+02 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  4600
 predict <-   18.0399999999999991
 } else {
 nodeid <-  4601
 predict <-   15.9233333333333320
 }
 } else {
 nodeid <-  2301
 predict <-   25.2100000000000009
 }
 } else {
 nodeid <-  1151
 predict <-   16.0249999999999986
 }
 }
 }
 }
 }
 }
 }
 } else {
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.9400000000000000E+02 ){
 if(is.na(wr) | wr <=   12.5000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.6600000000000000E+02 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.5400000000000000E+02 ){
 nodeid <-  576
 predict <-   14.8099999999999969
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.5800000000000000E+02 ){
 nodeid <-  2308
 predict <-   13.6099999999999994
 } else {
 nodeid <-  2309
 predict <-   11.1250000000000000
 }
 } else {
 nodeid <-  1155
 predict <-    8.6724999999999994
 }
 }
 } else {
 nodeid <-  289
 predict <-   -1.1023851612539655
 x <- ifelse(is.na(wrnd),  44.4444444444444429 ,wrnd)
 predict <- predict +    4.9808964204327132E-03  * x
 x <- ifelse(is.na(rdsz),   1.5422222222222223E+02 ,rdsz)
 predict <- predict +    6.4373824238271801E-03  * x
 x <- ifelse(is.na(qdep),  17.0000000000000000 ,qdep)
 predict <- predict +    0.2883244474037118  * x
 x <- ifelse(is.na(wrsz),  85.7777777777777715 ,wrsz)
 predict <- predict +    4.0204744309917996E-04  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 } else {
 if(is.na(wrsz) | wrsz <=   54.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   18.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   22.0000000000000000 ){
 nodeid <-  2320
 predict <-    3.5074999999999998
 } else {
 nodeid <-  2321
 predict <-   14.4849999999999994
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 nodeid <-  2322
 predict <-   11.1474999999999991
 } else {
 nodeid <-  2323
 predict <-    6.0400000000000000
 }
 }
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(is.na(qdep) | qdep <=   50.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.7800000000000000E+02 ){
 nodeid <-  4648
 predict <-   12.6200000000000010
 } else {
 nodeid <-  4649
 predict <-   16.3825000000000003
 }
 } else {
 nodeid <-  2325
 predict <-   22.7349999999999994
 }
 } else {
 if(!is.na(qdep) & qdep <=   46.0000000000000000 ){
 nodeid <-  2326
 predict <-   13.6149999999999984
 } else {
 nodeid <-  2327
 predict <-   19.2899999999999991
 }
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   86.0000000000000000 ){
 if(is.na(qdep) | qdep <=   32.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    8.0000000000000000 ){
 nodeid <-  2328
 predict <-    1.1650000000000000
 } else {
 if(is.na(rrnd) | rrnd <=   25.0000000000000000 ){
 nodeid <-  4658
 predict <-    7.6933333333333325
 } else {
 nodeid <-  4659
 predict <-    5.3350000000000000
 }
 }
 } else {
 nodeid <-  1165
 predict <-   18.5650000000000013
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.1200000000000000E+02 ){
 if(is.na(qdep) | qdep <=   40.0000000000000000 ){
 nodeid <-  2332
 predict <-    4.0850000000000000
 } else {
 nodeid <-  2333
 predict <-   22.8300000000000018
 }
 } else {
 nodeid <-  1167
 predict <-    3.1124999999999998
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   38.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   20.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.8800000000000000E+02 ){
 if(!is.na(qdep) & qdep <=    2.5000000000000000 ){
 nodeid <-  1168
 predict <-    0.7750000000000000
 } else {
 if(!is.na(qdep) & qdep <=    6.0000000000000000 ){
 nodeid <-  2338
 predict <-    1.9350000000000001
 } else {
 if(!is.na(qdep) & qdep <=   14.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   10.0000000000000000 ){
 nodeid <-  9356
 predict <-    3.0325000000000002
 } else {
 nodeid <-  9357
 predict <-    4.6825000000000001
 }
 } else {
 nodeid <-  4679
 predict <-    6.4199999999999999
 }
 }
 }
 } else {
 nodeid <-  585
 predict <-    2.3100000000000001
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   30.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.6600000000000000E+02 ){
 nodeid <-  2344
 predict <-    9.8000000000000007
 } else {
 nodeid <-  2345
 predict <-   11.8100000000000005
 }
 } else {
 nodeid <-  1173
 predict <-   14.5775000000000006
 }
 } else {
 if(!is.na(qdep) & qdep <=   26.0000000000000000 ){
 nodeid <-  1174
 predict <-    8.6966666666666672
 } else {
 nodeid <-  1175
 predict <-   12.9600000000000009
 }
 }
 }
 } else {
 if(is.na(wrsz) | wrsz <=   58.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.5800000000000000E+02 ){
 nodeid <-  588
 predict <-   15.4933333333333341
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.8200000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=    1.7000000000000000E+02 ){
 nodeid <-  4712
 predict <-   19.3524999999999991
 } else {
 if(is.na(qdep) | qdep <=   46.0000000000000000 ){
 nodeid <-  9426
 predict <-   15.1233333333333348
 } else {
 nodeid <-  9427
 predict <-   20.8350000000000009
 }
 }
 } else {
 nodeid <-  2357
 predict <-   21.5249999999999986
 }
 } else {
 if(is.na(wrnd) | wrnd <=   25.0000000000000000 ){
 nodeid <-  2358
 predict <-   15.4333333333333353
 } else {
 nodeid <-  2359
 predict <-   20.4800000000000004
 }
 }
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(is.na(qdep) | qdep <=   54.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   46.0000000000000000 ){
 nodeid <-  2360
 predict <-   17.9549999999999983
 } else {
 if(!is.na(wrsz) & wrsz <=   86.0000000000000000 ){
 nodeid <-  4722
 predict <-   19.7199999999999989
 } else {
 nodeid <-  4723
 predict <-   20.5700000000000003
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   94.0000000000000000 ){
 nodeid <-  2362
 predict <-   23.3299999999999983
 } else {
 nodeid <-  2363
 predict <-   24.5333333333333314
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.0800000000000000E+02 ){
 nodeid <-  1182
 predict <-   18.8700000000000010
 } else {
 nodeid <-  1183
 predict <-   26.6299999999999990
 }
 }
 }
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   30.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    2.1000000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=    1.9800000000000000E+02 ){
 nodeid <-  296
 predict <-   17.5449999999999982
 } else {
 if(!is.na(rdsz) & rdsz <=    2.0200000000000000E+02 ){
 nodeid <-  594
 predict <-    3.5850000000000000
 } else {
 if(!is.na(rdsz) & rdsz <=    2.0600000000000000E+02 ){
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 nodeid <-  2380
 predict <-    8.9866666666666664
 } else {
 nodeid <-  2381
 predict <-   21.1574999999999989
 }
 } else {
 nodeid <-  1191
 predict <-   26.2199999999999989
 }
 }
 }
 } else {
 if(is.na(qdep) | qdep <=   34.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    6.0000000000000000 ){
 nodeid <-  596
 predict <-    1.4050000000000000
 } else {
 if(!is.na(rdsz) & rdsz <=    2.4000000000000000E+02 ){
 if(!is.na(wr) & wr <=   12.5000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    2.2000000000000000E+02 ){
 nodeid <-  4776
 predict <-   11.7233333333333345
 } else {
 nodeid <-  4777
 predict <-    7.1400000000000006
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   20.0000000000000000 ){
 nodeid <-  4778
 predict <-   17.6000000000000014
 } else {
 nodeid <-  4779
 predict <-    7.6200000000000001
 }
 }
 } else {
 nodeid <-  1195
 predict <-   11.9366666666666674
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    2.2200000000000000E+02 ){
 nodeid <-  598
 predict <-   24.0249999999999986
 } else {
 if(!is.na(rdsz) & rdsz <=    2.3600000000000000E+02 ){
 if(!is.na(wr) & wr <=   12.5000000000000000 ){
 nodeid <-  2396
 predict <-   20.3566666666666656
 } else {
 nodeid <-  2397
 predict <-   24.2175000000000011
 }
 } else {
 nodeid <-  1199
 predict <-   21.2549999999999990
 }
 }
 }
 }
 } else {
 if(is.na(wrsz) | wrsz <=   90.0000000000000000 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   62.0000000000000000 ){
 nodeid <-  600
 predict <-   -9.3865557552831032
 x <- ifelse(is.na(wrnd),  50.0000000000000000 ,wrnd)
 predict <- predict +   -9.4520654204432661E-04  * x
 x <- ifelse(is.na(rdsz),   2.1709090909090909E+02 ,rdsz)
 predict <- predict +    3.9186699921907112E-02  * x
 x <- ifelse(is.na(qdep),  34.5454545454545467 ,qdep)
 predict <- predict +    0.4645088782071310  * x
 x <- ifelse(is.na(wr),  13.6363636363636367 ,wr)
 predict <- predict +    7.1749170449191740E-02  * x
 x <- ifelse(is.na(wrsz),  44.3636363636363669 ,wrsz)
 predict <- predict +   -1.8691827445418446E-02  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 if(!is.na(rdsz) & rdsz <=    2.2600000000000000E+02 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  2404
 predict <-   15.0899999999999999
 } else {
 if(is.na(wrsz) | wrsz <=   80.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   24.0000000000000000 ){
 nodeid <-  9620
 predict <-    6.2166666666666659
 } else {
 nodeid <-  9621
 predict <-   15.5524999999999984
 }
 } else {
 nodeid <-  4811
 predict <-    7.7149999999999999
 }
 }
 } else {
 nodeid <-  1203
 predict <-    6.7999999999999998
 }
 }
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    8.0000000000000000 ){
 nodeid <-  2408
 predict <-    1.6549999999999998
 } else {
 if(!is.na(qdep) & qdep <=   28.0000000000000000 ){
 nodeid <-  4818
 predict <-    8.3849999999999998
 } else {
 if(is.na(qdep) | qdep <=   42.0000000000000000 ){
 nodeid <-  9638
 predict <-   16.3125000000000000
 } else {
 nodeid <-  9639
 predict <-   20.8833333333333329
 }
 }
 }
 } else {
 if(!is.na(wr) & wr <=   12.5000000000000000 ){
 if(is.na(qdep) | qdep <=   38.0000000000000000 ){
 nodeid <-  4820
 predict <-    9.1724999999999994
 } else {
 nodeid <-  4821
 predict <-   26.7166666666666650
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(is.na(qdep) | qdep <=   36.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    2.0800000000000000E+02 ){
 nodeid <-  19288
 predict <-    6.5899999999999999
 } else {
 nodeid <-  19289
 predict <-   13.5333333333333332
 }
 } else {
 nodeid <-  9645
 predict <-   22.8449999999999989
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   48.0000000000000000 ){
 nodeid <-  9646
 predict <-    1.5549999999999999
 } else {
 nodeid <-  9647
 predict <-   10.4299999999999997
 }
 }
 }
 }
 } else {
 nodeid <-  603
 predict <-   -9.5940056670108831
 x <- ifelse(is.na(wrnd),  44.1176470588235290 ,wrnd)
 predict <- predict +    7.2200859377229551E-03  * x
 x <- ifelse(is.na(rdsz),   2.2023529411764707E+02 ,rdsz)
 predict <- predict +    3.7888901719838677E-02  * x
 x <- ifelse(is.na(qdep),  23.4705882352941160 ,qdep)
 predict <- predict +    0.4247057281667475  * x
 x <- ifelse(is.na(wr),  10.2941176470588243 ,wr)
 predict <- predict +    1.4328337720940124E-02  * x
 x <- ifelse(is.na(wrsz),  63.0588235294117680 ,wrsz)
 predict <- predict +    1.2940241797686641E-02  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    2.3400000000000000E+02 ){
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.0200000000000000E+02 ){
 nodeid <-  1208
 predict <-   20.7250000000000014
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.1000000000000000E+02 ){
 nodeid <-  4836
 predict <-   18.0299999999999976
 } else {
 nodeid <-  4837
 predict <-    8.9849999999999994
 }
 } else {
 if(!is.na(wr) & wr <=   12.5000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    2.2400000000000000E+02 ){
 nodeid <-  9676
 predict <-    8.6099999999999994
 } else {
 nodeid <-  9677
 predict <-   22.7699999999999996
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    2.0600000000000000E+02 ){
 nodeid <-  9678
 predict <-   13.1299999999999990
 } else {
 nodeid <-  9679
 predict <-    3.5466666666666669
 }
 }
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.1200000000000000E+02 ){
 if(!is.na(wr) & wr <=   12.5000000000000000 ){
 nodeid <-  2420
 predict <-   23.0000000000000000
 } else {
 nodeid <-  2421
 predict <-    6.8066666666666675
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  2422
 predict <-    2.9950000000000001
 } else {
 nodeid <-  2423
 predict <-   16.3966666666666647
 }
 }
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(is.na(qdep) | qdep <=   36.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    2.4200000000000000E+02 ){
 nodeid <-  2424
 predict <-    7.1500000000000004
 } else {
 if(!is.na(qdep) & qdep <=   12.0000000000000000 ){
 nodeid <-  4850
 predict <-    1.8000000000000000
 } else {
 if(!is.na(wrsz) & wrsz <=    1.1400000000000000E+02 ){
 nodeid <-  9702
 predict <-   14.5000000000000000
 } else {
 nodeid <-  9703
 predict <-   11.1166666666666654
 }
 }
 }
 } else {
 nodeid <-  1213
 predict <-   27.2500000000000000
 }
 } else {
 nodeid <-  607
 predict <-   14.9433333333333334
 }
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   30.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   18.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    6.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    2.5000000000000000 ){
 if(is.na(wrsz) | wrsz <=   80.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   38.0000000000000000 ){
 nodeid <-  1216
 predict <-    0.6125000000000000
 } else {
 nodeid <-  1217
 predict <-    0.7224999999999999
 }
 } else {
 nodeid <-  609
 predict <-    0.9850000000000001
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   46.0000000000000000 ){
 nodeid <-  610
 predict <-    1.3425000000000000
 } else {
 if(!is.na(wrsz) & wrsz <=   72.0000000000000000 ){
 nodeid <-  1222
 predict <-    1.9366666666666668
 } else {
 nodeid <-  1223
 predict <-    2.3075000000000001
 }
 }
 }
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  1224
 predict <-    4.1649999999999991
 } else {
 nodeid <-  1225
 predict <-    5.3475000000000001
 }
 } else {
 if(!is.na(qdep) & qdep <=   12.0000000000000000 ){
 if(is.na(wrsz) | wrsz <=   84.0000000000000000 ){
 nodeid <-  2452
 predict <-    3.1875000000000000
 } else {
 nodeid <-  2453
 predict <-    3.9350000000000005
 }
 } else {
 nodeid <-  1227
 predict <-    7.0250000000000004
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    2.1000000000000000E+02 ){
 nodeid <-  614
 predict <-    3.6749999999999998
 } else {
 if(!is.na(rdsz) & rdsz <=    2.2800000000000000E+02 ){
 nodeid <-  1230
 predict <-    4.9850000000000003
 } else {
 nodeid <-  1231
 predict <-    4.4866666666666672
 }
 }
 }
 }
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   14.0000000000000000 ){
 nodeid <-  1232
 predict <-   10.8449999999999989
 } else {
 nodeid <-  1233
 predict <-   10.1233333333333331
 }
 } else {
 if(is.na(wrnd) | wrnd <=   25.0000000000000000 ){
 if(is.na(wrsz) | wrsz <=    1.0600000000000000E+02 ){
 nodeid <-  2468
 predict <-    8.9266666666666676
 } else {
 nodeid <-  2469
 predict <-   13.1850000000000005
 }
 } else {
 nodeid <-  1235
 predict <-    9.1374999999999993
 }
 }
 } else {
 nodeid <-  309
 predict <-    9.5250000000000004
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.7600000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=    1.5600000000000000E+02 ){
 nodeid <-  620
 predict <-   10.2599999999999998
 } else {
 nodeid <-  621
 predict <-    8.1099999999999994
 }
 } else {
 if(is.na(wrnd) | wrnd <=   50.0000000000000000 ){
 nodeid <-  622
 predict <-   12.3725000000000005
 } else {
 nodeid <-  623
 predict <-    9.9949999999999992
 }
 }
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   62.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   16.0000000000000000 ){
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    2.5000000000000000 ){
 nodeid <-  624
 predict <-   11.7850000000000001
 } else {
 if(!is.na(qdep) & qdep <=   58.0000000000000000 ){
 nodeid <-  1250
 predict <-   18.0925000000000011
 } else {
 nodeid <-  1251
 predict <-   19.7950000000000017
 }
 }
 } else {
 nodeid <-  313
 predict <-   13.3174999999999990
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.9400000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=    1.8400000000000000E+02 ){
 nodeid <-  1256
 predict <-   12.1449999999999996
 } else {
 nodeid <-  1257
 predict <-   14.0700000000000003
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   46.0000000000000000 ){
 nodeid <-  1258
 predict <-   16.2500000000000000
 } else {
 nodeid <-  1259
 predict <-   24.0450000000000017
 }
 }
 } else {
 if(is.na(qdep) | qdep <=   50.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   36.0000000000000000 ){
 nodeid <-  1260
 predict <-   13.7724999999999991
 } else {
 if(!is.na(qdep) & qdep <=   38.0000000000000000 ){
 nodeid <-  2522
 predict <-   13.0500000000000007
 } else {
 if(is.na(wrnd) | wrnd <=   25.0000000000000000 ){
 nodeid <-  5046
 predict <-   17.0300000000000011
 } else {
 nodeid <-  5047
 predict <-   15.6500000000000004
 }
 }
 }
 } else {
 nodeid <-  631
 predict <-   17.5749999999999993
 }
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   86.0000000000000000 ){
 if(is.na(qdep) | qdep <=   58.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 nodeid <-  632
 predict <-   14.3249999999999993
 } else {
 if(is.na(qdep) | qdep <=   46.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   50.0000000000000000 ){
 nodeid <-  2532
 predict <-   17.7450000000000010
 } else {
 if(!is.na(qdep) & qdep <=   38.0000000000000000 ){
 nodeid <-  5066
 predict <-   14.7550000000000008
 } else {
 nodeid <-  5067
 predict <-   18.6925000000000026
 }
 }
 } else {
 nodeid <-  1267
 predict <-   21.3466666666666640
 }
 }
 } else {
 nodeid <-  317
 predict <-   28.3625000000000007
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.9400000000000000E+02 ){
 nodeid <-  318
 predict <-  -23.5368261079651901
 x <- ifelse(is.na(wrnd),  50.0000000000000000 ,wrnd)
 predict <- predict +   -5.2423633373337294E-03  * x
 x <- ifelse(is.na(rrnd),  40.9090909090909065 ,rrnd)
 predict <- predict +   -1.3733246246595275E-02  * x
 x <- ifelse(is.na(rdsz),   1.7454545454545453E+02 ,rdsz)
 predict <- predict +    4.6433363200869909E-02  * x
 x <- ifelse(is.na(qdep),  42.1818181818181799 ,qdep)
 predict <- predict +    0.5543278516176577  * x
 x <- ifelse(is.na(wrsz),   1.0690909090909091E+02 ,wrsz)
 predict <- predict +    0.1374899382124617  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  319
 predict <-  -25.6394246175742317
 x <- ifelse(is.na(wrnd),  45.4545454545454533 ,wrnd)
 predict <- predict +   -1.9479180539300062E-03  * x
 x <- ifelse(is.na(rrnd),  54.5454545454545467 ,rrnd)
 predict <- predict +   -1.1677619984943216E-02  * x
 x <- ifelse(is.na(rdsz),   2.2327272727272728E+02 ,rdsz)
 predict <- predict +    6.0685359555503311E-02  * x
 x <- ifelse(is.na(qdep),  43.6363636363636331 ,qdep)
 predict <- predict +    0.5160418251545501  * x
 x <- ifelse(is.na(wrsz),  99.2727272727272663 ,wrsz)
 predict <- predict +    0.1386729694768749  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 }
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   30.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   14.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    6.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    2.5000000000000000 ){
 if(!is.na(wr) & wr <=   87.5000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   36.0000000000000000 ){
 nodeid <-  320
 predict <-    0.4300000000000000
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(is.na(wrsz) | wrsz <=   90.0000000000000000 ){
 nodeid <-  2568
 predict <-    0.5774999999999999
 } else {
 nodeid <-  2569
 predict <-    0.8200000000000000
 }
 } else {
 if(is.na(rdsz) | rdsz <=    1.7200000000000000E+02 ){
 nodeid <-  2570
 predict <-    0.4399999999999999
 } else {
 nodeid <-  2571
 predict <-    0.8166666666666665
 }
 }
 } else {
 if(is.na(rdsz) | rdsz <=    1.5400000000000000E+02 ){
 nodeid <-  1286
 predict <-    0.6025000000000000
 } else {
 nodeid <-  1287
 predict <-    0.4350000000000000
 }
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   20.0000000000000000 ){
 nodeid <-  322
 predict <-    8.6666666666666670E-02
 } else {
 nodeid <-  323
 predict <-    1.2955574191224561E-02
 x <- ifelse(is.na(wrnd),  61.5384615384615401 ,wrnd)
 predict <- predict +   -6.7581312695698696E-05  * x
 x <- ifelse(is.na(rrnd),  50.0000000000000000 ,rrnd)
 predict <- predict +   -7.4711458442678659E-05  * x
 x <- ifelse(is.na(rdsz),   1.5323076923076923E+02 ,rdsz)
 predict <- predict +   -3.9885491782848522E-05  * x
 x <- ifelse(is.na(wrsz),  67.6923076923076934 ,wrsz)
 predict <- predict +    6.1064283332838819E-03  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 }
 } else {
 if(!is.na(wr) & wr <=   87.5000000000000000 ){
 if(is.na(rdsz) | rdsz <=    1.5800000000000000E+02 ){
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=    1.2600000000000000E+02 ){
 if(is.na(wrsz) | wrsz <=   62.0000000000000000 ){
 if(is.na(wrnd) | wrnd <=   25.0000000000000000 ){
 nodeid <-  5184
 predict <-    0.4533333333333333
 } else {
 nodeid <-  5185
 predict <-    0.9800000000000000
 }
 } else {
 nodeid <-  2593
 predict <-    1.7200000000000000
 }
 } else {
 nodeid <-  1297
 predict <-    1.7800000000000000
 }
 } else {
 if(is.na(rdsz) | rdsz <=    1.1800000000000000E+02 ){
 nodeid <-  1298
 predict <-    1.2866666666666668
 } else {
 nodeid <-  1299
 predict <-    2.0800000000000001
 }
 }
 } else {
 nodeid <-  325
 predict <-    2.2249999999999996
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(is.na(wrsz) | wrsz <=   72.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   94.0000000000000000 ){
 nodeid <-  1304
 predict <-    0.8850000000000000
 } else {
 nodeid <-  1305
 predict <-    1.3399999999999999
 }
 } else {
 nodeid <-  653
 predict <-    2.5200000000000000
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   48.0000000000000000 ){
 nodeid <-  654
 predict <-    2.2566666666666664
 } else {
 if(!is.na(wrsz) & wrsz <=   28.0000000000000000 ){
 nodeid <-  1310
 predict <-    0.3150000000000000
 } else {
 nodeid <-  1311
 predict <-    1.1566666666666667
 }
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   10.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=    1.7400000000000000E+02 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(is.na(wrsz) | wrsz <=   68.0000000000000000 ){
 nodeid <-  656
 predict <-    1.7725000000000000
 } else {
 nodeid <-  657
 predict <-    4.5666666666666664
 }
 } else {
 if(is.na(wrsz) | wrsz <=    1.1400000000000000E+02 ){
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(is.na(wrsz) | wrsz <=   76.0000000000000000 ){
 if(!is.na(wr) & wr <=   87.5000000000000000 ){
 nodeid <-  5264
 predict <-    2.1800000000000002
 } else {
 nodeid <-  5265
 predict <-    3.1433333333333331
 }
 } else {
 nodeid <-  2633
 predict <-    4.7166666666666677
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   64.0000000000000000 ){
 nodeid <-  2634
 predict <-    0.6999999999999998
 } else {
 nodeid <-  2635
 predict <-    2.7599999999999998
 }
 }
 } else {
 nodeid <-  659
 predict <-    5.5400000000000000
 }
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  330
 predict <-    4.3224999999999998
 } else {
 if(!is.na(wrsz) & wrsz <=   28.0000000000000000 ){
 nodeid <-  662
 predict <-    0.8133333333333334
 } else {
 nodeid <-  663
 predict <-    3.5449999999999999
 }
 }
 }
 } else {
 if(is.na(wrsz) | wrsz <=   62.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   14.0000000000000000 ){
 nodeid <-  332
 predict <-    2.5200000000000000
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(is.na(wrsz) | wrsz <=   40.0000000000000000 ){
 nodeid <-  2664
 predict <-    1.8766666666666663
 } else {
 nodeid <-  2665
 predict <-    4.0300000000000002
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   24.0000000000000000 ){
 nodeid <-  2666
 predict <-    0.4566666666666666
 } else {
 nodeid <-  2667
 predict <-    3.7649999999999997
 }
 }
 } else {
 if(is.na(wrsz) | wrsz <=   38.0000000000000000 ){
 nodeid <-  1334
 predict <-    1.6200000000000001
 } else {
 nodeid <-  1335
 predict <-    3.7100000000000000
 }
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   70.0000000000000000 ){
 nodeid <-  334
 predict <-    4.7450000000000001
 } else {
 if(!is.na(wrsz) & wrsz <=   86.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.4800000000000000E+02 ){
 nodeid <-  1340
 predict <-    5.9399999999999995
 } else {
 nodeid <-  1341
 predict <-    6.0433333333333330
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.3200000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=   76.0000000000000000 ){
 nodeid <-  2684
 predict <-    7.0150000000000006
 } else {
 nodeid <-  2685
 predict <-    8.6633333333333340
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  2686
 predict <-    6.2833333333333323
 } else {
 nodeid <-  2687
 predict <-    7.1049999999999995
 }
 }
 }
 }
 }
 }
 }
 } else {
 if(!is.na(wr) & wr <=   87.5000000000000000 ){
 if(is.na(wrsz) | wrsz <=   70.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   18.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  336
 predict <-    3.4633333333333334
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   42.0000000000000000 ){
 nodeid <-  1348
 predict <-    2.1599999999999997
 } else {
 nodeid <-  1349
 predict <-    5.9600000000000000
 }
 } else {
 if(is.na(rdsz) | rdsz <=    1.2000000000000000E+02 ){
 nodeid <-  1350
 predict <-    1.8166666666666667
 } else {
 nodeid <-  1351
 predict <-    4.4950000000000001
 }
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   26.0000000000000000 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  676
 predict <-    5.2199999999999998
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  1354
 predict <-    3.1349999999999998
 } else {
 if(!is.na(qdep) & qdep <=   26.0000000000000000 ){
 nodeid <-  2710
 predict <-    3.8100000000000001
 } else {
 nodeid <-  2711
 predict <-    6.4899999999999993
 }
 }
 }
 } else {
 if(is.na(rdsz) | rdsz <=    2.3000000000000000E+02 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  1356
 predict <-    7.9100000000000001
 } else {
 if(is.na(wrsz) | wrsz <=   62.0000000000000000 ){
 if(is.na(wrsz) | wrsz <=   42.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.6400000000000000E+02 ){
 nodeid <-  10856
 predict <-    4.8500000000000005
 } else {
 nodeid <-  10857
 predict <-    6.1050000000000004
 }
 } else {
 nodeid <-  5429
 predict <-    7.9550000000000001
 }
 } else {
 nodeid <-  2715
 predict <-    9.8249999999999993
 }
 }
 } else {
 nodeid <-  679
 predict <-    8.4699999999999989
 }
 }
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   18.0000000000000000 ){
 nodeid <-  1360
 predict <-    7.6600000000000001
 } else {
 nodeid <-  1361
 predict <-   15.9966666666666644
 }
 } else {
 if(is.na(rdsz) | rdsz <=    1.2800000000000000E+02 ){
 if(!is.na(qdep) & qdep <=   24.0000000000000000 ){
 nodeid <-  2724
 predict <-    8.9000000000000004
 } else {
 nodeid <-  2725
 predict <-   15.4674999999999994
 }
 } else {
 nodeid <-  1363
 predict <-   10.1699999999999999
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.0000000000000000E+02 ){
 nodeid <-  682
 predict <-    9.2599999999999998
 } else {
 nodeid <-  683
 predict <-   15.6699999999999999
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   24.0000000000000000 ){
 nodeid <-  342
 predict <-   10.0400000000000009
 } else {
 nodeid <-  343
 predict <-   18.6449999999999996
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   22.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.4800000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=   10.0000000000000000 ){
 nodeid <-  344
 predict <-    0.6300000000000000
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(is.na(wrsz) | wrsz <=    1.1000000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=   66.0000000000000000 ){
 nodeid <-  2760
 predict <-    5.1150000000000002
 } else {
 nodeid <-  2761
 predict <-    8.3775000000000013
 }
 } else {
 nodeid <-  1381
 predict <-   14.9899999999999984
 }
 } else {
 if(is.na(wrsz) | wrsz <=   64.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   18.0000000000000000 ){
 nodeid <-  2764
 predict <-    5.2366666666666672
 } else {
 nodeid <-  2765
 predict <-    1.9550000000000001
 }
 } else {
 nodeid <-  1383
 predict <-    8.9550000000000001
 }
 }
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.7600000000000000E+02 ){
 nodeid <-  692
 predict <-   11.3300000000000001
 } else {
 if(is.na(wrsz) | wrsz <=   68.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=    2.1400000000000000E+02 ){
 nodeid <-  2772
 predict <-    4.3099999999999996
 } else {
 nodeid <-  2773
 predict <-    0.3500000000000000
 }
 } else {
 nodeid <-  1387
 predict <-   11.7366666666666664
 }
 }
 } else {
 if(is.na(wrsz) | wrsz <=   78.0000000000000000 ){
 nodeid <-  694
 predict <-    6.1224999999999996
 } else {
 nodeid <-  695
 predict <-   11.0066666666666659
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   26.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=    1.3200000000000000E+02 ){
 if(is.na(rdsz) | rdsz <=    1.2400000000000000E+02 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   82.0000000000000000 ){
 nodeid <-  2784
 predict <-    7.9400000000000004
 } else {
 nodeid <-  2785
 predict <-   15.9933333333333341
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   42.0000000000000000 ){
 nodeid <-  2786
 predict <-   13.7149999999999999
 } else {
 nodeid <-  2787
 predict <-    7.7625000000000002
 }
 }
 } else {
 nodeid <-  697
 predict <-    5.4866666666666672
 }
 } else {
 nodeid <-  349
 predict <-    1.4314089921183992E-02
 x <- ifelse(is.na(wrnd),  45.8333333333333357 ,wrnd)
 predict <- predict +   -8.0361092347600721E-04  * x
 x <- ifelse(is.na(rrnd),  41.6666666666666643 ,rrnd)
 predict <- predict +   -8.8687212195043275E-04  * x
 x <- ifelse(is.na(rdsz),   1.8966666666666666E+02 ,rdsz)
 predict <- predict +    3.6443867876805730E-04  * x
 x <- ifelse(is.na(wrsz),  54.3333333333333357 ,wrsz)
 predict <- predict +    0.1486720714063753  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 } else {
 nodeid <-  175
 predict <-    0.4372067214792708
 x <- ifelse(is.na(wrnd),  35.7142857142857153 ,wrnd)
 predict <- predict +   -3.8796505414363022E-03  * x
 x <- ifelse(is.na(rrnd),  54.7619047619047592 ,rrnd)
 predict <- predict +    2.9464191554601608E-03  * x
 x <- ifelse(is.na(rdsz),   1.0961904761904762E+02 ,rdsz)
 predict <- predict +   -2.1259751321276825E-03  * x
 x <- ifelse(is.na(wrsz),  70.6666666666666714 ,wrsz)
 predict <- predict +    0.1696792516714227  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 }
 }
 }
 } else {
 if(!is.na(wr) & wr <=   87.5000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=    1.9000000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=    1.2200000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=   36.0000000000000000 ){
 nodeid <-  352
 predict <-   10.1099999999999994
 } else {
 if(!is.na(wrsz) & wrsz <=   12.0000000000000000 ){
 nodeid <-  706
 predict <-    3.5150000000000001
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  1414
 predict <-   17.1275000000000013
 } else {
 if(is.na(wrsz) | wrsz <=   88.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   42.0000000000000000 ){
 nodeid <-  5660
 predict <-   11.8100000000000005
 } else {
 nodeid <-  5661
 predict <-   17.1033333333333353
 }
 } else {
 nodeid <-  2831
 predict <-   27.2450000000000010
 }
 }
 }
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   78.0000000000000000 ){
 nodeid <-  708
 predict <-   14.0433333333333312
 } else {
 nodeid <-  709
 predict <-   21.7850000000000037
 }
 } else {
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 nodeid <-  710
 predict <-   15.1149999999999984
 } else {
 if(is.na(qdep) | qdep <=   58.0000000000000000 ){
 if(is.na(qdep) | qdep <=   42.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.4400000000000000E+02 ){
 nodeid <-  5688
 predict <-   12.1699999999999999
 } else {
 nodeid <-  5689
 predict <-   18.2549999999999990
 }
 } else {
 nodeid <-  2845
 predict <-   14.1675000000000004
 }
 } else {
 nodeid <-  1423
 predict <-   22.0899999999999999
 }
 }
 }
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  178
 predict <-   17.0633333333333326
 } else {
 nodeid <-  179
 predict <-  -19.7631818778375816
 x <- ifelse(is.na(rrnd),  77.7777777777777715 ,rrnd)
 predict <- predict +   -2.7898213191833304E-03  * x
 x <- ifelse(is.na(rdsz),   2.2755555555555554E+02 ,rdsz)
 predict <- predict +   -1.1724284024108647E-03  * x
 x <- ifelse(is.na(qdep),  41.7777777777777786 ,qdep)
 predict <- predict +    0.6185707904545166  * x
 x <- ifelse(is.na(wrsz),  92.4444444444444429 ,wrsz)
 predict <- predict +    0.1915384951697185  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 }
 } else {
 if(is.na(qdep) | qdep <=   50.0000000000000000 ){
 if(is.na(qdep) | qdep <=   42.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   26.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    6.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.2600000000000000E+02 ){
 nodeid <-  1440
 predict <-    2.1000000000000001
 } else {
 nodeid <-  1441
 predict <-    7.6400000000000006
 }
 } else {
 if(is.na(rdsz) | rdsz <=    1.8000000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=    1.3000000000000000E+02 ){
 nodeid <-  2884
 predict <-    4.1600000000000001
 } else {
 nodeid <-  2885
 predict <-    6.3333333333333330
 }
 } else {
 nodeid <-  1443
 predict <-    9.3799999999999990
 }
 }
 } else {
 if(is.na(rdsz) | rdsz <=    1.2600000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=   30.0000000000000000 ){
 nodeid <-  1444
 predict <-    5.8300000000000001
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   38.0000000000000000 ){
 nodeid <-  5780
 predict <-    9.1124999999999989
 } else {
 nodeid <-  5781
 predict <-   11.3549999999999986
 }
 } else {
 if(is.na(wrsz) | wrsz <=    1.0000000000000000E+02 ){
 nodeid <-  5782
 predict <-   11.5474999999999994
 } else {
 nodeid <-  5783
 predict <-   20.7133333333333347
 }
 }
 }
 } else {
 nodeid <-  723
 predict <-  -13.1429619805479252
 x <- ifelse(is.na(wrnd),  71.4285714285714306 ,wrnd)
 predict <- predict +   -4.3310050110282258E-03  * x
 x <- ifelse(is.na(rrnd),  50.0000000000000000 ,rrnd)
 predict <- predict +   -4.9255550342952150E-03  * x
 x <- ifelse(is.na(rdsz),   1.9028571428571428E+02 ,rdsz)
 predict <- predict +    2.8395001762288145E-02  * x
 x <- ifelse(is.na(qdep),  35.1428571428571459 ,qdep)
 predict <- predict +    0.3743744809670606  * x
 x <- ifelse(is.na(wrsz),  72.2857142857142918 ,wrsz)
 predict <- predict +    0.1515354070476052  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 }
 } else {
 if(is.na(wrsz) | wrsz <=   86.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    4.5000000000000000 ){
 nodeid <-  724
 predict <-    4.6550000000000002
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(rrnd) & rrnd <=   75.0000000000000000 ){
 if(is.na(wrsz) | wrsz <=   56.0000000000000000 ){
 nodeid <-  5800
 predict <-   10.3266666666666662
 } else {
 nodeid <-  5801
 predict <-   20.9549999999999983
 }
 } else {
 if(is.na(wrsz) | wrsz <=   46.0000000000000000 ){
 nodeid <-  5802
 predict <-   10.5633333333333326
 } else {
 nodeid <-  5803
 predict <-   12.1699999999999999
 }
 }
 } else {
 if(is.na(rdsz) | rdsz <=    1.9200000000000000E+02 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  5804
 predict <-   12.8700000000000010
 } else {
 nodeid <-  5805
 predict <-   10.0225000000000009
 }
 } else {
 nodeid <-  2903
 predict <-   13.4733333333333345
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   50.0000000000000000 ){
 nodeid <-  726
 predict <-   28.4133333333333304
 } else {
 if(!is.na(rdsz) & rdsz <=    1.5200000000000000E+02 ){
 nodeid <-  1454
 predict <-   23.8300000000000018
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 nodeid <-  2910
 predict <-   27.7600000000000016
 } else {
 nodeid <-  2911
 predict <-   23.9649999999999999
 }
 }
 }
 }
 }
 } else {
 if(is.na(wrsz) | wrsz <=   60.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   54.0000000000000000 ){
 if(is.na(wrsz) | wrsz <=   50.0000000000000000 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 nodeid <-  1456
 predict <-    8.8324999999999996
 } else {
 nodeid <-  1457
 predict <-   12.1449999999999996
 }
 } else {
 nodeid <-  729
 predict <-   17.9600000000000009
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    8.0000000000000000 ){
 nodeid <-  730
 predict <-    5.2599999999999998
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(is.na(wrsz) | wrsz <=   34.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   75.0000000000000000 ){
 nodeid <-  5848
 predict <-   11.2200000000000006
 } else {
 if(!is.na(rdsz) & rdsz <=    1.8200000000000000E+02 ){
 nodeid <-  11698
 predict <-   11.8599999999999994
 } else {
 nodeid <-  11699
 predict <-   14.4074999999999989
 }
 }
 } else {
 nodeid <-  2925
 predict <-   14.9533333333333331
 }
 } else {
 nodeid <-  1463
 predict <-   13.0949999999999989
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   54.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   75.0000000000000000 ){
 nodeid <-  732
 predict <-   24.6200000000000010
 } else {
 nodeid <-  733
 predict <-   28.5349999999999966
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   64.0000000000000000 ){
 nodeid <-  1468
 predict <-   22.6266666666666652
 } else {
 if(!is.na(wrsz) & wrsz <=   88.0000000000000000 ){
 nodeid <-  2938
 predict <-   27.0500000000000007
 } else {
 nodeid <-  2939
 predict <-   32.3200000000000003
 }
 }
 } else {
 nodeid <-  735
 predict <-   28.3175000000000026
 }
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   46.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   84.0000000000000000 ){
 nodeid <-  184
 predict <-    0.9450099676339008
 x <- ifelse(is.na(wrnd),  54.5454545454545467 ,wrnd)
 predict <- predict +   -8.9776586280815446E-03  * x
 x <- ifelse(is.na(rrnd),  63.6363636363636331 ,rrnd)
 predict <- predict +   -4.1737224228292347E-03  * x
 x <- ifelse(is.na(rdsz),  35.0000000000000000 ,rdsz)
 predict <- predict +   -3.0709370572903905E-04  * x
 x <- ifelse(is.na(wrsz),  67.6363636363636402 ,wrsz)
 predict <- predict +    0.1987248955719921  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  185
 predict <-    0.9313390136159825
 x <- ifelse(is.na(wrnd),  72.7272727272727337 ,wrnd)
 predict <- predict +   -5.9007864887789252E-03  * x
 x <- ifelse(is.na(rrnd),  68.1818181818181870 ,rrnd)
 predict <- predict +    1.3545741245919590E-03  * x
 x <- ifelse(is.na(rdsz),   1.7490909090909091E+02 ,rdsz)
 predict <- predict +   -1.0709349888233608E-03  * x
 x <- ifelse(is.na(wrsz),  70.5454545454545467 ,wrsz)
 predict <- predict +    0.1906062478947158  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 } else {
 if(is.na(rdsz) | rdsz <=    1.5600000000000000E+02 ){
 if(is.na(rdsz) | rdsz <=   84.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=   74.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   42.0000000000000000 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  2976
 predict <-    7.6050000000000004
 } else {
 if(!is.na(rdsz) & rdsz <=   30.0000000000000000 ){
 nodeid <-  5954
 predict <-   20.3000000000000007
 } else {
 if(!is.na(wrsz) & wrsz <=   54.0000000000000000 ){
 nodeid <-  11910
 predict <-    7.2633333333333328
 } else {
 if(is.na(rdsz) | rdsz <=   54.0000000000000000 ){
 nodeid <-  23822
 predict <-   16.7433333333333358
 } else {
 nodeid <-  23823
 predict <-   21.7749999999999986
 }
 }
 }
 }
 } else {
 nodeid <-  1489
 predict <-   11.4566666666666652
 }
 } else {
 nodeid <-  745
 predict <-   12.2599999999999998
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=   78.0000000000000000 ){
 nodeid <-  1492
 predict <-   15.5733333333333324
 } else {
 nodeid <-  1493
 predict <-   26.4866666666666681
 }
 } else {
 if(is.na(wrsz) | wrsz <=   32.0000000000000000 ){
 nodeid <-  1494
 predict <-    4.9799999999999995
 } else {
 nodeid <-  1495
 predict <-   16.5899999999999999
 }
 }
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 nodeid <-  374
 predict <-   -9.2877755321750968
 x <- ifelse(is.na(wrnd),  28.5714285714285730 ,wrnd)
 predict <- predict +    1.3987846623536877E-02  * x
 x <- ifelse(is.na(rrnd),  53.5714285714285694 ,rrnd)
 predict <- predict +   -3.6953417348218678E-03  * x
 x <- ifelse(is.na(rdsz),   1.9485714285714286E+02 ,rdsz)
 predict <- predict +   -1.1860451274117993E-02  * x
 x <- ifelse(is.na(qdep),  39.1428571428571459 ,qdep)
 predict <- predict +    0.3081601899057522  * x
 x <- ifelse(is.na(wrsz),  44.9285714285714306 ,wrsz)
 predict <- predict +    0.2323986520025352  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 if(!is.na(rdsz) & rdsz <=    2.2400000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=   74.0000000000000000 ){
 nodeid <-  1500
 predict <-   16.3099999999999987
 } else {
 nodeid <-  1501
 predict <-   22.4800000000000004
 }
 } else {
 nodeid <-  751
 predict <-    9.6500000000000004
 }
 }
 }
 }
 } else {
 if(is.na(qdep) | qdep <=   54.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   50.0000000000000000 ){
 nodeid <-  188
 predict <-    0.3541158114043501
 x <- ifelse(is.na(wrnd),  55.0000000000000000 ,wrnd)
 predict <- predict +   -3.7863174862963062E-03  * x
 x <- ifelse(is.na(rrnd),  50.0000000000000000 ,rrnd)
 predict <- predict +   -2.3649388316245024E-04  * x
 x <- ifelse(is.na(rdsz),   1.3200000000000000E+02 ,rdsz)
 predict <- predict +    4.9622563627200328E-04  * x
 x <- ifelse(is.na(wrsz),  46.8500000000000014 ,wrsz)
 predict <- predict +    0.2925497238102916  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 if(!is.na(rdsz) & rdsz <=    1.1400000000000000E+02 ){
 if(is.na(wrsz) | wrsz <=    1.2000000000000000E+02 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  1512
 predict <-    9.6266666666666669
 } else {
 nodeid <-  1513
 predict <-    3.1072753694988080
 x <- ifelse(is.na(wrnd),  77.7777777777777715 ,wrnd)
 predict <- predict +   -2.5974163432692134E-02  * x
 x <- ifelse(is.na(rrnd),  61.1111111111111143 ,rrnd)
 predict <- predict +    7.4838284754907003E-03  * x
 x <- ifelse(is.na(rdsz),  52.8888888888888857 ,rdsz)
 predict <- predict +   -1.4523442875622683E-02  * x
 x <- ifelse(is.na(wrsz),  72.0000000000000000 ,wrsz)
 predict <- predict +    0.3081689648528885  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 } else {
 nodeid <-  757
 predict <-   39.6850000000000023
 }
 } else {
 nodeid <-  379
 predict <-    1.0129511053106768
 x <- ifelse(is.na(wrnd),  53.5714285714285694 ,wrnd)
 predict <- predict +    3.7801671437915729E-04  * x
 x <- ifelse(is.na(rrnd),  60.7142857142857153 ,rrnd)
 predict <- predict +   -6.8048059822286844E-03  * x
 x <- ifelse(is.na(rdsz),   1.7171428571428572E+02 ,rdsz)
 predict <- predict +   -1.2385167217011245E-03  * x
 x <- ifelse(is.na(wrsz),  57.5000000000000000 ,wrsz)
 predict <- predict +    0.3141449084145715  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   58.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   78.0000000000000000 ){
 nodeid <-  380
 predict <-    2.8696671147910635E-02
 x <- ifelse(is.na(wrnd),  27.2727272727272734 ,wrnd)
 predict <- predict +   -3.2062512893758985E-03  * x
 x <- ifelse(is.na(rrnd),  50.0000000000000000 ,rrnd)
 predict <- predict +   -2.4584205195364375E-03  * x
 x <- ifelse(is.na(rdsz),  46.1818181818181799 ,rdsz)
 predict <- predict +    1.5532653222291914E-02  * x
 x <- ifelse(is.na(wrsz),  38.1818181818181799 ,wrsz)
 predict <- predict +    0.3369470367928727  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 if(!is.na(rdsz) & rdsz <=   86.0000000000000000 ){
 nodeid <-  762
 predict <-   27.2999999999999972
 } else {
 if(is.na(rdsz) | rdsz <=    2.4600000000000000E+02 ){
 nodeid <-  1526
 predict <-   -1.4087446063533271
 x <- ifelse(is.na(wrnd),  29.1666666666666679 ,wrnd)
 predict <- predict +   -5.4658660326235876E-03  * x
 x <- ifelse(is.na(rrnd),  62.5000000000000000 ,rrnd)
 predict <- predict +    1.1037361366921294E-02  * x
 x <- ifelse(is.na(rdsz),   1.7100000000000000E+02 ,rdsz)
 predict <- predict +    3.7167486471092158E-03  * x
 x <- ifelse(is.na(wrsz),  73.0000000000000000 ,wrsz)
 predict <- predict +    0.3505401816650674  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  1527
 predict <-   21.2399999999999984
 }
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=   18.0000000000000000 ){
 nodeid <-  382
 predict <-    4.3949999999999996
 } else {
 if(!is.na(rdsz) & rdsz <=   40.0000000000000000 ){
 nodeid <-  766
 predict <-   36.9099999999999966
 } else {
 nodeid <-  767
 predict <-    0.1984558368181730
 x <- ifelse(is.na(wrnd),  50.0000000000000000 ,wrnd)
 predict <- predict +   -1.3795516016155834E-03  * x
 x <- ifelse(is.na(rrnd),  70.0000000000000000 ,rrnd)
 predict <- predict +    3.1635625667158877E-04  * x
 x <- ifelse(is.na(rdsz),   1.4119999999999999E+02 ,rdsz)
 predict <- predict +   -4.4112910226322063E-04  * x
 x <- ifelse(is.na(wrsz),  48.3999999999999986 ,wrsz)
 predict <- predict +    0.3664600048457679  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 }
 }
 }
 }
 }
 }
 }
 } else {
 if(is.na(wr) | wr <=   62.5000000000000000 ){
 if(!is.na(wr) & wr <=   12.5000000000000000 ){
 if(!is.na(qdep) & qdep <=   30.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.1800000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=   62.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   18.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   24.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  768
 predict <-    0.3066666666666666
 } else {
 if(!is.na(rdsz) & rdsz <=   10.0000000000000000 ){
 nodeid <-  1538
 predict <-    0.1900000000000000
 } else {
 nodeid <-  1539
 predict <-    0.4100000000000000
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   10.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  1540
 predict <-    0.4700000000000000
 } else {
 if(!is.na(rdsz) & rdsz <=   30.0000000000000000 ){
 nodeid <-  3082
 predict <-    0.4566666666666667
 } else {
 nodeid <-  3083
 predict <-    0.7150000000000001
 }
 }
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 nodeid <-  1542
 predict <-    1.6599999999999999
 } else {
 nodeid <-  1543
 predict <-    1.0049999999999999
 }
 }
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  386
 predict <-    0.7900000000000000
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   26.0000000000000000 ){
 nodeid <-  1548
 predict <-    2.4850000000000003
 } else {
 nodeid <-  1549
 predict <-    2.6333333333333333
 }
 } else {
 nodeid <-  775
 predict <-    1.1533333333333333
 }
 }
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   86.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   82.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   10.0000000000000000 ){
 nodeid <-  1552
 predict <-    0.8633333333333333
 } else {
 if(!is.na(rdsz) & rdsz <=   70.0000000000000000 ){
 nodeid <-  3106
 predict <-    2.7966666666666669
 } else {
 nodeid <-  3107
 predict <-    3.4900000000000002
 }
 }
 } else {
 nodeid <-  777
 predict <-    3.3466666666666662
 }
 } else {
 if(!is.na(qdep) & qdep <=   18.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   90.0000000000000000 ){
 nodeid <-  1556
 predict <-    2.7100000000000000
 } else {
 nodeid <-  1557
 predict <-    1.2625000000000000
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  1558
 predict <-    4.2149999999999999
 } else {
 nodeid <-  1559
 predict <-    5.0799999999999992
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   78.0000000000000000 ){
 nodeid <-  390
 predict <-    0.7749999999999999
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   14.0000000000000000 ){
 nodeid <-  1564
 predict <-    1.0899999999999999
 } else {
 nodeid <-  1565
 predict <-    5.2800000000000002
 }
 } else {
 nodeid <-  783
 predict <-    1.8750000000000000
 }
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.8000000000000000E+02 ){
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.6600000000000000E+02 ){
 nodeid <-  1568
 predict <-    7.9024999999999999
 } else {
 nodeid <-  1569
 predict <-    4.2850000000000001
 }
 } else {
 nodeid <-  785
 predict <-    4.2199999999999998
 }
 } else {
 if(!is.na(qdep) & qdep <=    2.5000000000000000 ){
 nodeid <-  786
 predict <-    0.7066666666666667
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   22.0000000000000000 ){
 nodeid <-  3148
 predict <-    2.5733333333333337
 } else {
 nodeid <-  3149
 predict <-    9.3524999999999991
 }
 } else {
 if(!is.na(qdep) & qdep <=   16.0000000000000000 ){
 nodeid <-  3150
 predict <-    1.5000000000000000
 } else {
 nodeid <-  3151
 predict <-    7.6950000000000003
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   18.0000000000000000 ){
 nodeid <-  394
 predict <-    2.4674999999999998
 } else {
 nodeid <-  395
 predict <-    8.3533333333333335
 }
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=    2.5000000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    1.5000000000000000E+02 ){
 nodeid <-  792
 predict <-    4.1399999999999997
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  1586
 predict <-    3.6766666666666663
 } else {
 if(is.na(rdsz) | rdsz <=    2.4200000000000000E+02 ){
 if(!is.na(qdep) & qdep <=   14.0000000000000000 ){
 nodeid <-  6348
 predict <-    2.2850000000000001
 } else {
 if(!is.na(wrnd) & wrnd <=   75.0000000000000000 ){
 nodeid <-  12698
 predict <-   13.9499999999999993
 } else {
 nodeid <-  12699
 predict <-    9.2133333333333329
 }
 }
 } else {
 nodeid <-  3175
 predict <-    2.9800000000000000
 }
 }
 }
 } else {
 nodeid <-  397
 predict <-    2.9950000000000001
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    2.4200000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    2.3400000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    2.2800000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    2.1600000000000000E+02 ){
 if(!is.na(qdep) & qdep <=   16.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    6.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    2.5000000000000000 ){
 nodeid <-  25472
 predict <-    1.0500000000000000
 } else {
 nodeid <-  25473
 predict <-    1.7600000000000000
 }
 } else {
 nodeid <-  12737
 predict <-    3.6600000000000001
 }
 } else {
 nodeid <-  6369
 predict <-   10.9800000000000004
 }
 } else {
 nodeid <-  3185
 predict <-    6.2349999999999994
 }
 } else {
 nodeid <-  1593
 predict <-    5.6533333333333333
 }
 } else {
 nodeid <-  797
 predict <-    5.1050000000000004
 }
 } else {
 nodeid <-  399
 predict <-    3.9650000000000003
 }
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.0600000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=   42.0000000000000000 ){
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   10.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   52.0000000000000000 ){
 nodeid <-  800
 predict <-    0.5525000000000000
 } else {
 nodeid <-  801
 predict <-    0.9750000000000000
 }
 } else {
 if(is.na(qdep) | qdep <=   56.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   36.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    2.0800000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=   18.0000000000000000 ){
 nodeid <-  6416
 predict <-    1.6299999999999999
 } else {
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 nodeid <-  12834
 predict <-    1.9099999999999999
 } else {
 nodeid <-  12835
 predict <-    2.5674999999999999
 }
 }
 } else {
 nodeid <-  3209
 predict <-    2.7199999999999998
 }
 } else {
 nodeid <-  1605
 predict <-    3.4249999999999998
 }
 } else {
 nodeid <-  803
 predict <-    4.5750000000000002
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    8.0000000000000000 ){
 nodeid <-  402
 predict <-    0.6000000000000000
 } else {
 if(!is.na(wrsz) & wrsz <=    2.4600000000000000E+02 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   26.0000000000000000 ){
 if(is.na(qdep) | qdep <=   46.0000000000000000 ){
 nodeid <-  6448
 predict <-    2.0200000000000000
 } else {
 nodeid <-  6449
 predict <-    2.1850000000000001
 }
 } else {
 nodeid <-  3225
 predict <-    3.2450000000000001
 }
 } else {
 nodeid <-  1613
 predict <-    2.9375000000000000
 }
 } else {
 nodeid <-  807
 predict <-    2.7450000000000001
 }
 }
 }
 } else {
 if(is.na(qdep) | qdep <=   50.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    2.1400000000000000E+02 ){
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 nodeid <-  808
 predict <-    5.6200000000000001
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    2.0200000000000000E+02 ){
 if(is.na(qdep) | qdep <=   42.0000000000000000 ){
 nodeid <-  6472
 predict <-    5.4550000000000001
 } else {
 nodeid <-  6473
 predict <-    7.0300000000000002
 }
 } else {
 nodeid <-  3237
 predict <-    7.7599999999999998
 }
 } else {
 nodeid <-  1619
 predict <-    6.2033333333333331
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 nodeid <-  810
 predict <-    5.1549999999999994
 } else {
 if(!is.na(rdsz) & rdsz <=   58.0000000000000000 ){
 nodeid <-  1622
 predict <-    4.4649999999999999
 } else {
 if(!is.na(rdsz) & rdsz <=   86.0000000000000000 ){
 if(is.na(qdep) | qdep <=   46.0000000000000000 ){
 nodeid <-  6492
 predict <-    7.1266666666666678
 } else {
 nodeid <-  6493
 predict <-    8.0449999999999999
 }
 } else {
 nodeid <-  3247
 predict <-    8.8800000000000008
 }
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   94.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   75.0000000000000000 ){
 nodeid <-  812
 predict <-    6.6025000000000000
 } else {
 if(!is.na(rdsz) & rdsz <=   76.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   54.0000000000000000 ){
 nodeid <-  3252
 predict <-    6.0800000000000001
 } else {
 nodeid <-  3253
 predict <-    7.1699999999999990
 }
 } else {
 nodeid <-  1627
 predict <-    9.0350000000000001
 }
 }
 } else {
 nodeid <-  407
 predict <-   10.8650000000000002
 }
 }
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.4600000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    1.3400000000000000E+02 ){
 if(is.na(wrnd) | wrnd <=   25.0000000000000000 ){
 nodeid <-  816
 predict <-   14.0266666666666655
 } else {
 nodeid <-  817
 predict <-   26.4349999999999987
 }
 } else {
 if(!is.na(qdep) & qdep <=   38.0000000000000000 ){
 nodeid <-  818
 predict <-    8.0899999999999999
 } else {
 nodeid <-  819
 predict <-   13.3033333333333328
 }
 }
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.6400000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=    1.3400000000000000E+02 ){
 nodeid <-  3280
 predict <-   10.7149999999999999
 } else {
 nodeid <-  3281
 predict <-   12.6833333333333318
 }
 } else {
 if(is.na(qdep) | qdep <=   56.0000000000000000 ){
 nodeid <-  3282
 predict <-   20.1966666666666654
 } else {
 nodeid <-  3283
 predict <-   24.2250000000000014
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 nodeid <-  1642
 predict <-    8.3650000000000002
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   42.0000000000000000 ){
 nodeid <-  6572
 predict <-   12.7750000000000004
 } else {
 if(is.na(rdsz) | rdsz <=    1.7800000000000000E+02 ){
 nodeid <-  13146
 predict <-   14.6274999999999995
 } else {
 nodeid <-  13147
 predict <-   19.0049999999999990
 }
 }
 } else {
 if(is.na(rdsz) | rdsz <=    2.0000000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=    1.3400000000000000E+02 ){
 nodeid <-  13148
 predict <-   11.5500000000000007
 } else {
 if(!is.na(qdep) & qdep <=   48.0000000000000000 ){
 nodeid <-  26298
 predict <-   14.5199999999999996
 } else {
 nodeid <-  26299
 predict <-   17.1466666666666647
 }
 }
 } else {
 nodeid <-  6575
 predict <-   21.6024999999999991
 }
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.3400000000000000E+02 ){
 nodeid <-  822
 predict <-   12.7500000000000000
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=    2.1400000000000000E+02 ){
 nodeid <-  3292
 predict <-   14.9799999999999986
 } else {
 nodeid <-  3293
 predict <-   24.8449999999999989
 }
 } else {
 if(!is.na(qdep) & qdep <=   42.0000000000000000 ){
 nodeid <-  3294
 predict <-   13.2933333333333312
 } else {
 nodeid <-  3295
 predict <-   22.8624999999999972
 }
 }
 }
 }
 }
 } else {
 if(is.na(rdsz) | rdsz <=    2.1000000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=    1.4000000000000000E+02 ){
 if(!is.na(qdep) & qdep <=   46.0000000000000000 ){
 nodeid <-  824
 predict <-    8.8350000000000009
 } else {
 nodeid <-  825
 predict <-   12.0574999999999992
 }
 } else {
 if(is.na(qdep) | qdep <=   58.0000000000000000 ){
 if(!is.na(rrnd) & rrnd <=   75.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   36.0000000000000000 ){
 nodeid <-  3304
 predict <-   10.1999999999999993
 } else {
 nodeid <-  3305
 predict <-   14.6933333333333334
 }
 } else {
 if(is.na(rdsz) | rdsz <=    1.9000000000000000E+02 ){
 nodeid <-  3306
 predict <-   16.0674999999999990
 } else {
 nodeid <-  3307
 predict <-   25.0549999999999997
 }
 }
 } else {
 nodeid <-  827
 predict <-   19.3550000000000004
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    2.0600000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=    2.2600000000000000E+02 ){
 nodeid <-  828
 predict <-   15.4233333333333320
 } else {
 if(!is.na(wrsz) & wrsz <=    1.8400000000000000E+02 ){
 nodeid <-  1658
 predict <-   28.4933333333333287
 } else {
 nodeid <-  1659
 predict <-   23.3750000000000000
 }
 }
 } else {
 nodeid <-  415
 predict <-   28.1000000000000050
 }
 }
 }
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    2.0600000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=    1.2200000000000000E+02 ){
 if(!is.na(qdep) & qdep <=   26.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   10.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    2.5000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    2.0200000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    1.5200000000000000E+02 ){
 nodeid <-  1664
 predict <-    0.7300000000000000
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  3330
 predict <-    0.8400000000000001
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   32.0000000000000000 ){
 nodeid <-  13324
 predict <-    0.6250000000000000
 } else {
 if(is.na(wrnd) | wrnd <=   25.0000000000000000 ){
 nodeid <-  26650
 predict <-    0.6625000000000001
 } else {
 nodeid <-  26651
 predict <-    0.7900000000000000
 }
 }
 } else {
 nodeid <-  6663
 predict <-    0.9300000000000000
 }
 }
 }
 } else {
 nodeid <-  833
 predict <-    0.9650000000000000
 }
 } else {
 if(!is.na(wr) & wr <=   37.5000000000000000 ){
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   42.0000000000000000 ){
 nodeid <-  3336
 predict <-    1.5000000000000000
 } else {
 if(!is.na(qdep) & qdep <=    6.0000000000000000 ){
 nodeid <-  6674
 predict <-    1.9424999999999999
 } else {
 nodeid <-  6675
 predict <-    3.1766666666666672
 }
 }
 } else {
 nodeid <-  1669
 predict <-    1.8566666666666667
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   26.0000000000000000 ){
 nodeid <-  1670
 predict <-    4.6250000000000000
 } else {
 if(!is.na(rdsz) & rdsz <=   50.0000000000000000 ){
 nodeid <-  3342
 predict <-    2.2549999999999999
 } else {
 if(!is.na(wrsz) & wrsz <=    1.5000000000000000E+02 ){
 nodeid <-  6686
 predict <-    4.2575000000000003
 } else {
 if(!is.na(qdep) & qdep <=    6.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.7600000000000000E+02 ){
 nodeid <-  26748
 predict <-    2.5825000000000000
 } else {
 nodeid <-  26749
 predict <-    3.0350000000000001
 }
 } else {
 nodeid <-  13375
 predict <-    5.6400000000000006
 }
 }
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   18.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   14.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   75.0000000000000000 ){
 nodeid <-  1672
 predict <-    4.9124999999999996
 } else {
 if(!is.na(wr) & wr <=   37.5000000000000000 ){
 nodeid <-  3346
 predict <-    4.8150000000000004
 } else {
 if(!is.na(wrsz) & wrsz <=    1.5800000000000000E+02 ){
 nodeid <-  6694
 predict <-    5.8700000000000001
 } else {
 nodeid <-  6695
 predict <-    7.6633333333333331
 }
 }
 }
 } else {
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.0600000000000000E+02 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.7600000000000000E+02 ){
 nodeid <-  13392
 predict <-    5.4924999999999997
 } else {
 nodeid <-  13393
 predict <-    6.8300000000000001
 }
 } else {
 nodeid <-  6697
 predict <-    5.3633333333333333
 }
 } else {
 nodeid <-  3349
 predict <-    6.7199999999999998
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 nodeid <-  3350
 predict <-    9.2699999999999996
 } else {
 nodeid <-  3351
 predict <-    8.0733333333333324
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   34.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.6000000000000000E+02 ){
 nodeid <-  1676
 predict <-    6.7925000000000004
 } else {
 if(!is.na(rrnd) & rrnd <=   75.0000000000000000 ){
 nodeid <-  3354
 predict <-   11.2824999999999989
 } else {
 nodeid <-  3355
 predict <-   12.7575000000000003
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   98.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.4600000000000000E+02 ){
 nodeid <-  3356
 predict <-    7.2275000000000000
 } else {
 if(!is.na(wr) & wr <=   37.5000000000000000 ){
 nodeid <-  6714
 predict <-    8.0666666666666682
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 nodeid <-  13430
 predict <-   12.9800000000000022
 } else {
 nodeid <-  13431
 predict <-   16.2749999999999986
 }
 }
 }
 } else {
 nodeid <-  1679
 predict <-   10.5950000000000006
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   42.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 if(!is.na(wr) & wr <=   37.5000000000000000 ){
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.5200000000000000E+02 ){
 nodeid <-  3360
 predict <-   10.9050000000000011
 } else {
 if(is.na(rrnd) | rrnd <=   25.0000000000000000 ){
 nodeid <-  6722
 predict <-    8.8599999999999994
 } else {
 nodeid <-  6723
 predict <-   11.3066666666666666
 }
 }
 } else {
 nodeid <-  1681
 predict <-   10.1224999999999987
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  1682
 predict <-   18.7033333333333331
 } else {
 if(!is.na(qdep) & qdep <=   30.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.5800000000000000E+02 ){
 nodeid <-  6732
 predict <-   14.1899999999999995
 } else {
 nodeid <-  6733
 predict <-   16.7025000000000006
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.7600000000000000E+02 ){
 nodeid <-  6734
 predict <-   19.0866666666666660
 } else {
 nodeid <-  6735
 predict <-   21.9549999999999983
 }
 }
 }
 }
 } else {
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   88.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.4800000000000000E+02 ){
 nodeid <-  6736
 predict <-    9.9166666666666661
 } else {
 nodeid <-  6737
 predict <-   12.9299999999999997
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   75.0000000000000000 ){
 nodeid <-  6738
 predict <-   15.1750000000000007
 } else {
 nodeid <-  6739
 predict <-   16.8866666666666667
 }
 }
 } else {
 nodeid <-  1685
 predict <-   15.1349999999999998
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   84.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   50.0000000000000000 ){
 nodeid <-  3372
 predict <-   17.9266666666666659
 } else {
 nodeid <-  3373
 predict <-   19.4849999999999994
 }
 } else {
 nodeid <-  1687
 predict <-   23.1533333333333324
 }
 }
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   70.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   62.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.3800000000000000E+02 ){
 nodeid <-  3376
 predict <-   20.3750000000000000
 } else {
 if(!is.na(qdep) & qdep <=   50.0000000000000000 ){
 nodeid <-  6754
 predict <-   23.0166666666666693
 } else {
 if(!is.na(wr) & wr <=   37.5000000000000000 ){
 nodeid <-  13510
 predict <-   20.1250000000000000
 } else {
 if(!is.na(wrsz) & wrsz <=    1.5000000000000000E+02 ){
 nodeid <-  27022
 predict <-   25.9900000000000020
 } else {
 nodeid <-  27023
 predict <-   34.3275000000000006
 }
 }
 }
 }
 } else {
 nodeid <-  1689
 predict <-   26.0466666666666669
 }
 } else {
 nodeid <-  845
 predict <-   33.2749999999999986
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   98.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   48.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   36.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   22.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    4.5000000000000000 ){
 nodeid <-  13536
 predict <-   21.0400000000000027
 } else {
 nodeid <-  13537
 predict <-   28.4899999999999984
 }
 } else {
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 nodeid <-  13538
 predict <-   16.1166666666666636
 } else {
 nodeid <-  13539
 predict <-   29.2800000000000011
 }
 }
 } else {
 nodeid <-  3385
 predict <-   19.1766666666666659
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   66.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   50.0000000000000000 ){
 nodeid <-  6772
 predict <-   21.7466666666666661
 } else {
 nodeid <-  6773
 predict <-   21.1400000000000006
 }
 } else {
 if(is.na(qdep) | qdep <=   54.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   88.0000000000000000 ){
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   82.0000000000000000 ){
 nodeid <-  54192
 predict <-   17.7033333333333331
 } else {
 nodeid <-  54193
 predict <-   20.4549999999999983
 }
 } else {
 nodeid <-  27097
 predict <-   30.4499999999999993
 }
 } else {
 nodeid <-  13549
 predict <-   27.3350000000000009
 }
 } else {
 nodeid <-  6775
 predict <-   28.7400000000000020
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.1000000000000000E+02 ){
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 nodeid <-  3388
 predict <-   19.4199999999999982
 } else {
 nodeid <-  3389
 predict <-   33.2150000000000034
 }
 } else {
 if(!is.na(qdep) & qdep <=   50.0000000000000000 ){
 nodeid <-  3390
 predict <-   24.9450000000000003
 } else {
 nodeid <-  3391
 predict <-   28.8100000000000023
 }
 }
 }
 }
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.5800000000000000E+02 ){
 if(!is.na(qdep) & qdep <=   26.0000000000000000 ){
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    2.0600000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    1.3600000000000000E+02 ){
 nodeid <-  1696
 predict <-    7.0100000000000007
 } else {
 if(!is.na(qdep) & qdep <=   10.0000000000000000 ){
 nodeid <-  3394
 predict <-    2.7324999999999999
 } else {
 nodeid <-  3395
 predict <-    6.8224999999999998
 }
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 nodeid <-  1698
 predict <-   -6.2060074040032180
 x <- ifelse(is.na(wrnd),  22.2222222222222214 ,wrnd)
 predict <- predict +    4.6180843569277189E-03  * x
 x <- ifelse(is.na(rrnd),  33.3333333333333357 ,rrnd)
 predict <- predict +   -1.1581233514362990E-03  * x
 x <- ifelse(is.na(rdsz),   2.3111111111111111E+02 ,rdsz)
 predict <- predict +    4.0500167522280008E-02  * x
 x <- ifelse(is.na(qdep),  11.2222222222222214 ,qdep)
 predict <- predict +    0.5848058048705982  * x
 x <- ifelse(is.na(wrsz),   1.4533333333333334E+02 ,wrsz)
 predict <- predict +   -2.2345449516971344E-02  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  1699
 predict <-    9.5125000000000011
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.2800000000000000E+02 ){
 nodeid <-  850
 predict <-    7.0150000000000006
 } else {
 if(is.na(rdsz) | rdsz <=    2.4600000000000000E+02 ){
 if(!is.na(qdep) & qdep <=    2.5000000000000000 ){
 nodeid <-  3404
 predict <-    1.1199999999999999
 } else {
 if(!is.na(qdep) & qdep <=   10.0000000000000000 ){
 nodeid <-  6810
 predict <-    4.1699999999999999
 } else {
 if(!is.na(wrsz) & wrsz <=    1.3400000000000000E+02 ){
 nodeid <-  13622
 predict <-    9.6649999999999991
 } else {
 if(!is.na(qdep) & qdep <=   18.0000000000000000 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 nodeid <-  54492
 predict <-   11.0166666666666657
 } else {
 nodeid <-  54493
 predict <-    9.1400000000000006
 }
 } else {
 nodeid <-  27247
 predict <-   12.7533333333333321
 }
 }
 }
 }
 } else {
 nodeid <-  1703
 predict <-    3.5199999999999996
 }
 }
 }
 } else {
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   36.0000000000000000 ){
 nodeid <-  3408
 predict <-   15.1966666666666672
 } else {
 nodeid <-  3409
 predict <-   19.1499999999999986
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.4600000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    1.3200000000000000E+02 ){
 nodeid <-  6820
 predict <-   23.8850000000000016
 } else {
 if(!is.na(qdep) & qdep <=   44.0000000000000000 ){
 nodeid <-  13642
 predict <-   16.9299999999999997
 } else {
 nodeid <-  13643
 predict <-   27.4033333333333324
 }
 }
 } else {
 nodeid <-  3411
 predict <-   25.0099999999999980
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.6000000000000000E+02 ){
 if(is.na(qdep) | qdep <=   40.0000000000000000 ){
 nodeid <-  3412
 predict <-   15.8800000000000008
 } else {
 nodeid <-  3413
 predict <-   23.3249999999999993
 }
 } else {
 if(is.na(rdsz) | rdsz <=    2.2400000000000000E+02 ){
 nodeid <-  3414
 predict <-   27.8174999999999990
 } else {
 nodeid <-  3415
 predict <-   33.0949999999999989
 }
 }
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  854
 predict <-   29.4399999999999977
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(is.na(qdep) | qdep <=   48.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.4800000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    1.4200000000000000E+02 ){
 nodeid <-  13680
 predict <-   25.1349999999999980
 } else {
 nodeid <-  13681
 predict <-   21.2466666666666661
 }
 } else {
 nodeid <-  6841
 predict <-   26.6166666666666636
 }
 } else {
 nodeid <-  3421
 predict <-   33.0833333333333357
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.3600000000000000E+02 ){
 nodeid <-  3422
 predict <-   31.9250000000000043
 } else {
 nodeid <-  3423
 predict <-   27.3850000000000016
 }
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   30.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   14.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    6.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.7400000000000000E+02 ){
 nodeid <-  1712
 predict <-    1.8950000000000000
 } else {
 if(!is.na(rdsz) & rdsz <=    1.5200000000000000E+02 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  6852
 predict <-    2.3100000000000001
 } else {
 nodeid <-  6853
 predict <-    1.7533333333333332
 }
 } else {
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 nodeid <-  6854
 predict <-    2.7425000000000002
 } else {
 nodeid <-  6855
 predict <-    3.3950000000000000
 }
 }
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=    1.8400000000000000E+02 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  13712
 predict <-    4.5899999999999999
 } else {
 nodeid <-  13713
 predict <-    6.0499999999999998
 }
 } else {
 nodeid <-  6857
 predict <-    6.6000000000000005
 }
 } else {
 nodeid <-  3429
 predict <-    6.9250000000000007
 }
 } else {
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 nodeid <-  3430
 predict <-    5.3266666666666671
 } else {
 nodeid <-  3431
 predict <-    9.0299999999999994
 }
 }
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   75.0000000000000000 ){
 nodeid <-  1716
 predict <-   14.0350000000000001
 } else {
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 nodeid <-  3434
 predict <-   11.6924999999999990
 } else {
 nodeid <-  3435
 predict <-   16.3200000000000038
 }
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(!is.na(rrnd) & rrnd <=   75.0000000000000000 ){
 nodeid <-  3436
 predict <-   19.1566666666666698
 } else {
 nodeid <-  3437
 predict <-   13.5433333333333348
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.3400000000000000E+02 ){
 nodeid <-  3438
 predict <-   12.9649999999999999
 } else {
 if(is.na(rdsz) | rdsz <=    2.0200000000000000E+02 ){
 nodeid <-  6878
 predict <-  -18.3055343622791042
 x <- ifelse(is.na(wrnd),  59.0909090909090935 ,wrnd)
 predict <- predict +   -9.6281960144732250E-03  * x
 x <- ifelse(is.na(rrnd),  86.3636363636363598 ,rrnd)
 predict <- predict +   -9.0587367959633659E-03  * x
 x <- ifelse(is.na(rdsz),   1.7854545454545453E+02 ,rdsz)
 predict <- predict +    8.7044220409036328E-03  * x
 x <- ifelse(is.na(qdep),  22.5454545454545467 ,qdep)
 predict <- predict +    0.7403211038781241  * x
 x <- ifelse(is.na(wr),  40.9090909090909065 ,wr)
 predict <- predict +    0.1551843238651918  * x
 x <- ifelse(is.na(wrsz),   1.8109090909090909E+02 ,wrsz)
 predict <- predict +    5.6760512530721424E-02  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  6879
 predict <-   19.4299999999999997
 }
 }
 }
 }
 }
 } else {
 if(is.na(rdsz) | rdsz <=    2.2200000000000000E+02 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(!is.na(wr) & wr <=   37.5000000000000000 ){
 if(!is.na(qdep) & qdep <=   42.0000000000000000 ){
 nodeid <-  3440
 predict <-   19.3833333333333364
 } else {
 nodeid <-  3441
 predict <-   28.7400000000000020
 }
 } else {
 if(is.na(rdsz) | rdsz <=    1.7800000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=    1.4400000000000000E+02 ){
 nodeid <-  6884
 predict <-   35.1666666666666643
 } else {
 nodeid <-  6885
 predict <-   28.0433333333333330
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  6886
 predict <-   28.2349999999999994
 } else {
 nodeid <-  6887
 predict <-   39.1633333333333269
 }
 }
 }
 } else {
 if(is.na(rdsz) | rdsz <=    2.0200000000000000E+02 ){
 if(is.na(rdsz) | rdsz <=    1.6000000000000000E+02 ){
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 nodeid <-  6888
 predict <-   18.7549999999999990
 } else {
 if(!is.na(wrsz) & wrsz <=    1.8400000000000000E+02 ){
 if(!is.na(qdep) & qdep <=   48.0000000000000000 ){
 nodeid <-  27556
 predict <-   24.7233333333333327
 } else {
 nodeid <-  27557
 predict <-   39.1400000000000006
 }
 } else {
 if(is.na(rdsz) | rdsz <=    1.4200000000000000E+02 ){
 nodeid <-  27558
 predict <-   19.3333333333333321
 } else {
 nodeid <-  27559
 predict <-   28.7700000000000031
 }
 }
 }
 } else {
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 if(is.na(qdep) | qdep <=   56.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.9600000000000000E+02 ){
 nodeid <-  27560
 predict <-   26.7566666666666642
 } else {
 nodeid <-  27561
 predict <-   25.3149999999999977
 }
 } else {
 nodeid <-  13781
 predict <-   33.2733333333333334
 }
 } else {
 nodeid <-  6891
 predict <-   33.8500000000000014
 }
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  3446
 predict <-   34.0499999999999972
 } else {
 if(is.na(qdep) | qdep <=   42.0000000000000000 ){
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 nodeid <-  13788
 predict <-   21.1566666666666663
 } else {
 nodeid <-  13789
 predict <-   28.9500000000000028
 }
 } else {
 nodeid <-  6895
 predict <-   37.2633333333333283
 }
 }
 }
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  862
 predict <-   31.2375000000000007
 } else {
 if(!is.na(rrnd) & rrnd <=   75.0000000000000000 ){
 nodeid <-  1726
 predict <-   36.3999999999999986
 } else {
 if(!is.na(rdsz) & rdsz <=    2.3200000000000000E+02 ){
 nodeid <-  3454
 predict <-   40.1850000000000023
 } else {
 nodeid <-  3455
 predict <-   26.6200000000000010
 }
 }
 }
 }
 }
 }
 }
 } else {
 if(is.na(wr) | wr <=   37.5000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.0200000000000000E+02 ){
 if(is.na(qdep) | qdep <=   38.0000000000000000 ){
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(is.na(qdep) | qdep <=   22.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   34.0000000000000000 ){
 nodeid <-  1728
 predict <-    6.3300000000000001
 } else {
 if(!is.na(wrsz) & wrsz <=    2.1800000000000000E+02 ){
 nodeid <-  3458
 predict <-    5.8250000000000002
 } else {
 if(!is.na(rrnd) & rrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   44.0000000000000000 ){
 nodeid <-  13836
 predict <-    7.9100000000000001
 } else {
 nodeid <-  13837
 predict <-    5.5549999999999997
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   74.0000000000000000 ){
 nodeid <-  13838
 predict <-    7.6766666666666667
 } else {
 nodeid <-  13839
 predict <-    3.5700000000000003
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    2.3400000000000000E+02 ){
 if(!is.na(qdep) & qdep <=   26.0000000000000000 ){
 nodeid <-  6920
 predict <-   10.8699999999999992
 } else {
 nodeid <-  6921
 predict <-   12.0049999999999990
 }
 } else {
 nodeid <-  3461
 predict <-   13.2100000000000009
 }
 } else {
 nodeid <-  1731
 predict <-   16.2866666666666653
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   74.0000000000000000 ){
 nodeid <-  866
 predict <-   -5.9618286822134596
 x <- ifelse(is.na(rrnd),  50.0000000000000000 ,rrnd)
 predict <- predict +    2.8256986788781826E-03  * x
 x <- ifelse(is.na(rdsz),  38.6666666666666643 ,rdsz)
 predict <- predict +    1.5437049591992050E-02  * x
 x <- ifelse(is.na(qdep),  17.4444444444444429 ,qdep)
 predict <- predict +    0.4312088722752379  * x
 x <- ifelse(is.na(wrsz),   2.3244444444444446E+02 ,wrsz)
 predict <- predict +    2.2788244516826182E-02  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  867
 predict <-   11.0900000000000016
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   44.0000000000000000 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 nodeid <-  868
 predict <-   18.1500000000000021
 } else {
 nodeid <-  869
 predict <-   16.4750000000000014
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    2.5000000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    2.1600000000000000E+02 ){
 nodeid <-  1740
 predict <-   21.8866666666666667
 } else {
 if(!is.na(rdsz) & rdsz <=   18.0000000000000000 ){
 nodeid <-  3482
 predict <-   20.6766666666666659
 } else {
 nodeid <-  3483
 predict <-   24.8550000000000004
 }
 }
 } else {
 nodeid <-  871
 predict <-   26.8933333333333344
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.7400000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    2.3400000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    2.3000000000000000E+02 ){
 nodeid <-  872
 predict <-  -18.4302764537142565
 x <- ifelse(is.na(wrnd),  62.5000000000000000 ,wrnd)
 predict <- predict +   -4.4379938306414856E-03  * x
 x <- ifelse(is.na(rrnd),  53.1250000000000000 ,rrnd)
 predict <- predict +   -1.4235753300820182E-03  * x
 x <- ifelse(is.na(rdsz),   1.3850000000000000E+02 ,rdsz)
 predict <- predict +    3.8302803382082577E-02  * x
 x <- ifelse(is.na(qdep),  32.2500000000000000 ,qdep)
 predict <- predict +    0.5508405384310131  * x
 x <- ifelse(is.na(wrsz),   2.2025000000000000E+02 ,wrsz)
 predict <- predict +    6.0802986945386370E-02  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  873
 predict <-   16.4349999999999987
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(is.na(qdep) | qdep <=   40.0000000000000000 ){
 nodeid <-  1748
 predict <-    7.5649999999999995
 } else {
 nodeid <-  1749
 predict <-   32.8999999999999986
 }
 } else {
 if(!is.na(qdep) & qdep <=   50.0000000000000000 ){
 nodeid <-  1750
 predict <-   23.4199999999999982
 } else {
 nodeid <-  1751
 predict <-   32.2700000000000031
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   20.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    6.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    2.5000000000000000 ){
 nodeid <-  1752
 predict <-    1.2224999999999999
 } else {
 nodeid <-  1753
 predict <-    2.9600000000000000
 }
 } else {
 if(!is.na(qdep) & qdep <=   14.0000000000000000 ){
 nodeid <-  1754
 predict <-    5.9775000000000000
 } else {
 nodeid <-  1755
 predict <-   10.0449999999999999
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    2.1400000000000000E+02 ){
 nodeid <-  878
 predict <-   28.8833333333333364
 } else {
 if(is.na(rdsz) | rdsz <=    2.3400000000000000E+02 ){
 nodeid <-  1758
 predict <-  -26.6045400200745803
 x <- ifelse(is.na(wrnd),  34.6153846153846132 ,wrnd)
 predict <- predict +    5.4011590232475194E-03  * x
 x <- ifelse(is.na(rrnd),  30.7692307692307701 ,rrnd)
 predict <- predict +    8.0927264965026738E-04  * x
 x <- ifelse(is.na(rdsz),   2.0369230769230768E+02 ,rdsz)
 predict <- predict +    5.8752396881914783E-02  * x
 x <- ifelse(is.na(qdep),  36.9230769230769198 ,qdep)
 predict <- predict +    0.6679843501085506  * x
 x <- ifelse(is.na(wrsz),   2.3200000000000000E+02 ,wrsz)
 predict <- predict +    6.1659799285554789E-02  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  1759
 predict <-   23.3900000000000006
 }
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   26.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.0200000000000000E+02 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   10.0000000000000000 ){
 nodeid <-  880
 predict <-    3.3075000000000001
 } else {
 nodeid <-  881
 predict <-   13.8966666666666665
 }
 } else {
 if(!is.na(qdep) & qdep <=   22.0000000000000000 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   12.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    2.3800000000000000E+02 ){
 nodeid <-  7056
 predict <-    5.1600000000000001
 } else {
 nodeid <-  7057
 predict <-    6.5999999999999996
 }
 } else {
 nodeid <-  3529
 predict <-   14.0300000000000011
 }
 } else {
 if(!is.na(qdep) & qdep <=   10.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    6.0000000000000000 ){
 nodeid <-  7060
 predict <-    2.7200000000000002
 } else {
 nodeid <-  7061
 predict <-    5.9100000000000001
 }
 } else {
 nodeid <-  3531
 predict <-   10.1849999999999987
 }
 }
 } else {
 nodeid <-  883
 predict <-   19.6133333333333333
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   14.0000000000000000 ){
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 nodeid <-  884
 predict <-   -7.6255624611050585
 x <- ifelse(is.na(wrnd),  22.7272727272727266 ,wrnd)
 predict <- predict +    2.6979026874048573E-03  * x
 x <- ifelse(is.na(rrnd),  54.5454545454545467 ,rrnd)
 predict <- predict +   -1.6160183869962296E-03  * x
 x <- ifelse(is.na(rdsz),   2.0000000000000000E+02 ,rdsz)
 predict <- predict +    5.9904325172000627E-03  * x
 x <- ifelse(is.na(qdep),   9.4545454545454550 ,qdep)
 predict <- predict +    0.8463205428952615  * x
 x <- ifelse(is.na(wrsz),   2.2327272727272728E+02 ,wrsz)
 predict <- predict +    3.1567603596724281E-02  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  885
 predict <-    1.9725000000000001
 }
 } else {
 if(!is.na(qdep) & qdep <=   22.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   18.0000000000000000 ){
 nodeid <-  1772
 predict <-   14.3075000000000010
 } else {
 nodeid <-  1773
 predict <-   18.3399999999999999
 }
 } else {
 if(is.na(wrnd) | wrnd <=   25.0000000000000000 ){
 nodeid <-  1774
 predict <-   23.3900000000000006
 } else {
 nodeid <-  1775
 predict <-   21.9699999999999989
 }
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.2800000000000000E+02 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    6.0000000000000000 ){
 nodeid <-  888
 predict <-   23.9399999999999977
 } else {
 if(!is.na(wrsz) & wrsz <=    2.4600000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=    1.1000000000000000E+02 ){
 nodeid <-  3556
 predict <-  -33.6847318354359544
 x <- ifelse(is.na(wrnd),  38.8888888888888857 ,wrnd)
 predict <- predict +   -4.0878536192494102E-03  * x
 x <- ifelse(is.na(rrnd),  16.6666666666666679 ,rrnd)
 predict <- predict +    4.0172374419684791E-04  * x
 x <- ifelse(is.na(rdsz),  52.4444444444444429 ,rdsz)
 predict <- predict +    3.6704664815070086E-02  * x
 x <- ifelse(is.na(qdep),  47.5555555555555571 ,qdep)
 predict <- predict +    0.7749711310517748  * x
 x <- ifelse(is.na(wrsz),   2.2844444444444446E+02 ,wrsz)
 predict <- predict +    0.1376171356231317  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  3557
 predict <-   22.8350000000000009
 }
 } else {
 nodeid <-  1779
 predict <-   44.0450000000000017
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    2.4200000000000000E+02 ){
 nodeid <-  890
 predict <-  -33.8517418273484765
 x <- ifelse(is.na(wrnd),  37.5000000000000000 ,wrnd)
 predict <- predict +   -1.2808574345136688E-03  * x
 x <- ifelse(is.na(rdsz),  82.0000000000000000 ,rdsz)
 predict <- predict +    3.6009296627582309E-02  * x
 x <- ifelse(is.na(qdep),  45.6666666666666643 ,qdep)
 predict <- predict +    0.7968801280569150  * x
 x <- ifelse(is.na(wrsz),   2.2900000000000000E+02 ,wrsz)
 predict <- predict +    0.1331891665636763  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  891
 predict <-   37.6150000000000020
 }
 }
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    2.1600000000000000E+02 ){
 nodeid <-  892
 predict <-   28.5166666666666657
 } else {
 if(!is.na(qdep) & qdep <=   30.0000000000000000 ){
 nodeid <-  1786
 predict <-   25.6099999999999994
 } else {
 if(is.na(qdep) | qdep <=   46.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   38.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  14296
 predict <-   33.0000000000000000
 } else {
 nodeid <-  14297
 predict <-   32.1133333333333368
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.7600000000000000E+02 ){
 nodeid <-  14298
 predict <-   39.3700000000000045
 } else {
 nodeid <-  14299
 predict <-   37.3774999999999977
 }
 }
 } else {
 nodeid <-  3575
 predict <-   45.3133333333333326
 }
 }
 }
 } else {
 nodeid <-  447
 predict <-  -31.9033649020438794
 x <- ifelse(is.na(wrnd),  59.0909090909090935 ,wrnd)
 predict <- predict +   -3.2233684530785598E-03  * x
 x <- ifelse(is.na(rdsz),   1.9345454545454547E+02 ,rdsz)
 predict <- predict +    3.5288901173212914E-02  * x
 x <- ifelse(is.na(qdep),  41.4545454545454533 ,qdep)
 predict <- predict +    0.8959434448820568  * x
 x <- ifelse(is.na(wrsz),   2.3054545454545453E+02 ,wrsz)
 predict <- predict +    0.1100738213147353  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 }
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   30.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.9000000000000000E+02 ){
 if(!is.na(qdep) & qdep <=   18.0000000000000000 ){
 if(!is.na(wr) & wr <=   87.5000000000000000 ){
 if(!is.na(qdep) & qdep <=    2.5000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   46.0000000000000000 ){
 nodeid <-  896
 predict <-    0.8250000000000000
 } else {
 nodeid <-  897
 predict <-    0.9625000000000000
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   75.0000000000000000 ){
 nodeid <-  898
 predict <-    1.0449999999999999
 } else {
 if(!is.na(wrsz) & wrsz <=    1.6800000000000000E+02 ){
 nodeid <-  1798
 predict <-    1.0100000000000000
 } else {
 nodeid <-  1799
 predict <-    1.1233333333333333
 }
 }
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   14.0000000000000000 ){
 nodeid <-  1800
 predict <-    8.4600000000000009
 } else {
 nodeid <-  1801
 predict <-   13.3150000000000013
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.7000000000000000E+02 ){
 nodeid <-  1802
 predict <-    7.9533333333333331
 } else {
 nodeid <-  1803
 predict <-   11.0800000000000001
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    1.1600000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=   46.0000000000000000 ){
 nodeid <-  1804
 predict <-    7.1924999999999999
 } else {
 if(!is.na(qdep) & qdep <=   14.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   90.0000000000000000 ){
 nodeid <-  7220
 predict <-    9.8300000000000001
 } else {
 nodeid <-  7221
 predict <-    7.2866666666666662
 }
 } else {
 nodeid <-  3611
 predict <-   14.1466666666666665
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   10.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    6.0000000000000000 ){
 nodeid <-  3612
 predict <-    3.8750000000000000
 } else {
 nodeid <-  3613
 predict <-    7.2166666666666659
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.6400000000000000E+02 ){
 nodeid <-  3614
 predict <-   12.5233333333333334
 } else {
 nodeid <-  3615
 predict <-   10.9699999999999989
 }
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=    6.0000000000000000 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    2.5000000000000000 ){
 nodeid <-  904
 predict <-    0.9666666666666667
 } else {
 nodeid <-  905
 predict <-    4.0475000000000003
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=    1.5800000000000000E+02 ){
 if(!is.na(qdep) & qdep <=    2.5000000000000000 ){
 nodeid <-  3624
 predict <-    0.9800000000000000
 } else {
 nodeid <-  3625
 predict <-    3.5850000000000000
 }
 } else {
 nodeid <-  1813
 predict <-    4.4500000000000002
 }
 } else {
 nodeid <-  907
 predict <-    1.5800000000000001
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   10.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.7200000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    1.6200000000000000E+02 ){
 nodeid <-  1816
 predict <-    7.6250000000000000
 } else {
 nodeid <-  1817
 predict <-    8.1099999999999994
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   75.0000000000000000 ){
 nodeid <-  1818
 predict <-    8.9299999999999997
 } else {
 nodeid <-  1819
 predict <-    9.0099999999999998
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   14.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.6200000000000000E+02 ){
 nodeid <-  1820
 predict <-   10.8566666666666674
 } else {
 if(!is.na(wrsz) & wrsz <=    1.7600000000000000E+02 ){
 nodeid <-  3642
 predict <-   12.3533333333333335
 } else {
 nodeid <-  3643
 predict <-   13.5600000000000005
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   44.0000000000000000 ){
 nodeid <-  1822
 predict <-   13.8149999999999995
 } else {
 nodeid <-  1823
 predict <-   14.6066666666666674
 }
 }
 }
 }
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.3400000000000000E+02 ){
 nodeid <-  228
 predict <-   14.9166666666666661
 } else {
 if(!is.na(rdsz) & rdsz <=   22.0000000000000000 ){
 nodeid <-  458
 predict <-   19.9600000000000009
 } else {
 if(!is.na(rdsz) & rdsz <=   48.0000000000000000 ){
 nodeid <-  918
 predict <-   23.1400000000000006
 } else {
 if(is.na(rdsz) | rdsz <=    2.0000000000000000E+02 ){
 if(is.na(rdsz) | rdsz <=    1.0800000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    1.5400000000000000E+02 ){
 nodeid <-  7352
 predict <-   15.8999999999999986
 } else {
 nodeid <-  7353
 predict <-   24.5150000000000006
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  7354
 predict <-   23.7749999999999986
 } else {
 nodeid <-  7355
 predict <-   26.9466666666666690
 }
 }
 } else {
 nodeid <-  1839
 predict <-   24.0800000000000018
 }
 }
 }
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   75.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=    2.2600000000000000E+02 ){
 if(!is.na(wr) & wr <=   87.5000000000000000 ){
 if(is.na(rdsz) | rdsz <=    1.9200000000000000E+02 ){
 if(is.na(rdsz) | rdsz <=    1.3600000000000000E+02 ){
 nodeid <-  3680
 predict <-   17.0375000000000014
 } else {
 nodeid <-  3681
 predict <-   18.2033333333333331
 }
 } else {
 nodeid <-  1841
 predict <-   21.4033333333333324
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.5400000000000000E+02 ){
 nodeid <-  1842
 predict <-   20.8733333333333313
 } else {
 nodeid <-  1843
 predict <-   27.3024999999999984
 }
 }
 } else {
 nodeid <-  461
 predict <-   24.3900000000000006
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.5000000000000000E+02 ){
 if(is.na(rdsz) | rdsz <=    1.2600000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    1.4400000000000000E+02 ){
 nodeid <-  1848
 predict <-   16.0899999999999999
 } else {
 nodeid <-  1849
 predict <-   24.2674999999999983
 }
 } else {
 nodeid <-  925
 predict <-   15.9425000000000008
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.8200000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    1.7800000000000000E+02 ){
 nodeid <-  1852
 predict <-  -28.1481143113839671
 x <- ifelse(is.na(wrnd),  66.6666666666666714 ,wrnd)
 predict <- predict +   -5.1300829352562347E-03  * x
 x <- ifelse(is.na(rdsz),   1.2088888888888889E+02 ,rdsz)
 predict <- predict +    5.8783047430039760E-03  * x
 x <- ifelse(is.na(qdep),  21.3333333333333321 ,qdep)
 predict <- predict +    0.7713327246941237  * x
 x <- ifelse(is.na(wr),  91.6666666666666714 ,wr)
 predict <- predict +    0.1323437179426078  * x
 x <- ifelse(is.na(wrsz),   1.6533333333333334E+02 ,wrsz)
 predict <- predict +    0.1179946455371626  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  1853
 predict <-   28.8299999999999983
 }
 } else {
 nodeid <-  927
 predict <-   28.1600000000000001
 }
 }
 }
 }
 }
 } else {
 if(!is.na(wr) & wr <=   87.5000000000000000 ){
 if(!is.na(qdep) & qdep <=   10.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    2.2600000000000000E+02 ){
 nodeid <-  232
 predict <-   -5.7959064660792183
 x <- ifelse(is.na(wrnd),  55.5555555555555571 ,wrnd)
 predict <- predict +    6.7225727253664809E-05  * x
 x <- ifelse(is.na(rrnd),  52.7777777777777786 ,rrnd)
 predict <- predict +   -4.9987787314253519E-04  * x
 x <- ifelse(is.na(rdsz),   1.2711111111111111E+02 ,rdsz)
 predict <- predict +    3.2410595311470466E-03  * x
 x <- ifelse(is.na(qdep),   5.6666666666666670 ,qdep)
 predict <- predict +    1.0142185176694567  * x
 x <- ifelse(is.na(wrsz),   2.1044444444444446E+02 ,wrsz)
 predict <- predict +    2.7145760726533432E-02  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  233
 predict <-   -2.3451514289331037
 x <- ifelse(is.na(wrnd),  57.8947368421052602 ,wrnd)
 predict <- predict +    1.4832082445905584E-04  * x
 x <- ifelse(is.na(rrnd),  36.8421052631578974 ,rrnd)
 predict <- predict +    3.8300155002401510E-04  * x
 x <- ifelse(is.na(rdsz),   1.3536842105263159E+02 ,rdsz)
 predict <- predict +    8.5880103106861650E-04  * x
 x <- ifelse(is.na(qdep),   3.8421052631578947 ,qdep)
 predict <- predict +    1.1404542773399142  * x
 x <- ifelse(is.na(wrsz),   2.3936842105263159E+02 ,wrsz)
 predict <- predict +    1.0500137348303651E-02  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 } else {
 if(!is.na(qdep) & qdep <=   22.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   14.0000000000000000 ){
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    2.3400000000000000E+02 ){
 nodeid <-  1872
 predict <-   12.7300000000000004
 } else {
 nodeid <-  1873
 predict <-   14.3866666666666649
 }
 } else {
 nodeid <-  937
 predict <-   13.9133333333333340
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 nodeid <-  938
 predict <-  -18.3823547427040985
 x <- ifelse(is.na(wrnd),  15.3846153846153850 ,wrnd)
 predict <- predict +    2.8579863058041978E-03  * x
 x <- ifelse(is.na(rrnd),  50.0000000000000000 ,rrnd)
 predict <- predict +    4.8018588381123842E-03  * x
 x <- ifelse(is.na(rdsz),   1.1261538461538461E+02 ,rdsz)
 predict <- predict +    9.7911690686783431E-03  * x
 x <- ifelse(is.na(qdep),  18.4615384615384599 ,qdep)
 predict <- predict +    1.0696806459668258  * x
 x <- ifelse(is.na(wrsz),   2.1692307692307693E+02 ,wrsz)
 predict <- predict +    7.8234106241360291E-02  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  939
 predict <-   17.1850000000000023
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    2.1600000000000000E+02 ){
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  1880
 predict <-   24.8133333333333326
 } else {
 if(is.na(wrnd) | wrnd <=   25.0000000000000000 ){
 nodeid <-  3762
 predict <-   27.0699999999999967
 } else {
 nodeid <-  3763
 predict <-   28.2800000000000011
 }
 }
 } else {
 nodeid <-  941
 predict <-   28.6625000000000014
 }
 } else {
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  1884
 predict <-   27.1649999999999991
 } else {
 nodeid <-  1885
 predict <-  -27.9703899491888457
 x <- ifelse(is.na(wrnd),  66.6666666666666714 ,wrnd)
 predict <- predict +   -4.4127459932001142E-03  * x
 x <- ifelse(is.na(rrnd),  27.7777777777777786 ,rrnd)
 predict <- predict +    4.8440342588328099E-03  * x
 x <- ifelse(is.na(rdsz),  98.7777777777777715 ,rdsz)
 predict <- predict +    1.1419284684362315E-02  * x
 x <- ifelse(is.na(qdep),  26.2222222222222214 ,qdep)
 predict <- predict +    1.1436147458502828  * x
 x <- ifelse(is.na(wrsz),   2.3466666666666666E+02 ,wrsz)
 predict <- predict +    0.1146994907522948  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 } else {
 nodeid <-  943
 predict <-   26.8599999999999994
 }
 }
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    2.2200000000000000E+02 ){
 if(!is.na(qdep) & qdep <=   18.0000000000000000 ){
 if(!is.na(qdep) & qdep <=    6.0000000000000000 ){
 nodeid <-  472
 predict <-   -1.8794052318468761
 x <- ifelse(is.na(wrnd),  65.0000000000000000 ,wrnd)
 predict <- predict +   -1.1199756316429236E-03  * x
 x <- ifelse(is.na(rrnd),  85.0000000000000000 ,rrnd)
 predict <- predict +   -3.4953630263549285E-04  * x
 x <- ifelse(is.na(rdsz),   1.2880000000000001E+02 ,rdsz)
 predict <- predict +    3.4334115035901879E-04  * x
 x <- ifelse(is.na(qdep),   2.2000000000000002 ,qdep)
 predict <- predict +    1.2141600079013044  * x
 x <- ifelse(is.na(wrsz),   2.0840000000000001E+02 ,wrsz)
 predict <- predict +    9.5947210944269234E-03  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 if(!is.na(wrsz) & wrsz <=    1.9600000000000000E+02 ){
 nodeid <-  946
 predict <-   17.3200000000000003
 } else {
 if(!is.na(qdep) & qdep <=   10.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    2.1000000000000000E+02 ){
 nodeid <-  3788
 predict <-    9.9399999999999995
 } else {
 if(is.na(rdsz) | rdsz <=    1.1000000000000000E+02 ){
 nodeid <-  7578
 predict <-   10.4799999999999986
 } else {
 nodeid <-  7579
 predict <-   10.3650000000000002
 }
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   75.0000000000000000 ){
 nodeid <-  3790
 predict <-   16.1250000000000000
 } else {
 nodeid <-  3791
 predict <-   17.1649999999999991
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   22.0000000000000000 ){
 nodeid <-  474
 predict <-   25.5925000000000011
 } else {
 if(!is.na(wrsz) & wrsz <=    2.1000000000000000E+02 ){
 if(!is.na(qdep) & qdep <=   26.0000000000000000 ){
 nodeid <-  1900
 predict <-   29.6099999999999994
 } else {
 nodeid <-  1901
 predict <-   33.3324999999999960
 }
 } else {
 if(!is.na(qdep) & qdep <=   26.0000000000000000 ){
 nodeid <-  1902
 predict <-   31.7266666666666701
 } else {
 nodeid <-  1903
 predict <-   36.2150000000000034
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   10.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    2.3000000000000000E+02 ){
 if(is.na(rrnd) | rrnd <=   25.0000000000000000 ){
 nodeid <-  952
 predict <-    7.4100000000000001
 } else {
 nodeid <-  953
 predict <-    1.3849999999999998
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    2.5000000000000000E+02 ){
 if(!is.na(qdep) & qdep <=    6.0000000000000000 ){
 nodeid <-  1908
 predict <-    4.3766666666666660
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  3818
 predict <-   11.8949999999999996
 } else {
 nodeid <-  3819
 predict <-   11.7233333333333345
 }
 }
 } else {
 nodeid <-  955
 predict <-    5.3825000000000003
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   18.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    2.4200000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    2.3800000000000000E+02 ){
 nodeid <-  1912
 predict <-  -15.2307053633565541
 x <- ifelse(is.na(wrnd),  44.4444444444444429 ,wrnd)
 predict <- predict +   -4.2706566484630639E-04  * x
 x <- ifelse(is.na(rrnd),  33.3333333333333357 ,rrnd)
 predict <- predict +   -1.5031080810105663E-03  * x
 x <- ifelse(is.na(rdsz),   1.0411111111111111E+02 ,rdsz)
 predict <- predict +   -1.9444673798934978E-04  * x
 x <- ifelse(is.na(qdep),  15.1111111111111107 ,qdep)
 predict <- predict +    1.4078557484754477  * x
 x <- ifelse(is.na(wrsz),   2.3066666666666666E+02 ,wrsz)
 predict <- predict +    6.5771638613336125E-02  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  1913
 predict <-   23.5749999999999993
 }
 } else {
 nodeid <-  957
 predict <-   21.6466666666666647
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    2.2600000000000000E+02 ){
 nodeid <-  958
 predict <-   33.2699999999999960
 } else {
 if(!is.na(qdep) & qdep <=   26.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   22.0000000000000000 ){
 nodeid <-  3836
 predict <-   29.2666666666666693
 } else {
 nodeid <-  3837
 predict <-   35.5750000000000028
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    2.4600000000000000E+02 ){
 nodeid <-  3838
 predict <-   40.5549999999999997
 } else {
 nodeid <-  3839
 predict <-   42.7100000000000009
 }
 }
 }
 }
 }
 }
 }
 }
 } else {
 if(is.na(qdep) | qdep <=   50.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=    1.3800000000000000E+02 ){
 if(!is.na(wr) & wr <=   87.5000000000000000 ){
 if(is.na(qdep) | qdep <=   42.0000000000000000 ){
 if(is.na(qdep) | qdep <=   38.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 nodeid <-  960
 predict <-    0.8371672535916410
 x <- ifelse(is.na(wrnd),  25.0000000000000000 ,wrnd)
 predict <- predict +    1.2571592427219891E-04  * x
 x <- ifelse(is.na(rrnd),  50.0000000000000000 ,rrnd)
 predict <- predict +   -1.4796672717158031E-03  * x
 x <- ifelse(is.na(rdsz),  62.7999999999999972 ,rdsz)
 predict <- predict +    1.8497558455695021E-02  * x
 x <- ifelse(is.na(wrsz),   2.0800000000000000E+02 ,wrsz)
 predict <- predict +    0.1436058968311043  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  961
 predict <-    0.8609207261569871
 x <- ifelse(is.na(wrnd),  80.7692307692307736 ,wrnd)
 predict <- predict +   -3.6629182369654067E-03  * x
 x <- ifelse(is.na(rrnd),  46.1538461538461533 ,rrnd)
 predict <- predict +   -8.0390097404274552E-04  * x
 x <- ifelse(is.na(rdsz),  73.2307692307692264 ,rdsz)
 predict <- predict +    1.7592610407051758E-02  * x
 x <- ifelse(is.na(wrsz),   1.8984615384615384E+02 ,wrsz)
 predict <- predict +    0.1645860089893376  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 } else {
 if(is.na(wrnd) | wrnd <=   25.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=   80.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.7400000000000000E+02 ){
 nodeid <-  3848
 predict <-   29.4750000000000014
 } else {
 if(!is.na(wrsz) & wrsz <=    1.9000000000000000E+02 ){
 nodeid <-  7698
 predict <-   35.1600000000000037
 } else {
 nodeid <-  7699
 predict <-   40.1799999999999997
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   94.0000000000000000 ){
 nodeid <-  3850
 predict <-   45.2400000000000020
 } else {
 nodeid <-  3851
 predict <-   44.0133333333333283
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.8800000000000000E+02 ){
 nodeid <-  1926
 predict <-   29.9233333333333320
 } else {
 nodeid <-  1927
 predict <-   42.5099999999999980
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   46.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    2.0400000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    1.6800000000000000E+02 ){
 nodeid <-  1928
 predict <-   31.5233333333333299
 } else {
 nodeid <-  1929
 predict <-   39.1799999999999997
 }
 } else {
 nodeid <-  965
 predict <-   48.5549999999999997
 }
 } else {
 if(!is.na(rdsz) & rdsz <=   64.0000000000000000 ){
 nodeid <-  966
 predict <-   38.1124999999999972
 } else {
 if(is.na(wrnd) | wrnd <=   25.0000000000000000 ){
 nodeid <-  1934
 predict <-   50.9449999999999932
 } else {
 nodeid <-  1935
 predict <-   45.1649999999999991
 }
 }
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.8600000000000000E+02 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 nodeid <-  968
 predict <-   32.8999999999999986
 } else {
 if(!is.na(qdep) & qdep <=   38.0000000000000000 ){
 nodeid <-  1938
 predict <-   32.9399999999999977
 } else {
 if(!is.na(wrsz) & wrsz <=    1.4600000000000000E+02 ){
 nodeid <-  3878
 predict <-   34.6433333333333380
 } else {
 if(!is.na(rdsz) & rdsz <=   46.0000000000000000 ){
 nodeid <-  7758
 predict <-   41.7999999999999972
 } else {
 nodeid <-  7759
 predict <-   42.8899999999999935
 }
 }
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.8200000000000000E+02 ){
 if(!is.na(wrnd) & wrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   70.0000000000000000 ){
 nodeid <-  3880
 predict <-   34.2650000000000006
 } else {
 if(is.na(rdsz) | rdsz <=    1.1400000000000000E+02 ){
 nodeid <-  7762
 predict <-   40.5633333333333326
 } else {
 nodeid <-  7763
 predict <-   34.7650000000000006
 }
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  3882
 predict <-   43.1800000000000068
 } else {
 if(!is.na(wrsz) & wrsz <=    1.5200000000000000E+02 ){
 nodeid <-  7766
 predict <-   30.4800000000000004
 } else {
 nodeid <-  7767
 predict <-   38.2199999999999989
 }
 }
 }
 } else {
 nodeid <-  971
 predict <-   46.5450000000000017
 }
 }
 } else {
 if(is.na(qdep) | qdep <=   38.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    6.0000000000000000 ){
 nodeid <-  972
 predict <-   43.2849999999999966
 } else {
 if(!is.na(wrsz) & wrsz <=    1.9400000000000000E+02 ){
 nodeid <-  1946
 predict <-   41.9966666666666626
 } else {
 if(!is.na(wrsz) & wrsz <=    2.0400000000000000E+02 ){
 nodeid <-  3894
 predict <-   41.6833333333333371
 } else {
 if(is.na(wrnd) | wrnd <=   25.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 nodeid <-  15580
 predict <-   47.1600000000000037
 } else {
 if(!is.na(wrsz) & wrsz <=    2.3400000000000000E+02 ){
 nodeid <-  31162
 predict <-   50.1974999999999980
 } else {
 nodeid <-  31163
 predict <-   53.5399999999999991
 }
 }
 } else {
 nodeid <-  7791
 predict <-   48.3633333333333368
 }
 }
 }
 }
 } else {
 if(!is.na(qdep) & qdep <=   46.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  1948
 predict <-   57.7824999999999989
 } else {
 if(!is.na(wrnd) & wrnd <=   75.0000000000000000 ){
 nodeid <-  3898
 predict <-   51.4650000000000034
 } else {
 if(!is.na(wrsz) & wrsz <=    2.0800000000000000E+02 ){
 nodeid <-  7798
 predict <-   49.6850000000000023
 } else {
 nodeid <-  7799
 predict <-   54.5399999999999991
 }
 }
 }
 } else {
 nodeid <-  975
 predict <-   68.0199999999999960
 }
 }
 }
 }
 } else {
 if(!is.na(wr) & wr <=   87.5000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    2.0200000000000000E+02 ){
 if(is.na(qdep) | qdep <=   42.0000000000000000 ){
 if(is.na(qdep) | qdep <=   38.0000000000000000 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.6000000000000000E+02 ){
 nodeid <-  7808
 predict <-   25.0700000000000003
 } else {
 nodeid <-  7809
 predict <-   28.8249999999999993
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.6000000000000000E+02 ){
 nodeid <-  7810
 predict <-   28.6566666666666663
 } else {
 nodeid <-  7811
 predict <-   33.0874999999999986
 }
 }
 } else {
 nodeid <-  1953
 predict <-   24.5966666666666676
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.8000000000000000E+02 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  3908
 predict <-   30.7200000000000024
 } else {
 nodeid <-  3909
 predict <-   32.0574999999999974
 }
 } else {
 nodeid <-  1955
 predict <-   40.8466666666666711
 }
 }
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   46.0000000000000000 ){
 nodeid <-  1956
 predict <-   35.7100000000000009
 } else {
 nodeid <-  1957
 predict <-   40.1899999999999977
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    2.2600000000000000E+02 ){
 nodeid <-  1958
 predict <-   43.8149999999999977
 } else {
 nodeid <-  1959
 predict <-   49.8750000000000000
 }
 }
 }
 } else {
 if(is.na(qdep) | qdep <=   46.0000000000000000 ){
 if(is.na(qdep) | qdep <=   42.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    2.3000000000000000E+02 ){
 nodeid <-  1960
 predict <-   39.9549999999999983
 } else {
 if(is.na(qdep) | qdep <=   38.0000000000000000 ){
 nodeid <-  3922
 predict <-   44.6766666666666694
 } else {
 nodeid <-  3923
 predict <-   47.3250000000000028
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    2.3400000000000000E+02 ){
 nodeid <-  1962
 predict <-   49.2475000000000023
 } else {
 nodeid <-  1963
 predict <-   55.0750000000000028
 }
 }
 } else {
 nodeid <-  491
 predict <-   51.7266666666666666
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.9400000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    1.4600000000000000E+02 ){
 if(!is.na(qdep) & qdep <=   38.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   34.0000000000000000 ){
 nodeid <-  1968
 predict <-   26.8266666666666644
 } else {
 nodeid <-  1969
 predict <-   31.4200000000000017
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    2.1000000000000000E+02 ){
 nodeid <-  1970
 predict <-   35.2566666666666677
 } else {
 nodeid <-  1971
 predict <-   40.1774999999999949
 }
 }
 } else {
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    2.0400000000000000E+02 ){
 nodeid <-  1972
 predict <-   33.8833333333333329
 } else {
 nodeid <-  1973
 predict <-   41.3400000000000034
 }
 } else {
 if(is.na(qdep) | qdep <=   38.0000000000000000 ){
 nodeid <-  1974
 predict <-   36.0375000000000014
 } else {
 nodeid <-  1975
 predict <-   50.7466666666666626
 }
 }
 }
 } else {
 if(!is.na(rdsz) & rdsz <=    2.0000000000000000E+02 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  988
 predict <-   58.3250000000000028
 } else {
 nodeid <-  989
 predict <-  -59.7457085828908347
 x <- ifelse(is.na(wrnd),  66.6666666666666714 ,wrnd)
 predict <- predict +    4.0471929070550975E-03  * x
 x <- ifelse(is.na(rrnd),  83.3333333333333286 ,rrnd)
 predict <- predict +    2.6450259747524137E-02  * x
 x <- ifelse(is.na(rdsz),   1.7600000000000000E+02 ,rdsz)
 predict <- predict +    2.2749744571570453E-02  * x
 x <- ifelse(is.na(qdep),  40.4444444444444429 ,qdep)
 predict <- predict +    1.1956440717238792  * x
 x <- ifelse(is.na(wrsz),   2.2044444444444446E+02 ,wrsz)
 predict <- predict +    0.2674522828539727  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    2.4600000000000000E+02 ){
 if(!is.na(rdsz) & rdsz <=    2.5000000000000000E+02 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  3960
 predict <-   54.6599999999999966
 } else {
 if(!is.na(rdsz) & rdsz <=    2.3000000000000000E+02 ){
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 nodeid <-  15844
 predict <-   51.3433333333333337
 } else {
 nodeid <-  15845
 predict <-   43.4899999999999949
 }
 } else {
 nodeid <-  7923
 predict <-   54.7866666666666617
 }
 }
 } else {
 nodeid <-  1981
 predict <-   55.9449999999999932
 }
 } else {
 nodeid <-  991
 predict <-   69.2300000000000040
 }
 }
 }
 }
 }
 } else {
 if(!is.na(wr) & wr <=   87.5000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.8600000000000000E+02 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(is.na(rdsz) | rdsz <=    1.2800000000000000E+02 ){
 if(!is.na(qdep) & qdep <=   58.0000000000000000 ){
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  1984
 predict <-   36.5799999999999983
 } else {
 nodeid <-  1985
 predict <-   38.8666666666666671
 }
 } else {
 nodeid <-  993
 predict <-   43.4600000000000009
 }
 } else {
 nodeid <-  497
 predict <-   49.5166666666666586
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 nodeid <-  498
 predict <-   45.5075000000000003
 } else {
 if(!is.na(qdep) & qdep <=   54.0000000000000000 ){
 nodeid <-  998
 predict <-   38.5666666666666700
 } else {
 if(!is.na(rdsz) & rdsz <=   52.0000000000000000 ){
 nodeid <-  1998
 predict <-   39.6833333333333300
 } else {
 nodeid <-  1999
 predict <-  -45.6520891283189272
 x <- ifelse(is.na(wrnd),  66.6666666666666714 ,wrnd)
 predict <- predict +    1.3454755632386323E-02  * x
 x <- ifelse(is.na(rrnd),  77.7777777777777715 ,rrnd)
 predict <- predict +    3.9532241063799378E-03  * x
 x <- ifelse(is.na(rdsz),   1.5688888888888889E+02 ,rdsz)
 predict <- predict +    2.9287553718466055E-02  * x
 x <- ifelse(is.na(qdep),  57.7777777777777786 ,qdep)
 predict <- predict +    0.7528588968809864  * x
 x <- ifelse(is.na(wrsz),   1.6088888888888889E+02 ,wrsz)
 predict <- predict +    0.2742044472875591  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 }
 }
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   58.0000000000000000 ){
 nodeid <-  1000
 predict <-   65.4474999999999909
 } else {
 nodeid <-  1001
 predict <-   63.7950000000000017
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    2.2200000000000000E+02 ){
 nodeid <-  1002
 predict <-   52.9025000000000034
 } else {
 nodeid <-  1003
 predict <-   65.8166666666666629
 }
 }
 } else {
 if(is.na(rdsz) | rdsz <=    2.4400000000000000E+02 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=    1.3000000000000000E+02 ){
 nodeid <-  2008
 predict <-   53.5249999999999986
 } else {
 if(is.na(wrnd) | wrnd <=   75.0000000000000000 ){
 nodeid <-  4018
 predict <-   65.4700000000000131
 } else {
 nodeid <-  4019
 predict <-   69.2899999999999920
 }
 }
 } else {
 nodeid <-  1005
 predict <-  -60.5040624393797728
 x <- ifelse(is.na(wrnd),  83.3333333333333286 ,wrnd)
 predict <- predict +    3.4775774542204846E-02  * x
 x <- ifelse(is.na(rrnd),  55.5555555555555571 ,rrnd)
 predict <- predict +    3.1372176262574186E-02  * x
 x <- ifelse(is.na(rdsz),   1.2711111111111111E+02 ,rdsz)
 predict <- predict +    1.9304320192735258E-02  * x
 x <- ifelse(is.na(qdep),  56.8888888888888857 ,qdep)
 predict <- predict +    1.0290226131109785  * x
 x <- ifelse(is.na(wrsz),   2.1555555555555554E+02 ,wrsz)
 predict <- predict +    0.2574149086098800  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 }
 } else {
 nodeid <-  503
 predict <-   64.2000000000000028
 }
 }
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   25.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    1.9800000000000000E+02 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 nodeid <-  504
 predict <-  -56.0279270533743698
 x <- ifelse(is.na(rrnd),  22.2222222222222214 ,rrnd)
 predict <- predict +    1.3631887637227807E-02  * x
 x <- ifelse(is.na(rdsz),   1.2544444444444444E+02 ,rdsz)
 predict <- predict +   -2.6569215792896228E-03  * x
 x <- ifelse(is.na(qdep),  56.4444444444444429 ,qdep)
 predict <- predict +    1.0752557606124116  * x
 x <- ifelse(is.na(wrsz),   1.6222222222222223E+02 ,wrsz)
 predict <- predict +    0.3189073315238607  * x
 predict <- min(max(predict,   7.0000000000000007E-02 ),  85.9200000000000017 )
 } else {
 nodeid <-  505
 predict <-   62.6350000000000051
 }
 } else {
 if(!is.na(qdep) & qdep <=   58.0000000000000000 ){
 if(!is.na(wrsz) & wrsz <=    2.1800000000000000E+02 ){
 if(is.na(rdsz) | rdsz <=    1.4600000000000000E+02 ){
 nodeid <-  2024
 predict <-   70.2433333333333252
 } else {
 nodeid <-  2025
 predict <-   65.6050000000000040
 }
 } else {
 nodeid <-  1013
 predict <-   81.7700000000000102
 }
 } else {
 nodeid <-  507
 predict <-   84.7750000000000057
 }
 }
 } else {
 if(!is.na(wrnd) & wrnd <=   75.0000000000000000 ){
 if(!is.na(qdep) & qdep <=   58.0000000000000000 ){
 if(is.na(rrnd) | rrnd <=   75.0000000000000000 ){
 if(!is.na(rdsz) & rdsz <=   12.0000000000000000 ){
 nodeid <-  2032
 predict <-   50.9549999999999983
 } else {
 if(!is.na(wrsz) & wrsz <=    1.9000000000000000E+02 ){
 nodeid <-  4066
 predict <-   53.0649999999999977
 } else {
 if(!is.na(wrsz) & wrsz <=    2.3400000000000000E+02 ){
 nodeid <-  8134
 predict <-   69.8750000000000000
 } else {
 nodeid <-  8135
 predict <-   83.2750000000000057
 }
 }
 }
 } else {
 nodeid <-  1017
 predict <-   75.7299999999999898
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.8200000000000000E+02 ){
 nodeid <-  1018
 predict <-   60.9033333333333360
 } else {
 nodeid <-  1019
 predict <-   75.2000000000000028
 }
 }
 } else {
 if(!is.na(wrsz) & wrsz <=    1.3600000000000000E+02 ){
 nodeid <-  510
 predict <-   42.3550000000000040
 } else {
 if(!is.na(rdsz) & rdsz <=   36.0000000000000000 ){
 nodeid <-  1022
 predict <-   59.5900000000000034
 } else {
 if(!is.na(rdsz) & rdsz <=   82.0000000000000000 ){
 nodeid <-  2046
 predict <-   61.9766666666666666
 } else {
 if(!is.na(wrsz) & wrsz <=    2.1400000000000000E+02 ){
 if(!is.na(wrsz) & wrsz <=    1.7000000000000000E+02 ){
 if(!is.na(rrnd) & rrnd <=   25.0000000000000000 ){
 nodeid <-  16376
 predict <-   49.2450000000000045
 } else {
 nodeid <-  16377
 predict <-   51.6875000000000000
 }
 } else {
 if(!is.na(qdep) & qdep <=   58.0000000000000000 ){
 nodeid <-  16378
 predict <-   65.6949999999999932
 } else {
 nodeid <-  16379
 predict <-   69.8433333333333337
 }
 }
 } else {
 nodeid <-  4095
 predict <-   73.0600000000000023
 }
 }
 }
 }
 }
 }
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
 wrnd <- as.numeric(newdata$wrnd[i])
 rrnd <- as.numeric(newdata$rrnd[i])
 rdsz <- as.numeric(newdata$rdsz[i])
 qdep <- as.numeric(newdata$qdep[i])
 wr <- as.numeric(newdata$wr[i])
 wrsz <- as.numeric(newdata$wrsz[i])
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
