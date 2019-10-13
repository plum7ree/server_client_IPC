import matplotlib.pyplot as plt

dic = { 1.0 : "some custom text"}
labels = [[], []]


data = [[], []]
with open("stats.csv", "r") as f:
    for line in f:
        cst, sms_size = line.split(",")
        data[0] += [int(sms_size)]
        data[1] += [float(cst)]
        labels[0] += [int(sms_size)]
        labels[1] += [sms_size]

plt.plot(*data)
plt.xlabel('cst')
plt.ylabel('segment_size')
plt.xticks(*labels)
plt.grid(True)
plt.show()
