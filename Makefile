CC := g++ -std=c++20
FLAGS := -O3 -g
HEADERS := lexer.hpp tokex.hpp expression.hpp regex.hpp \
	regex_manager.hpp

.PHONY:	all
all:	Makefile format tests.out regex_main.out

.PHONY:	run
run:	tests.out regex_main.out
	./tests.out
	./regex_main.out

.PHONY:	format
format:
	clang-format -i *.cpp *.hpp

tests.out:	tokex_unit_tests.o lexer.o
	$(CC) $(FLAGS) -o $@ $^

%.out:	%.o
	$(CC) $(FLAGS) -o $@ $^

%.o:	%.cpp $(HEADERS)
	$(CC) $(FLAGS) -c -o $@ $<

test:	tests.out
	./tests.out

clean:
	rm -f *.out *.o *.aux *.log *.toc *.pdf
