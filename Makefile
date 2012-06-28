all:
	@make --no-print-directory -C arm7
	@make --no-print-directory -f mainlib.mk

clean:
	@make --no-print-directory -C arm7 clean
	@make --no-print-directory -f mainlib.mk clean

install:
	@make --no-print-directory -C arm7 install
	@make --no-print-directory -f mainlib.mk install
