all:
	g++ -std=c++17 -I. -O3 -o file_search file_search.cpp

clean:
	rm -f file_search
