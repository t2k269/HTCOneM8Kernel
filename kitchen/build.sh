#!/bin/bash

$TOP/kitchen/aik/build.sh
cp $TOP/kitchen/aik/image-new.img $TOP/kitchen/package/boot.img

echo "Building flashable zip..."
rm -f $TOP/kitchen/*.zip

cd $TOP/source
HASH=`git rev-parse HEAD`
HASH=${HASH:0:7}
SUFFIX=`grep CONFIG_LOCALVERSION= $TOP/source/arch/arm/configs/elementalx_defconfig | awk -F '"' '{print $2}'`
if [[ "$TOOLCHAIN" != "" ]]; then
	SUFFIX="$SUFFIX-$TOOLCHAIN"
fi

cd $TOP/kitchen/package
zip -r -9 -q $TOP/kitchen/kernel${SUFFIX}-${HASH}.zip * 

echo "Done."
