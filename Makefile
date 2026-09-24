feira_sort: src/main.cpp src/regras.h src/arte.h src/som.h
	g++ -std=c++17 -O2 src/main.cpp -o feira_sort -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

clean:
	rm -f feira_sort
