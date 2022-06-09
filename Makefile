all:
	g++ -c -std=c++17 -o shm.o shm.cpp -lrt -O2 -g -fno-omit-frame-pointer
	g++ -std=c++17 -o write write.cpp shm.o -lrt -O2 -g -fno-omit-frame-pointer
	g++ -std=c++17 -o read read.cpp shm.o -lrt -O2 -g -fno-omit-frame-pointer
