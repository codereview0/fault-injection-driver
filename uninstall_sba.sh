#!/bin/csh

echo "Unmounting the filesystem ..."
./scripts/stop_sba.sh

echo "Unloading the device ..."
./unload_sba.sh

echo "Removing the logfile ..."
rm -f logfile

#echo "<<==========================================>>"
#echo "Uninstalling the non faulty driver ..."
#cd nf_disk;
#./uninstall_nf_sba.sh
