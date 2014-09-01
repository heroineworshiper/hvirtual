all:
	$(MAKE) -f build/Makefile.cinelerra

install:
	$(MAKE) -f build/Makefile.cinelerra install

clean:
	$(MAKE) -f build/Makefile.cinelerra clean

rebuild:
	$(MAKE) -C guicast clean
	$(MAKE) -C cinelerra clean
	$(MAKE) -C plugins clean
	$(MAKE) -C guicast
	$(MAKE) -C cinelerra
	$(MAKE) -C plugins

rebuild_install:
	$(MAKE) -C cinelerra install
	$(MAKE) -C plugins install
