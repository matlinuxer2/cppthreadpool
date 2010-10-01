all: threadpool.o
	g++ -o main.exe main.cpp threadpool.o -pthread

threadpool.o:
	g++ -c threadpool.cpp 

clean:
	rm -vf *.exe *.o
