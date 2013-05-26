#!/bin/sh
modname="sba"
module="SBA"
device="SBA"
mode="664"
SRCDIR="."

# Group: since distributions do it differently, look for wheel or use staff
if grep '^staff:' /etc/group > /dev/null; then
    group="staff"
else
    group="wheel"
fi

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in . by default
cd $SRCDIR;
/sbin/insmod ./$module.ko $* || exit 1

major=`cat /proc/devices | awk "\\$2==\"$modname\" {print \\$1}"`

# Remove stale nodes and replace them, then give gid and perms
rm -f /dev/${device}0
mknod /dev/${device}0 b $major 0
ln -sf ${device}0 /dev/${device}
chgrp $group /dev/${device}0
chmod $mode  /dev/${device}0

#rm -f /dev/${device}1
#mknod /dev/${device}1 b $major 1
#chgrp $group /dev/${device}1
#chmod $mode  /dev/${device}1
