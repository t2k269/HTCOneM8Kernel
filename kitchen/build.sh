#!/bin/sh

$TOP/kitchen/aik/build.sh
cp $TOP/kitchen/aik/image-new.img $TOP/kitchen/package/boot/boot.img

cd $TOP/kitchen/package
rm -f ../*.zip
zip -r -9 ../kernel-m8-gpe-4.4.3-s2w-pattern.zip * 
