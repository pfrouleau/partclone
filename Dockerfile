# This Dockerfile creates an images that can be used to build partclone and tests the binaries.

FROM ubuntu:focal

# Install custom package sources from Partclone maintainer
RUN apt-get update && \
    apt-get install -y gnupg2 wget

RUN wget http://drbl.nchc.org.tw/GPG-KEY-DRBL; \
    apt-key add GPG-KEY-DRBL; \
    echo "deb http://free.nchc.org.tw/drbl-core drbl stable testing unstable dev" >> /etc/apt/sources.list ;\
    echo "deb-src http://free.nchc.org.tw/drbl-core drbl stable testing unstable dev" >> /etc/apt/sources.list

# Ensure all Dockerfile package installation operations are non-interactive, DEBIAN_FRONTEND=noninteractive is insufficient [1]
# [1] https://github.com/phusion/baseimage-docker/issues/58
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections

RUN apt-get update

# Install all partclone's dependencies
RUN apt-get build-dep -y partclone

# Install optional tools for quality-of-life when debugging
RUN apt-get install -y git nano rsync tmux vim

# Restore interactivity of package installation within Docker
RUN echo 'debconf debconf/frontend select Dialog' | debconf-set-selections

WORKDIR /projects

# Scripts to simplify tasks in container
COPY scripts/* .
