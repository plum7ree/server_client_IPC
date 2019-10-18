#!/bin/bash

# client_simple() {
# 	sleep 1.5
# 	./client --conf ../input/simple.yaml
# }

# ./server --n_sms 1 --sms_size 32 & 
# 			client_simple

# python ./graph_simple.py

client_stress(){
	sleep 1.5
	./client --conf ../input/stress.yaml
}

for n_seg in 1, 3, 5
# for n_seg in 1, 3
do 
	for segsz in 32, 64, 128, 256, 512, 1024, 2048, 4096, 8912
	# for segsz in 512, 1024, 2048, 4096, 8912
		do
			./server --n_sms $n_seg --sms_size $segsz & 
			client_stress
			
		done
done		

# python ./graph_stress.py
