#!/bin/bash

client_simple() {
  sleep 1.5
  ./client --conf ../input/simple.yaml --state SYNC
}

./server --n_sms 1 --sms_size 32 & 
client_simple


client_stress_sync(){
  sleep 1.5
  ./client --conf ../input/stress.yaml --state SYNC
}

for n_seg in 1, 3, 5
  # for n_seg in 1, 3
do 
  for segsz in 32, 64, 128, 256, 512, 1024, 2048, 4096, 8912
    # for segsz in 4096, 8912
  do
    ./server --n_sms $n_seg --sms_size $segsz & 
    client_stress_sync
    wait

  done
done

client_stress_async(){
  sleep 1.5
  ./client --conf ../input/stress.yaml
}

for n_seg in 1, 3, 5
do 
  for segsz in 32, 64, 128, 256, 512, 1024, 2048, 4096, 8912
    # for segsz in 4096, 8912
  do
    ./server --n_sms $n_seg --sms_size $segsz & 
    client_stress_async
    wait

  done
done		

python ./graph_simple.py
python ./graph_stress.py


