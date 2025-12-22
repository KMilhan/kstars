FROM ubuntu:25.04

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

# PPA provides newer INDI/StellarSolver versions required by KStars.
RUN apt update && apt -y install --no-install-recommends software-properties-common gpg-agent ca-certificates
RUN add-apt-repository -y ppa:mutlaqja/ppa

# Suitable for builds
RUN apt update && apt -y install --no-install-recommends \
      gcc-multilib \
      g++-multilib \
      make \
      gettext \
      coreutils \
      cmake \
      extra-cmake-modules
RUN apt update && apt -y install --no-install-recommends \
      libeigen3-dev \
      zlib1g-dev \
      libcfitsio-dev \
      libnova-dev \
      wcslib-dev \
      libraw-dev \
      libgsl-dev \
      libsecret-1-dev \
      libcurl4-openssl-dev

# INDI + StellarSolver
RUN apt update && apt -y install --no-install-recommends \
      libindi1 \
      libindi-dev \
      libindi-data \
      indi-bin \
      libstellarsolver \
      libstellarsolver-dev
RUN apt update && apt -y install --no-install-recommends \
      qt6-base-dev \
      qt6-declarative-dev \
      qt6-multimedia-dev \
      qt6-positioning-dev \
      libqt6websockets6-dev \
      libqt6svg6-dev \
      libkf6doctools-dev \
      libkf6config-dev \
      libkf6guiaddons-dev \
      libkf6i18n-dev \
      libkf6newstuff-dev \
      libkf6notifications-dev \
      libkf6xmlgui-dev \
      libkf6plotting-dev \
      libkf6crash-dev \
      libkf6notifyconfig-dev \
      libqt6sql6-sqlite \
      qt6-datavis3d-dev

# Also install Qt5 + KF5 to support building/testing without Qt6.
RUN apt update && apt -y install --no-install-recommends \
      qtbase5-dev \
      qtdeclarative5-dev \
      qtmultimedia5-dev \
      qtpositioning5-dev \
      libqt5websockets5-dev \
      libqt5svg5-dev \
      libqt5sql5-sqlite \
      libqt5datavisualization5-dev \
      qt5keychain-dev \
      libkf5config-dev \
      libkf5guiaddons-dev \
      libkf5i18n-dev \
      libkf5newstuff-dev \
      libkf5notifications-dev \
      libkf5xmlgui-dev \
      libkf5plotting-dev \
      libkf5crash-dev \
      libkf5notifyconfig-dev \
      libkf5kio-dev
RUN apt update && apt -y install --no-install-recommends \
      phonon4qt6-backend-vlc \
      qtkeychain-qt6-dev \
      qml-module-qtquick-controls

# Suitable for tests
ENV TZ=Greenwich
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN apt update && apt -y --no-install-recommends install \
      make \
      cmake \
      extra-cmake-modules \
      xplanet \
      xplanet-images \
      astrometry.net \
      appstream \
      kded5 \
      kinit \
      breeze-icon-theme \
      xvfb \
      dbus \
      dbus-x11

# qt6 theme
ENV QT_QPA_PLATFORMTHEME=qt6ct
RUN apt update && apt -y --no-install-recommends install qt6ct
RUN d=/root/.config/qt6ct ; mkdir -p "$d" && echo '[Appearance]\nicon_theme=breeze' > "$d/qt6ct.conf"

# qt5 theme
RUN apt update && apt -y --no-install-recommends install qt5ct

# Ninja
RUN apt update && apt -y --no-install-recommends install ninja-build

# CCache
ENV CCACHE_DIR=/var/ccache
RUN apt update && apt -y --no-install-recommends install ccache
RUN mkdir -p $CCACHE_DIR && chmod a=rwx $CCACHE_DIR && update-ccache-symlinks

# AppImage
RUN apt update && apt -y --no-install-recommends install \
      python3-pip \
      python3-setuptools \
      patchelf \
      desktop-file-utils \
      libgdk-pixbuf2.0-dev \
      fakeroot \
      wget

# Saxon
RUN apt install -y --no-install-recommends \
      libsaxon-java \
      openjdk-11-jre-headless

# Prepare astrometry directory (tests should not require downloading indexes at runtime).
RUN mkdir -p /root/.local/share/kstars/astrometry
COPY docker/astrometry/ /opt/kstars/astrometry/
RUN cp -a /opt/kstars/astrometry/*.fits /root/.local/share/kstars/astrometry/ 2>/dev/null || true

# Offline test runner helper (usable with `docker run --network=none ... run-offline-tests`).
COPY docker/run-offline-tests.sh /usr/local/bin/run-offline-tests
RUN chmod +x /usr/local/bin/run-offline-tests

# From https://invent.kde.org/sysadmin/ci-tooling/-/blob/master/system-images/suse-qt615/Dockerfile
RUN dbus-uuidgen > /etc/machine-id

CMD /bin/bash
