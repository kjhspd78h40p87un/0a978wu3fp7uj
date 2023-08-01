# EESI Artifact Installation and Usage

# 0. Start here

All commands in this document are run from the root of the EESI repository,
unless otherwise specified.

To enhance reproducibility, extensive use is made of docker. Therefore having
a working docker installation is necessary. On Linux the typical installation procedure
is to install the docker package (`docker.io` in Ubuntu). You will need to add your user
to the `docker` group. If you receive a permission denied error, check that your user
is in the correct group for your operating system distribution.

Host machine dependencies:
- docker
- curl

Python dependencies for creating Tables:
- pandas 0.17.1

You can install the Python dependencies by running the Python scripts
inside the `defreez/eesi:dev` container.

## Compiling by hand in the dev environment

The scripts use the artifact docker image which has pre-built binaries.
If you want to build the source artifact by hand, it can be done as follows:

```
docker run -it --rm -v <PATH_TO_EESI>:/eesi defreez/eesi:dev
cd /eesi/src
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Now that EESI has been built, you can return to the [EESI README](README.md) to
finish reproducing the EESIER artifact results.
