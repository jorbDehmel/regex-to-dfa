CC := clang++ -std=c++20
FLAGS := -pedantic -Wall -O3 -g
HEADERS := lexer.hpp tokex.hpp expression.hpp regex.hpp

.PHONY:	all
all:	Makefile format tests.out regex_main.out tokex2.pdf

.PHONY:	run
run:	tests.out
	./tests.out ; $(MAKE) -C test_dots

.PHONY:	format
format:
	clang-format -i *.cpp *.hpp

tests.out:	tokex_unit_tests.o lexer.o
	$(CC) $(FLAGS) -o $@ $^

%.out:	%.o
	$(CC) $(FLAGS) -o $@ $^

%.o:	%.cpp $(HEADERS)
	$(CC) $(FLAGS) -c -o $@ $<

%.pdf:	%.tex
	$(MAKE) -C diagrams
	pdflatex $^ && pdflatex $^

test:	tests.out
	./tests.out

clean:
	rm -f *.out *.o *.aux *.log *.toc *.pdf
