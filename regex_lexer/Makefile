CPP := g++ -std=c++20 -O3 -g
DEPS := regex_lexer.hpp Makefile
TARGETS := regex_lexer_tests.out

.PHONY:	all
all:	format $(TARGETS)

%.out:	%.o $(DEPS)
	$(CPP) -o $@ $<
	cp $@ $@.debug
	strip --strip-all $@

%.o:	%.cpp $(DEPS)
	$(CPP) -c -o $@ $<

.PHONY:	clean
clean:
	rm -f *.out *.o

.PHONY:	format
format:
	find . -type f \( -iname "*.cpp" -or -iname "*.hpp" \) \
		-exec clang-format -i "{}" \;
