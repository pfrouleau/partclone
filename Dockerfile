# This Dockerfile creates an images that can be used to build partclone and tests the binaries.

FROM ubuntu:focal

# Ensure all Dockerfile package installation operations are non-interactive, DEBIAN_FRONTEND=noninteractive is insufficient [1]
# [1] https://github.com/phusion/baseimage-docker/issues/58
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections

RUN apt-get update

RUN apt-get install -y \
        # Build tools
        autoconf docbook-xsl fuse gcc libtool-bin make m4 pkg-config quilt sgml-base \
        # Build dependencies
        dpkg-dev libncurses5-dev libncursesw5-dev libreadline5 libssl-dev \
        # Install optional tools for quality-of-life when debugging
        git nano rsync tmux vim

# Install the file system dependencies separately to exploit a little bit Docker's image caching.
# Not ideal, but better than having all of them in one big block. And if we want to remove the
# support for one of them, it will be easy to remove the related dependencies.

## Btrfs
RUN apt-get install -y btrfs-progs libblkid-dev

## Ext family
RUN apt-get install -y e2fslibs-dev libext2fs-dev

## Fat family
RUN apt-get install -y dosfstools exfat-fuse exfat-utils

## Hfs
RUN apt-get install -y hfsprogs

## jfs - Has a lot of dependencies
#RUN apt-get install -y jfsutils libguestfs-jfs
# Disabled because configure does not find libjfs

## nilfs
RUN apt-get install -y nilfs-tools

## Ntfs
RUN apt-get install -y ntfs-3g ntfs-3g-dev

## Reiser4
RUN apt-get install -y libreiser4-dev reiser4progs

## Reiserfs - Has a lot of dependencies
#RUN apt-get install libguestfs-reiserfs
# Disabled because configure does not find reiserfs/reiserfs.h

## Vmware
RUN apt-get install -y vmfs-tools

## Xfs
RUN apt-get install -y xfslibs-dev xfsprogs

# Not availavle while running from Linux or Windows
#bup - Deprecated? (NB: not required for partclone; will be for rescuezilla)
#libjfs-dev
#libntfs-3g88 
#libufs2-dev
#libvmfs

# Other packages present in Rescuezilla's Dockerfile that does not seem to be required:
# comerr-dev
# libbsd-dev
# libtinfo-dev
# libxslt1.1
# squashfs-tools
# xsltproc

# Restore interactivity of package installation within Docker
RUN echo 'debconf debconf/frontend select Dialog' | debconf-set-selections

WORKDIR /projects

# Scripts to simplify tasks in container
COPY scripts/* .
