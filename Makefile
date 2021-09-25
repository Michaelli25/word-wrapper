all : ww

ww : ww.c
	gcc -Wall -Werror -fsanitize=address ww.c -o ww

clean :
	rm -f ww
