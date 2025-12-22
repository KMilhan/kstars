# Offline Astrometry Index Cache

Place astrometry.net index FITS files in this directory before building the
offline test container. The Dockerfile copies any `*.fits` files into
`/root/.local/share/kstars/astrometry/` inside the image.

Example (run with internet access on the host):

```bash
mkdir -p docker/astrometry
curl -L -o docker/astrometry/index-4208.fits http://data.astrometry.net/4200/index-4208.fits
curl -L -o docker/astrometry/index-4209.fits http://data.astrometry.net/4200/index-4209.fits
curl -L -o docker/astrometry/index-4210.fits http://data.astrometry.net/4200/index-4210.fits
```

If you skip this step, the offline container will still build and run tests,
but any test that depends on astrometry solving may fail or be skipped.
