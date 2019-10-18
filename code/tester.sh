#!/bin/bash

client(){
	sleep 1
	./client --conf ../input/sample.yaml
}

for n_seg in 1, 3, 5
do 
	for segsz in 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192
		do
			./server --n_sms $n_seg --sms_size $segsz & 
			client
			
		done
done		

