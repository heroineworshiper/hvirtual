

OBJS := \
	de.mo \
	es.mo \
	eu.mo \
	fr.mo \
	it.mo \
	nb.mo \
	pt_BR.mo \
	sl.mo


all: $(OBJS)


install:
	@for i in $(OBJS) ; \
	do \
		BASENAME=`echo $$i | sed 's/\.mo//'` ; \
		DST=cinelerra.mo ; \
		DIR=../bin/locale/$$BASENAME/LC_MESSAGES ; \
		echo mkdir -p $$DIR ; \
		mkdir -p $$DIR ; \
		echo cp $$i $$DIR/$$DST ; \
		cp $$i $$DIR/$$DST ; \
	done

clean:
	rm -f *.mo

$(OBJS):
	msgfmt -c -o $@ $(subst .mo,.po,$@)


de.mo:	     de.po
es.mo:	     es.po
eu.mo:	     eu.po
fr.mo:	     fr.po
it.mo:	     it.po
nb.mo:	     nb.po
pt_BR.mo:    pt_BR.po
sl.mo:	     sl.po


