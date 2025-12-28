#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
ROOTFS_PATH=${OUTDIR}/rootfs
CROSS_PATH="$(dirname "$(dirname "$(which "${CROSS_COMPILE}gcc")")")"

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here

    echo "Cleaning the source tree"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    echo "Building the default configuration"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    echo "Build all, modules and dtbs"
    make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all modules dtbs

fi

echo "Adding the Image in outdir"

cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories

echo "Create rootfs folders"
mkdir -p ${ROOTFS_PATH}
cd ${ROOTFS_PATH}

mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/lib64 usr/sbin var/log

## In case we need to install the modules in the new FS
#cd ${OUTDIR}/linux-stable
#make INSTALL_MOD_PATH=${ROOTFS_PATH} modules_install

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    echo "Confgure BusyBox"
    make distclean
    make defconfig
    echo "Petching the TC build to avoid error from kernel 6.6"
    sed -i -E \
        -e 's/^CONFIG_TC=.*/# CONFIG_TC is not set/' \
        -e 's/^CONFIG_FEATURE_TC_INGRESS=.*/# CONFIG_FEATURE_TC_INGRESS is not set/' \
        .config
else
    cd busybox
fi

# TODO: Make and install busybox
echo "Make BusyBox"
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc)
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX=${ROOTFS_PATH} install 

cd ${ROOTFS_PATH}

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs

echo "Copy the libraries in the rootfs from ${CROSS_PATH}"
cp ${CROSS_PATH}/aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 ${ROOTFS_PATH}/lib/ld-linux-aarch64.so.1

cp ${CROSS_PATH}/aarch64-none-linux-gnu/libc/lib64/libm.so.6 ${ROOTFS_PATH}/lib64/libm.so.6
cp ${CROSS_PATH}/aarch64-none-linux-gnu/libc/lib64/libresolv.so.2 ${ROOTFS_PATH}/lib64/libresolv.so.2
cp ${CROSS_PATH}/aarch64-none-linux-gnu/libc/lib64/libc.so.6 ${ROOTFS_PATH}/lib64/libc.so.6 

# TODO: Make device nodes
echo "Make the devices"

cd ${ROOTFS_PATH}/dev
sudo mknod -m 666 null c 1 3
sudo mknod -m 666 zero c 1 5
sudo mknod -m 666 random c 1 8
sudo mknod -m 666 urandom c 1 9
sudo mknod -m 600 console c 5 1
sudo mknod -m 666 tty c 5 0

# TODO: Clean and build the writer utility
echo "Build the writer file"
cd ${FINDER_APP_DIR}

make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} clean
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs

echo "Copy the build files to home"
cd ${ROOTFS_PATH}
cp -r ${FINDER_APP_DIR}/* ./home/
cp -rL ${FINDER_APP_DIR}/conf conf

# TODO: Chown the root directory
echo "Change the ownership of the root filesystem"
sudo chown -R root ${ROOTFS_PATH}
sudo chgrp -R root ${ROOTFS_PATH}

# TODO: Create initramfs.cpio.gz
echo "Build initrfamfs"
cd ${ROOTFS_PATH}
find . | cpio -H newc -ov --owner root:root | gzip -9 > ${OUTDIR}/initramfs.cpio.gz
