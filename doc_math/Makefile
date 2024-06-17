FTEX=   comp5_doc
$(FTEX).pdf : $(FTEX).tex clear
	pdflatex $(FTEX)
	pdflatex $(FTEX)
	pdflatex $(FTEX)

clear:
	rm -f $(FTEX).pdf *.aux *.log *.nav *.out *.snm *.toc *.soc
