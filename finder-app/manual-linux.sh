#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(dirname $(realpath -e $BASH_SOURCE))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
READELF=${CROSS_COMPILE}readelf

function copy_libdepends() {

	local program_interpreter=$(${READELF} -l ${OUTDIR}/rootfs/bin/busybox | grep -Eo "ld-[[:alnum:]]*-[[:alnum:]]*.so.[0-9]*" )
	local shared=$(${READELF} -d ${OUTDIR}/rootfs/bin/busybox | grep -Eow "lib[[:alnum:]]*\.so\.[[:digit:]]*" )
    test -z "${shared}" && return 1
    test -z "${program_interpreter}" && return 1

	for dep in ${program_interpreter} ${shared};
    do
        find ${FINDER_APP_DIR}/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu -name "${dep}" -exec cp -v {} ${OUTDIR}/rootfs/lib \;
    done
	return 0
}

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	test -d "${FINDER_APP_DIR}/outdir" && OUTDIR="${FINDER_APP_DIR}/outdir" || OUTDIR=${1}
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ];
then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi

if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    for action in mrproper defconfig all modules dtbs;
    do
        test -f ${OUTDIR}/linux-stable/.config && [ ${action} = "mrproper" ] && continue;
        [ ${action} = "all" ] && jobs="-j4" || jobs=
        make ${jobs} ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} ${action}
    done
fi

echo "Adding the Image in outdir"
cp -v "${OUTDIR}"/linux-stable/arch/arm64/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
for dir in bin boot etc home lib usr usr/bin usr/sbin sbin dev proc sys opt;
do
    mkdir -vp ${OUTDIR}/rootfs/${dir}
done

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
sudo make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX="${OUTDIR}/rootfs" install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
copy_libdepends
if [ $? -ne 0 ];
then
    echo "copy library dependencies fail."
fi

# TODO: Make device nodes
sudo mknod ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod ${OUTDIR}/rootfs/dev/console c 5 1

# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE} writer

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
for file in writer finder.sh finder-test.sh conf autorun-qemu.sh;
do
    if [ -L ${file} ]
    then
		sudo cp -vr $(readlink -e ${file}) ${OUTDIR}/rootfs/home
    else
        sudo cp -v ${FINDER_APP_DIR}/${file} ${OUTDIR}/rootfs/home
    fi
done

# TODO: Chown the root directory
sudo chown root:root -R ${OUTDIR}/rootfs/*

# TODO: Create initramfs.cpio.gz
( cd ${OUTDIR}/rootfs; find . | cpio --create --format=newc | gzip ) > ${OUTDIR}/initramfs.cpio.gz
