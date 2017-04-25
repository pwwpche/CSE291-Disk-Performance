# CSE291-Disk-Performance
Course project of UCSD CSE-291A Storage Systems

## Basics:

| Item  | Tools |
| ----- |:-----:|
|Regression| GUIDE |
|Performance test| iSCSI toolkit |


## Paper
The project mainly focus on implementation of this paper:

[Performance Modeling and Analysis of Flash-based Storage Devices](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=5937213)

# Implementation 

The implementation consists of two steps:

* Use disk workload generator ([Intel iSCSI Open Storage Toolkit](https://sourceforge.net/projects/intel-iscsi/?source=typ_redirect)) to generate random workload data on a specific flash drive, SSD, etc.

* Generate a regression tree ([GUIDE](http://pages.stat.wisc.edu/~loh/treeprogs/guide/)) to generate a regression tree on the training data generated in the first step. 

* Run tests on this model, and measure the accuracy of this model. (TBD)

# Details:

First, download this two software mentioned above. Notice `iSCSI` software only works in Linux kernel, so either get a virtual machine for it, or install a second system. Typically, a Ubuntu-16.04-amd64 works fine. 

After that, compile the iSCSI toolkit with following command:
```
./configure
make
sudo make install
```

Then if everything works fine, there will be `fitness`, `udisk`, `utest` under `/src`. 

(Not sure) Also modify `/etc/ips.conf`, and uncomment the line with ip address set to 127.0.0.1

Next, install iscsi drivers. (Not sure this step is required). 

Basically, `lsscsi`, `libsgutils`, `sg3utils` and the development libraries are installed. No idea whether iSCSI works fine without them. 

Try [this](https://www.ibm.com/developerworks/library/l-scsi-api/) example first. If this simple SCSI programs works fine, then normally iSCSI toolkit works. 

Next, use `mount` command to check the device mounted to the system. Normally a flash drive mount at `/dev/sdb1` is ok. 

Then use the `fitness` program under `/src`. Commands of how to use this program is in `README` file. 
