
all:
	clang -ggdb main.c glad/glad.c -I. -lglfw -Wall -Wextra
