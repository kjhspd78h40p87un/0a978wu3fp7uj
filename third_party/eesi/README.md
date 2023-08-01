#### Note: This is a modified fork of EESI that exists only for comparison against EESIER.

The original git repository for EESI is https://github.com/ucd-plse/eesi

This repository contains the tool EESI. This is the software artifact that
accompanies the paper "Effective Error-Handling Specification Inference via
Domain Knowledge Expansion" by Daniel DeFreez, Haaken Martinson Baldwin,
Cindy Rubio-González, and Aditya V. Thakur. 

# EESI Setup

Running EESI requires that you have [docker](https://docs.docker.com/engine/install/)
setup and installed. Once you have installed `docker`, please follow the
[INSTALL guide](INSTALL.md) to build and setup EESI.

## Replicating EESI Results (approx. running time: 20 minutes, ~15GB memory)

After following the [INSTALL guide](INSTALL.md), ensure that you are in the
docker container and run the script from the **root directory of EESI**:
```bash
$ ./src/scripts/setup.sh
```

This script just creates the `results/artifact` folder and copies over benchmark
bitcode files from [../../testdata/benchmarks/bitcode](../../testdata/benchmarks/bitcode).
Once done with the `setup.sh` script, you can run the
[tabledata.sh](./src/scripts/tabledata.sh) script from the **root directory of EESI**:

```bash
$ ./src/scripts/tabledata.sh [littlefs | zlib | netdata | openssl | mbedtls | pidgin | large | small | all]
```

Where you can optionally input the individual benchmark that you wish to infer
specifications for or you can select one of the optional benchmark subsets:
`large` or `small`. `large` includes benchmarks `OpenSSL`, `MbedTLS`, and `Netdata`.
`small` includes benchmarks `zlib`, `littlefs`, and `pidgin`. If no optional
argument is supplied, then the results for all benchmarks will be generated. 

Please ensure that you are aware of the large memory consumption that occurs
when EESI performs its analysis. Running on all benchmarks can consume nearly
~100 GB of memory.

After running this script, the specifications should now be stored in
[./results/artifact/](./results/artifact/) under this project. You can use
these scripts to calculate the precision, recall and F-score of EESI by using these
specifications as mentioned in the [EESIER README](../../README.md/######Precision,-Recall,-and-F-Score).

## Specifications Count Table (approx. running time: <1 minute)

After running the `tabledata.sh` script previously, we can replicate the
specifications count table for EESI, i.e., `Table IV` from the EESIER paper,
by running the script:
```
$ ./src/scripts/table4.py --results ./results/artifact/
```

If ran correctly, the resulting output should be:
```
    Program   <0   >0   ==0  <=0 >=0 !=0 top
0   openssl  307  104  2290  310  73   5  10
1    pidgin    1    4    24    0   0   0   0
2   mbedtls  359    0    37  380   6   2   7
3   netdata   13   43    98    3   3   1   0
4  littlefs   37    0     4    3   0   0   0
5      zlib   68    0     7    1   0   0   0
```
