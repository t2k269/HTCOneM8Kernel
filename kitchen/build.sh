#!/bin/sh

$TOP/kitchen/aik/build.sh
cp $TOP/kitchen/aik/image-new.img $TOP/kitchen/package/boot/boot.img

cd $TOP/kitchen/package
rm -f ../elementalx.zip
zip -r -9 ../elementalx.zip * 
