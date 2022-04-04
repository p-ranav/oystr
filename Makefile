all:
	g++ -std=c++17 -I. -O3 -o fs file_search.cpp

clean:
	rm -f fs
