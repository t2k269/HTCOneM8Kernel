#!/bin/bash

cd $TOP/kitchen/aik

./cleanup.sh

./unpackimg.sh boot.img

cd ramdisk
grep sysinit init.rc > /dev/null
if [ $? = 1 ]; then 
	echo "Add support for init.d"
	echo "" >> init.rc
	echo "service sysinit /system/bin/sysinit" >> init.rc
	echo "    oneshot" >> init.rc 
fi 

cd $TOP/kitchen/aik

bin/dtbToolCM -o split_img/boot.img-dtb -s 2048 -d "htc,project-id = <" -p $TOP/source/scripts/dtc/ $TOP/source/arch/arm/boot/

cp -f $TOP/source/arch/arm/boot/zImage split_img/boot.img-zImage 

./repackimg.sh
