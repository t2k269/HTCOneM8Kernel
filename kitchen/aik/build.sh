#!/bin/sh

cd $TOP/kitchen/aik

./cleanup.sh

./unpackimg.sh elementalx-boot.img

bin/dtbToolCM -o split_img/elementalx-boot.img-dtb -s 2048 -d "htc,project-id = <" -p $TOP/source/scripts/dtc/ $TOP/source/arch/arm/boot/

cp -f $TOP/source/arch/arm/boot/zImage split_img/elementalx-boot.img-zImage 

./repackimg.sh
