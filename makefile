mine:
	g++ -c main.cpp
	g++ main.o -o main -lsfml-graphics -lsfml-window -lsfml-system
	./main
clean:
	rm -rf main
