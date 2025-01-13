#!/bin/sh

DEFCONFIG="vendor/rvkernel-mi8937_defconfig"
CLANGDIR="/home/rve/RvClang"

rm -rf out/compile.log

mkdir -p out

export KBUILD_BUILD_USER=Radika
export KBUILD_BUILD_HOST=Rve
export USE_CCACHE=1
export PATH="$CLANGDIR/bin:$PATH"

make O=out ARCH=arm64 $DEFCONFIG

rve () {
make -j$(nproc --all) O=out LLVM=1 LLVM_IAS=1 \
ARCH=arm64 \
CC="ccache clang" \
LD=ld.lld \
AR=llvm-ar \
AS=llvm-as \
NM=llvm-nm \
STRIP=llvm-strip \
OBJCOPY=llvm-objcopy \
OBJDUMP=llvm-objdump \
READELF=llvm-readelf \
HOSTCC=clang \
HOSTCXX=clang++ \
HOSTAR=llvm-ar \
HOSTLD=ld.lld \
CROSS_COMPILE=aarch64-linux-gnu- \
CROSS_COMPILE_ARM32=arm-linux-gnueabi-
}

rve 2>&1 | tee -a out/compile.log
