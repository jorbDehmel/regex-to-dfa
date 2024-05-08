DIAGRAMS := base.png plus.png question.png star.png suit.png \
	example.png classes.png email.png example.eps.png \
	example.subexpr.png

.PHONY:	all
all:	$(DIAGRAMS)

%.png:	%.dot
	dot -Tpng $^ -o $@

%.png:	%.plantuml
	plantuml $^ -o .

.PHONY:	clean
clean:
	rm *.png
