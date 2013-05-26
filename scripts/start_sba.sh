#!/bin/csh

if ($# != 5) then
	echo "Usage: start_sba.sh <ext3|reiser> <data|metadata|wback> <commit(sec)> <jsize(MB)> <sepdev(y|n)>"
	exit 0
endif

set device 		= "/dev/SBA"
set mount_pt	= "/mnt/sba"
set jmode		= $2
set jcommit		= $3
set jsize		= $4
set sepdev		= $5

switch($1)
	case "ext3":
		switch($sepdev)
			case "y":
				echo "Not yet supported"
			breaksw

			case "n":
				echo "----------------------------------------------------------"
				echo "Creating the journal and filesystem ..."
				mke2fs -j -J size=$jsize -b 4096 $device

				echo "----------------------------------------------------------"
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
			breaksw
		endsw
		exit
	breaksw

	case "reiser":
		@ jsize = $jsize * 256;
		echo "Number of journal blocks = " $jsize

		echo "Creating the file system ..."
		mkreiserfs -ff --format 3.6 -b 4096 -s $jsize $device
		
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

	case "jfs":
		echo "Creating the filesystem ..."
		mkfs.jfs -f -s $jsize $device
		mount $device $mount_pt -t jfs
		exit
	breaksw
endsw
