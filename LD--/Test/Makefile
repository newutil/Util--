#
# Makefile
#
# 2010.04.09 v0.0.3 install のオプションから -g wheel を削除(Ubuntu対応)
#

all: ../ld-- a.cmm b.cmm c.cmm
	c-- a.cmm > a.s
	c-- b.cmm > b.s
	c-- c.cmm > c.s
	as-- a.s
	as-- b.s
	as-- c.s
	../ld-- x.o a.o b.o
	../ld-- a.out x.o c.o

clean:
	rm -f *~ *.o *.lst ld-- a.s b.s c.s a.out
