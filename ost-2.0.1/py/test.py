"""
    ../src/fitness --device /dev/sdb1 \
        --wr 25 --cap 100 --qdep 8 --wrnd 50 --rrnd 0 --wrsz 128 --rdsz 64 \
        --warm 5 --test 5 --direct
"""

# Texlive commands:
"""


"""


import subprocess, os
from random import randint


result_file = open("result.txt", "w", 0)
res = []
first = True
device_name = "sdb"
samples = 15


# Generate workload data
for i in range(samples):
	input_str = """
		echo %s | sudo -S ../src/fitness --device /dev/%s \
		--cap 512 --seed %s \
		--warm 0 --test 5 --direct  
	""" % ("123456", device_name, str(randint(0, 10000)))					# Execute testing command
	result = subprocess.check_output(input_str, shell=True)
	print "Testing complete."

	titles, numbers = result.split("\n")[0].split(), result.split("\n")[1].split()
	if first:									# Write titles
		first = False
		result_file.write(result.split("\n")[0] + "    wght" + "\r\n")

	if i < (samples / 10):
		result_file.write(result.split("\n")[1] + "    0" + "\r\n")				# Write test set
	else:
		result_file.write(result.split("\n")[1] + "    1" + "\r\n")				# Write training set
	result_detail = dict(zip(titles, numbers))
	res.append(result_detail)
	print "Writing complete."
result_file.close()

subprocess.check_output("cp result.txt ../../guide/disk.dat", shell=True)
os.chdir("../../guide/")
subprocess.check_output("./guide < disk.in", shell=True)
subprocess.check_output("latex disk.tex", shell=True)
subprocess.check_output("dvips disk.dvi", shell=True)
subprocess.check_output("ps2pdf disk.ps", shell=True)

