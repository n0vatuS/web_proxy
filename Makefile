all: web_proxy utils

web_proxy: main.o utils.o
	g++ -o web_proxy main.o utils.o -pthread

main.o: main.cpp
	g++ -c -o main.o main.cpp -pthread

utils.o: utils.cpp
	g++ -c -o utils.o utils.cpp -pthread

clean:
	rm -f *.o web_proxy
