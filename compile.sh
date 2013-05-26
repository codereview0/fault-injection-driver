#!/bin/csh

rm -f Makefile

ln -s Makefile.ext3 Makefile
#ln -s Makefile.reiserfs Makefile
#ln -s Makefile.jfs Makefile

make -C /root/vijayan/repository/2.6.9/linux-2.6.9/ M=$PWD modules
