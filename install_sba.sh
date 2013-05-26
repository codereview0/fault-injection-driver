#!/bin/csh

if ($# != 5) then
	echo "Usage: install_sba.sh <ext3|reiser|jfs> <data|metadata|wback> <commit> <jsize> <sepdev(y|n)>"
	exit 0
endif

echo "Loading SBA driver ..."
if ($5 == 'y') then
	echo "This option is not yet supported"
	exit;
else
	./load_sba.sh
endif

echo "Building the FS ..."
./scripts/start_sba.sh $1 $2 $3 $4 $5;

./tools/sba start;

#echo "<<=============================================>>"
#echo "Installing the non faulty driver ..."
#cd nf_disk;
#./install_nf_sba.sh $1 $2 $3 $4 $5
