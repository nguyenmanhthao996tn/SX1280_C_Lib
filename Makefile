main: main.cpp Sx1280Hal.o Sx1280.o
	g++ -o main main.cpp Sx1280Hal.o Sx1280.o -Wall

Sx1280Hal.o: Sx1280Hal.h Sx1280Hal.cpp
	g++ -c Sx1280Hal.cpp -Wall

Sx1280.o: Sx1280.h Sx1280.cpp Radio.h
	g++ -c Sx1280.cpp -Wall

clean: 
	rm main; rm *.o