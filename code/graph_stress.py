import matplotlib.pyplot as plt

n_segsize = 9
# n_segsize = 2
n_filetype = 5

# example
# 0.08	2048
# 0.09	2048
# 0.06	2048
# 0.11	2048
# 0.07	2048
# 0.05	4096
# 0.06	4096
# 0.06	4096
# 0.07	4096
# 0.07	4096
# 0.13	8912
# 0.06	8912
# 0.03	8912
# 0.03	8912
# 0.04	8912

### ############### SYNC  ########################
for fileindex in [1,3,5] :
	xaxis = [x for x in range(1,n_segsize*n_filetype+1)]
	names = [] 
	

	# nameval = ['tiny', 'small', 'medium', 'large', 'huge']
	values = []

	group_name = []
	group_val = [] ## values for each seg size. 5 file type will be one group

	with open("../output/stress_testing_sync_" + str(fileindex) + "_segment.csv", "r") as f:
		linecount = 0
		for line in f:
			cst, sms_size = line.split(",")
			names.append(sms_size.strip());
			values.append(float(cst));
			if linecount % n_filetype == n_filetype-1:
				group_val.append(values)
				values = []
			if linecount % n_filetype == 0:
				group_name.append(sms_size.strip())
			linecount += 1

	print(group_name)
	print(group_val)
	plt.figure()
	plt.title('CST for different shared memory segment')
	plt.xlabel('segment size')
	plt.ylabel('second')

	barwidth = 10
	for idx, values in enumerate(group_val):
		nameval = [barwidth*(idx) + i for i in range(len(group_val[0]))]
		plt.bar(nameval, values, align="center")
	plt.xticks([ barwidth*i for i in range(len(group_name))], group_name)



	plt.savefig("../output/stress_testing_sync_" + str(fileindex) + "_segment.png")
	f.close()


## ###############ASYNC########################

for fileindex in [1,3,5] :
	xaxis = [x for x in range(1,n_segsize*n_filetype+1)]
	names = [] 
	

	# nameval = ['tiny', 'small', 'medium', 'large', 'huge']
	values = []

	group_name = []
	group_val = [] ## values for each seg size. 5 file type will be one group

	with open("../output/stress_testing_async_" + str(fileindex) + "_segment.csv", "r") as f:
		linecount = 0
		for line in f:
			cst, sms_size = line.split(",")
			names.append(sms_size.strip());
			values.append(float(cst));
			if linecount % n_filetype == n_filetype-1:
				group_val.append(values)
				values = []
			if linecount % n_filetype == 0:
				group_name.append(sms_size.strip())
			linecount += 1

	print(group_name)
	print(group_val)
	plt.figure()
	plt.title('CST for different shared memory segment')
	plt.xlabel('segment size')
	plt.ylabel('second')

	barwidth = 10
	for idx, values in enumerate(group_val):
		nameval = [barwidth*(idx) + i for i in range(len(group_val[0]))]
		plt.bar(nameval, values, align="center")
	plt.xticks([ barwidth*i for i in range(len(group_name))], group_name)



	plt.savefig("../output/stress_testing_async_" + str(fileindex) + "_segment.png")
	f.close()

