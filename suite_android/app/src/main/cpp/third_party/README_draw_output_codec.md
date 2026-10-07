# Third-party output codec

`jo_jpeg.cpp` is the public-domain jo_jpeg baseline JPEG writer (Jon Olick / jpcy/jo_jpeg), vendored only for D.RAW downstream presentation export.

D.RAW does not infer 4:4:4 or quality from the library name or requested settings. Every candidate output is parsed by `HighFidelityJpegContractV01` and rejected unless the JPEG itself carries SOF0 sampling 1x1/1x1/1x1, exact dimensions, and the admitted quality-100 all-one quantization tables.
