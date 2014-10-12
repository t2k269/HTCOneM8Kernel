#!/bin/sh

$TOP/kitchen/aik/build.sh
cp $TOP/kitchen/aik/image-new.img $TOP/kitchen/package/boot.img

echo "Building flashable zip..."
rm -f $TOP/kitchen/*.zip

SUFFIX=`grep CONFIG_LOCALVERSION= $TOP/source/arch/arm/configs/cm_m8_defconfig | awk -F '"' '{print $2}'`

cd $TOP/kitchen/package
zip -r -9 -q $TOP/kitchen/kernel${SUFFIX}.zip * 

echo "Done."
