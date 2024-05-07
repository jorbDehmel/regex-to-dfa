DIAGRAMS := base.png plus.png question.png star.png suit.png \
	example.png

.PHONY:	all
all:	$(DIAGRAMS)

%.png:	%.dot
	dot -Tpng $^ -o $@
