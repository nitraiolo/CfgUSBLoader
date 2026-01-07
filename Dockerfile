FROM debian:bookworm-slim

LABEL maintainer="devkitPro developers support@devkitpro.org"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y --no-install-recommends apt-utils && \
    apt-get install -y --no-install-recommends sudo ca-certificates pkg-config curl wget bzip2 xz-utils make libarchive-tools doxygen gnupg && \
    apt-get install -y --no-install-recommends git git-restore-mtime && \
    apt-get install -y --no-install-recommends rsync && \
    apt-get install -y --no-install-recommends zip unzip ninja-build && \
    apt-get install -y --no-install-recommends python3 python-is-python3 python3-lz4 && \
    apt-get install -y --no-install-recommends locales && \
    apt-get install -y --no-install-recommends patch && \
    apt-get install -y --no-install-recommends build-essential gettext cppcheck && \
    echo "deb http://deb.debian.org/debian bookworm-backports main" >> /etc/apt/sources.list.d/bookworm-backports.list && \
    apt update && \
    apt-get install -y --no-install-recommends -t bookworm-backports cmake && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && \
    echo -n "UTC" > /etc/timezone && \
    apt-get -y autoremove --purge && \
    apt-get -y clean && \
    dpkg-reconfigure -f noninteractive tzdata && \
    apt-get -y purge locales-all && \
    dpkg-reconfigure -f noninteractive locales && \
    echo "en_US.UTF-8 UTF-8" > /etc/locale.gen && \
    echo "en_US.UTF-8 UTF-8" > /etc/default/locale && \
    locale-gen en_US.UTF-8 && \
    update-locale

#RUN  mkdir -p /opt/devkitpro && curl 'https://wii.leseratte10.de/devkitPro/devkitPPC/r24%20%282011%29/devkitPPC_r24-x86_64-linux.tar.bz2' --silent --show-error --location | tar xjf - -C /opt/devkitpro
RUN  mkdir -p /opt/devkitpro && curl 'https://wii.leseratte10.de/devkitPro/devkitPPC/r27%20%282014%29/devkitPPC_r27-x86_64-linux.tar.bz2' --silent --show-error --location | tar xjf - -C /opt/devkitpro
#RUN  mkdir -p /opt/devkitpro/libogc && curl 'https://wii.leseratte10.de/devkitPro/libogc/libogc_1.8.9%20%282012-02-26%29/libogc-1.8.9-07.10.2011.zip' --silent --show-error --location | bsdtar xzf - -C /opt/devkitpro/libogc
RUN  mkdir -p /opt/devkitpro/libogc && curl 'https://wii.leseratte10.de/devkitPro/libogc/libogc_1.8.12%20%282014-04-02%29/libogc-1.8.12.tar.bz2' --silent --show-error --location | tar xjf - -C /opt/devkitpro/libogc

ENV LANG=en_US.UTF-8

ENV DEVKITPRO=/opt/devkitpro
ENV DEVKITPPC=${DEVKITPRO}/devkitPPC

WORKDIR /root
