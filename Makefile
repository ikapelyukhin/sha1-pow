run : clean build
clean:
	@rm -f sha1-pow
build:
	@cc sha1-pow.c sha1-fast-x8664.S -o sha1-pow -O3 -Wall -Wextra
