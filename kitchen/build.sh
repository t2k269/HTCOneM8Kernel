#!/bin/sh

$TOP/kitchen/aik/build.sh
cp $TOP/kitchen/aik/image-new.img $TOP/kitchen/package/boot/boot.img

rm -f $TOP/kitchen/package/system/lib/modules/*.ko

find $TOP/source -name "*.ko" -exec cp -f {} $TOP/kitchen/package/system/lib/modules/ \;

echo "Building flashable zip..."
rm -f $TOP/kitchen/*.zip

SUFFIX=`grep CONFIG_LOCALVERSION= $TOP/source/arch/arm/configs/elementalx_defconfig | awk -F '"' '{print $2}'`

cd $TOP/kitchen/package
zip -r -9 -q $TOP/kitchen/kernel${SUFFIX}.zip * 

echo "Done."
