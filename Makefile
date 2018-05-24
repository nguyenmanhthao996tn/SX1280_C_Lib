main: main.cpp Sx1280.o
	g++ -o main main.cpp Sx1280.o -Wall

Sx1280.o: Sx1280.h Sx1280.cpp
	g++ -c Sx1280.cpp -Wall

clean: 
	rm main; rm *.o