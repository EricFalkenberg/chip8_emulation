all: chip8.c
	gcc -std=c99 -o main chip8.c -framework OpenGL -framework GLUT

