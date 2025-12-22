# GitLab CI Gap Analysis (Reproducible Tests)

This document summarizes the gaps that prevented reproducible `ctest` runs in network-isolated containers, and the changes made to close them.

## Baseline: KDE Invent pipeline (templates)

KStars uses KDE Invent CI templates for Linux/Windows/macOS/Android builds. Those jobs primarily validate that the project compiles on the
template images and that packaging recipes remain functional.

Historically, end-to-end Ekos UI tests have been difficult to run reliably in that pipeline due to:

- missing or incompatible INDI/StellarSolver stacks in the template images
- UI tests needing deterministic, headless execution
- runtime downloads (catalogs, astrometry indexes) making the tests network-dependent

## Gaps

- **Network dependency**: tests (or code paths exercised by tests) could attempt HTTP downloads.
- **Non-deterministic data paths**: tests depended on a specific install layout or host user directories.
- **Headless flakiness**: offscreen/minimal platforms and missing window activation caused UI tests to fail intermittently.
- **Unstable fixtures**: some UI expectations were too strict for simulator timing/behaviour.
- **Non-reproducible CI images**: job-time package installs changed behaviour over time and made failures hard to reproduce locally.
- **Source tree writes during configure**: configuration steps wrote generated files into the source directory, preventing read-only mounts.

## Implemented changes

- **Deterministic test environment + offline defaults**
  - `Tests/testhelpers.h` forces isolated `HOME`/`XDG_*`, prefers offscreen, and disables networking unless `KSTARS_TEST_ENABLE_NETWORK=1`.
  - `Tests/CMakeLists.txt` defines `KSTARS_TEST_SOURCEDIR` so tests can find `kstars/data` without depending on install paths.
  - `kstars/auxiliary/kspaths.cpp` and related helpers respect the isolated test environment.

- **UI test stabilization**
  - UI tests are serialized in CTest to reduce cross-test interference.
  - Several UI tests were made less timing- and device-selection brittle for simulator-based runs.
  - `Tests/kstars_ui/test_ekos_mount.cpp` disables the mount-control test when running headless/offscreen (active window required).

- **Offline, pinned test runner image (Qt5 by default)**
  - `docker/Dockerfile` builds a reusable image with build/test dependencies and INDI/StellarSolver from the PPA.
  - `docker/run-offline-tests.sh` builds and runs `ctest` inside the container; it defaults to `-DBUILD_WITH_QT6=OFF` (Qt5).
  - `.gitlab-ci.yml` adds a custom, pinned-image test job that runs stable and unstable labels and exports JUnit via `.gitlab-ci/ctest-to-junit.xsl`.

- **Read-only source mounts**
  - `kstars/CMakeLists.txt` now generates `Options.kcfgc` and copies `kstars.kcfg` into the build directory instead of writing into the source tree.

## How to reproduce locally (no network)

Build the image once:

`docker build -t kstars-offline-tests:qt5 -f docker/Dockerfile .`

Run tests with the source mounted read-only and the container network disabled:

`docker run --rm --network=none -v "$PWD:/work:ro" -e BUILD_DIR=/tmp/kstars-build kstars-offline-tests:qt5 run-offline-tests /work`

## Remaining gaps / follow-ups

- **Qt6 test image**: Qt6 builds need a Qt6-based INDI/StellarSolver stack in the image; that is intentionally not enabled by default.
- **Template integration**: KDE template jobs are unchanged; only the custom pinned-image jobs cover the UI/offline reproducibility target.
