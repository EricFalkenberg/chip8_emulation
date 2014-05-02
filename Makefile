all: chip8.c
	gcc -std=c99 -o emu8 chip8.c -framework OpenGL -framework GLUT
