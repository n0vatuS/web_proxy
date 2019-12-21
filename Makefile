all: web_proxy

web_proxy: main.o utils.o
	g++ -std=c++11 -o web_proxy main.o utils.o -pthread

main.o: main.cpp
	g++ -std=c++11 -c -o main.o main.cpp -pthread

utils.o: utils.cpp
	g++ -std=c++11 -c -o utils.o utils.cpp -pthread

clean:
	rm -f *.o web_proxy
