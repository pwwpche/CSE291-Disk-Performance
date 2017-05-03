"""
    ../src/fitness --device /dev/sdb1 \
        --wr 25 --cap 100 --qdep 8 --wrnd 50 --rrnd 0 --wrsz 128 --rdsz 64 \
        --warm 5 --test 5 --direct
"""

# Texlive commands:
"""


"""


import subprocess, os
from random import *
from math import *

wr_arr = [0, 25, 50, 75, 100]
qdep_arr = [1] + [x * 4 + 4 for x in range(15)]
rdsz_arr = [1] + [x * 4 + 4 for x in range(63)]
wrsz_arr = [1] + [x * 4 + 4 for x in range(63)]
rrnd_arr = [0, 50, 100]
wrnd_arr = [0, 50, 100]
rd_stride_arr = [1, 64, 128, 256]
wr_stride_arr = [1, 64, 128, 256]


result_file = open("result.txt", "w", 0)
res = []
first = True
device_name = raw_input("Device name?")

samples = 10000


# Generate workload data
for i in range(samples):
	# Clear caches
	clear_cache = """echo "123456" | sudo -S sh -c 'echo 3 >/proc/sys/vm/drop_caches' """	
 	subprocess.check_output(clear_cache, shell=True)

	wr = choice(wr_arr)
	qdep = choice(qdep_arr)
	rdsz = choice(rdsz_arr)
	wrsz = choice(wrsz_arr)
	rrnd = choice(rrnd_arr)
	wrnd = choice(wrnd_arr)
	rd_stride = choice(rd_stride_arr)
	wr_stride = choice(wr_stride_arr)

	input_str = """
		echo %s | sudo -S ../src/fitness --device /dev/%s --seed %s \
		--wr %s --qdep %s --rdsz %s --wrsz %s --rrnd %s --wrnd %s \
		--rd_stride %s --wr_stride %s \
		--warm 0 --test 10 --direct  
	""" % ("123456", device_name, str(randint(0, 1000000000)), \
		str(wr), str(qdep), str(rdsz), str(wrsz), str(rrnd), str(wrnd), \
		str(rd_stride), str(wr_stride) \
	)			
	result = subprocess.check_output(input_str, shell=True)

	print "Testing complete."

	titles, numbers = result.split("\n")[0].split(), result.split("\n")[1].split()
	data = dict(zip(titles, numbers))
	data['wght'] = 0 if randint(0, 1000) < 100 else 1
	data['rdstrd'] = rd_stride
	data['wrstrd'] = wr_stride
	if first:									# Write titles
		first = False
		for k in data:
			result_file.write(k + "\t")
		result_file.write("\r\n")

	for k in data:
		result_file.write(str(data[k]) + "\t")
	result_file.write("\r\n")
	print "Writing complete."

result_file.close()

# Generate latex pdf 
subprocess.check_output("cp result.txt ../../guide/disk.dat", shell=True)
os.chdir("../../guide/")
subprocess.check_output("./guide < disk.in", shell=True)
subprocess.check_output("latex disk.tex", shell=True)
subprocess.check_output("dvips disk.dvi", shell=True)
subprocess.check_output("ps2pdf disk.ps", shell=True)



# Parse and calculate MSE
first = False
test_set, train_set = [], []
with open("disk_fit.txt") as f:
	# Load file
	content = f.readlines()
	for line in content:
		data = line.split()
		if first or (len(data) < 4):
			first = True
			continue
		if data[0] == 'n':
			test_set.append((float(data[2]), float(data[3])))
		else:
			train_set.append((float(data[2]), float(data[3])))			
	# Calculation
	MAE = sum( [abs(x[0] - x[1]) for x in test_set] ) / len(test_set)
	MRE = sum([abs(x[0] - x[1]) / x[0] for x in test_set]) / len(test_set)
	SSE = sum([abs(x[0] - x[1]) ** 2 for x in test_set])
	avg = sum([x[0] for x in test_set]) / len(test_set)	
	SST = sum([(x[0] - avg) ** 2 for x in test_set])
	
	R = 1 - SSE / SST
	print "MSE: %s\n MRE: %s\n R: %s\n" % (str(MAE), str(MRE), str(R))

