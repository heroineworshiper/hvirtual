default: parapin.ps parapin.txt

parapin.ps: parapin.tex
	latex parapin.tex
	latex parapin.tex
	dvips -o parapin.ps parapin.dvi

parapin.txt: parapin.tex
	rm -rf /tmp/parapintemp
	mkdir /tmp/parapintemp
	latex2html -split 0 -ascii_mode -noauto_link -rootdir /tmp/parapintemp parapin
	lynx -dump --nolist /tmp/parapintemp/parapin.html > parapin.txt
	cp /tmp/parapintemp/parapin.html .
	rm -rf /tmp/parapintemp

html:
	latex2html -split 4 -local_icons parapin
