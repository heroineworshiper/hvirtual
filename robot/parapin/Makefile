#
# Parapin Makefile
#
# Jeremy Elson, First released 30 March 2000
#
# $Id: Makefile,v 1.2 2003/11/24 08:52:43 heroine Exp $
#

CC := gcc
CFLAGS := -O2 -Wall

EXAMPLE_DIR := examples
EXAMPLE_PROGRAMS := \
	$(EXAMPLE_DIR)/inputtest \
	$(EXAMPLE_DIR)/outputtest\
	$(EXAMPLE_DIR)/adc0831

LIBPARAPIN_OBJS := parapin.o

###########################################################################


default: libparapin.a kparapin.o $(EXAMPLE_PROGRAMS) 

install: libparapin.a
	/usr/bin/install -o bin -g bin -m 755 libparapin.a /usr/local/lib
	/usr/bin/install -o bin -g bin -m 444 parapin.h /usr/local/include


##### libraries

libparapin.a: $(LIBPARAPIN_OBJS)
	ar -cr libparapin.a $(LIBPARAPIN_OBJS)

parapin.o: parapin.c parapin.h
	$(CC) $(CFLAGS) -c parapin.c

kparapin.o: parapin.c parapin.h
	$(CC) $(CFLAGS) -o kparapin.o -c parapin.c
#	$(CC) $(CFLAGS) -D__KERNEL__ -DMODULE -o kparapin.o -c parapin.c


# Example programs and the test suite

$(EXAMPLE_DIR)/%: $(EXAMPLE_DIR)/%.c libparapin.a parapin.h
	$(CC) $(CFLAGS) -I. -L. $< -o $@ -lparapin


clean:
	rm -f *.[oa] $(EXAMPLE_PROGRAMS)
