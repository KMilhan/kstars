# Donut Autofocus Container Setup

This note collects the packages and workflows that proved necessary to configure a build container for the new donut-metric autofocus pipeline.

## Base Image Guidance

* Ubuntu 24.04 LTS (noble) or Debian 12 work well because they ship recent Qt 5.15 and KDE Frameworks 5.115 stacks.
* Ensure the container has at least 6 GB of free disk space before installing dependencies—the KDE SDK pulls in many runtime assets.

## Core Packages to Install (APT)

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake ninja-build git extra-cmake-modules \
    qtbase5-dev qtbase5-dev-tools qtdeclarative5-dev qtdeclarative5-dev-tools \
    libqt5svg5-dev libqt5websockets5-dev libqt5opengl5-dev \
    libqt5datavisualization5-dev qt5keychain-dev \
    libkf5plotting-dev libkf5xmlgui-dev libkf5kio-dev libkf5newstuff-dev \
    libkf5doctools-dev libkf5notifications-dev libkf5notifyconfig-dev \
    libkf5wallet-bin libkf5globalaccel-dev libkf5configwidgets-dev \
    libkf5widgetsaddons-dev libkf5archive-dev libkf5completion-dev \
    libkf5itemviews-dev libkf5service-dev libkf5solid-dev \
    libcfitsio-dev libindi-dev libstellarsolver-dev wcslib-dev \
    libraw-dev libgsl-dev libeigen3-dev libxisf-dev \
    libqt5test5 qttranslations5-l10n gettext \
    libnova-dev libusb-1.0-0-dev libsecret-1-dev \
    xplanet xplanet-images breeze-icon-theme
```

### Optional but Recommended

* **INDI ≥ 2.0.0** – the in-tree CMake checks warn if an older revision is present. When using Ubuntu packages, add the [INDI nightly PPA](https://launchpad.net/~mutlaqja/+archive/ubuntu/ppa) to reach 2.0+.
* **OpenCV** – required only if donut blurriness statistics are enabled: `sudo apt-get install libopencv-dev`.
* **ERFA** – unit tests for some celestial mechanics expect it: `sudo apt-get install liberfa-dev`.
* **Python toolchain** – needed for dataset synthesis and NASA simulations: `sudo apt-get install python3 python3-venv python3-pip`.

## Build & Test Flow

1. Configure:
   ```bash
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
   ```
2. Build the focus unit tests (the full GUI build is large; this target isolates the new code path):
   ```bash
   cmake --build build --target testdonutmetric
   ```
3. Run the new donut metric test and the existing focus suite:
   ```bash
   cd build
   ctest -R FocusDonutMetricTest --output-on-failure
   ctest -R Focus --output-on-failure
   ```
4. Optional full build (for regression testing against the GUI):
   ```bash
   cmake --build build
   ctest --output-on-failure
   ```

## Offline Test Container

For reproducible, **offline** test runs (e.g., `--network=none`), use the
Docker image defined in `docker/Dockerfile`. The image bakes in build/test
dependencies and provides a `run-offline-tests` helper script.

### Build the Image (one-time, requires network)

```bash
docker build -f docker/Dockerfile -t kstars-tests-offline .
```

### Run Tests with No Network

```bash
docker run --rm --network=none \
  -v "$PWD":/work/kstars \
  -w /work \
  kstars-tests-offline \
  run-offline-tests /work/kstars
```

The offline container defaults to Qt5 builds. Note that Qt6 builds require a
Qt6-based StellarSolver stack, which is not available in all distributions/PPAs.

### Optional: Limit the Test Set

```bash
docker run --rm --network=none \
  -v "$PWD":/work/kstars \
  -w /work \
  -e KSTARS_CTEST_ARGS="-L stable --output-on-failure" \
  kstars-tests-offline \
  run-offline-tests /work/kstars
```

### Astrometry Index Cache (optional)

Some align-related tests may require astrometry index files. To bake those into
the offline image, place `*.fits` files in `docker/astrometry/` before building.
See `docker/astrometry/README.md` for a quick download example.

## NASA Optical Simulation Validation Plan

To validate the donut metric against realistic segmented mirrors or centrally obstructed telescopes, adopt the following workflow once the base image is ready:

1. **Install NASA POPPY/WebbPSF tools** inside a Python virtual environment:
   ```bash
   python3 -m venv ~/donut-env
   source ~/donut-env/bin/activate
   pip install poppy webbpsf astropy numpy
   ```
2. **Model a Schmidt-Cassegrain Telescope** by configuring POPPY with a circular aperture, central obstruction (~34%), and secondary mirror support vanes. Generate pairs of defocused PSFs at ±3 waves of defocus and export them as FITS files.
3. **Convert to Ekos test assets** by downsampling the FITS frames to match the autofocus subframe resolution (e.g., 128×128) and storing them under `Tests/data/focus/donut/` for future automated regression tests.
4. **Automate evaluation** by writing a small Qt-less driver (or extending `testdonutmetric`) that loads the FITS frames with `FITSData`, invokes `Focus::estimateDonutMetric`, and records metric/weight trends versus imposed defocus.
5. **Track performance metrics**: compare the recovered metric against the known simulated defocus in microns, and log convergence behaviour for Linear 1 Pass. Use this dataset as a baseline for future algorithm tweaks.

Document any additional external data sources or simulation parameters within `Tests/data/focus/README.md` so the provenance stays clear.
