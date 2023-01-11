# script to run server, slave

# make c++
make

# run server
./server input2 output & \

# run slaves
./slave part1 & ./slave part2 & ./slave part3 & ./slave part4 & ./slave part5 &

wait

# show parts
ls -lh | grep '[(part)|(input)]*'