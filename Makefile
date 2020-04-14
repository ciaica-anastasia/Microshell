all:
	rm Microsha.o main.o
	g++ -std=c++17 -c Microsha.cpp main.cpp
	g++ -std=c++17 Microsha.o main.o -o msh
