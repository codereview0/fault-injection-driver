#!/bin/csh

if ($# != 2) then
	echo "Usage: empty_files_create.sh <dir> <total>"
	exit -1
endif

set pdir = $1
set totalf = $2
set i = 0

while ($i < $totalf)
	touch $pdir/fIlE$i
	@ i = $i + 1
end
