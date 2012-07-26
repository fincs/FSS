all:
	@$(MAKE) -C arm7
	@$(MAKE) -f mainlib.mk

clean:
	@$(MAKE) -C arm7 clean
	@$(MAKE) -f mainlib.mk clean

install:
	@$(MAKE) -C arm7 install
	@$(MAKE) -f mainlib.mk install
