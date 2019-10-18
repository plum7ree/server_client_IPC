import matplotlib.pyplot as plt


for i in [1,3,5] :
	xaxis = [x for x in range(1,10)]
	names = []
	values = []
	with open("../output/stress_testing_sync_" + str(i) + "_segment.csv", "r") as f:
	    for line in f:
	        cst, sms_size = line.split(",")
	        names.append(sms_size.strip());
	        values.append(float(cst));
	print(names)
	print(values)
	plt.figure()
	plt.bar(xaxis, values, align="center")
	plt.xticks(xaxis, names)


	plt.savefig("../output/stress_testing_sync_" + str(i) + "_segment.png")
	f.close()


# dic = { 1.0 : "some custom text"}
# labels = [[], []]


# data = [[], []]
# with open("../output/stress_testing_sync_1_segment.csv", "r") as f:
#     for line in f:
#         cst, sms_size = line.split(",")
#         data[0] += [int(sms_size)]
#         data[1] += [float(cst)]
#         labels[0] += [int(sms_size)]
#         labels[1] += [sms_size]


# plt.bar(labels[0])
# plt.plot(*data)
# plt.xlabel('cst')
# plt.ylabel('segment_size')
# plt.xticks(*labels)
# plt.grid(True)
# # plt.show()

# plt.savefig('../output/stress_testing_sync_1_segment.png')
