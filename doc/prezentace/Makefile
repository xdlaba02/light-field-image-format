# usage: make filename.pdf
%.pdf: %.tex
	pdflatex $<
	pdflatex $<

.PHONY: clean

clean:
	rm -f *.snm *.out *.toc *.aux *.log *.nav
	rm -f *.pdf
	rm -f *~
