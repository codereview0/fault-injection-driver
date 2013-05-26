#!/bin/csh

if ($# != 3) then
	echo "Usage: clean_and_mount.sh <ext3|reiser> <data|metadata|wback> <commit(sec)>"
	exit 0
endif

set device 		= "/dev/SBA"
set mount_pt	= "/mnt/sba"
set jmode		= $2
set jcommit		= $3

echo "Cleaning /mnt/sba/ ..."
rm -rf /mnt/sba/*;

umount /mnt/sba/;

switch($1)
	case "ext3":
		switch($jmode)
		case "data":
			echo "Mounting the filesystem in data journaling mode in $mount_pt ..."
			mount -o commit=$jcommit -o data=journal $device $mount_pt -t ext3
		breaksw

		case "metadata":
			echo "Mounting the filesystem in ordered data mode in $mount_pt ..."
			mount -o commit=$jcommit -o data=ordered $device $mount_pt -t ext3
		breaksw

		case "wback":
			echo "Mounting the filesystem in writeback mode in $mount_pt ..."
			mount -o commit=$jcommit -o data=writeback $device $mount_pt -t ext3
		breaksw
		endsw

		exit
	breaksw

	case "reiser":
		switch($jmode)
		case "data":
			echo "Mounting the filesystem in data journaling mode in $mount_pt ..."
			mount -o commit=$jcommit -o data=journal $device $mount_pt -t reiserfs
		breaksw

		case "metadata":
			echo "Mounting the filesystem in ordered data mode in $mount_pt ..."
			mount -o commit=$jcommit -o data=ordered $device $mount_pt -t reiserfs
		breaksw

		case "wback":
			echo "Mounting the filesystem in writeback mode in $mount_pt ..."
			mount -o commit=$jcommit -o data=writeback $device $mount_pt -t reiserfs
		breaksw
		endsw

		exit
	breaksw
endsw
