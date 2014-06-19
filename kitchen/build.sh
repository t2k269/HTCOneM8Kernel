#!/bin/sh

$TOP/kitchen/aik/build.sh
cp $TOP/kitchen/aik/image-new.img $TOP/kitchen/package/boot/boot.img

cd $TOP/kitchen/package/system/lib/modules
rm -f *.ko

cd $TOP/kitchen/package
find $TOP/source -name "*.ko" -exec cp -f {} $TOP/kitchen/package/system/lib/modules/ \;

echo "Building flashable zip..."
rm -f ../*.zip
zip -r -9 -q ../kernel-m8-gpe-4.4.3-s2w-pattern.zip * 

echo "Done."
