# Dither

This library provides functions to dither png images or generate various noise images.

## Usage


## CLI
`dither [options]`
| Flag | Description | Necessity |
| :-- | :-- | :-- |
| `-h, --help` | displays help | Optional |
| `-i, --input <FILE_PATH>` | specifies the input png image file | Required |
| `-o, --output <FILE_PATH>` | specifies the output png image file | Required |
| `-b, --benchmark` | displays benchmark information to stdout | Optional |
| `-c, --convolve <KERNEL>` | convolves the input image using the specified kernel | Optional |
| `-e, --error_diffusion <ALGORITHM>` | dithers the image using the specified error diffusion algorithm | Optional |
| `-r, --ordered <THRESHOLD_MATRIX>` | dithers the image using the specified threshold matrix | Optional |

### Convolution Kernels
| KERNEL | Description |
| :-- | :-- |
| RIDGE_4 | { 0,-1, 0}</br>{-1, 4,-1}</br>{ 0,-1, 0} |
| RIDGE_8 | {-1,-1,-1}</br>{-1, 8,-1}</br>{-1,-1,-1} |
| SHARPEN_4 | { 0,-1, 0}</br>{-1, 5,-1}</br>{ 0,-1, 0} |
| SHARPEN_8 | {-1,-1,-1}</br>{-1, 9,-1}</br>{-1,-1,-1} |
| BOX_BLUR | { 1, 1, 1}</br>{ 1, 1, 1} * (1 / 9)</br>{ 1, 1, 1} |
| GAUSSIAN_BLUR | 5x5 Gaussian function matrix, sigma = 1.0 |
| UNSHARP_MASK | { 0, 0, 0, 0, 0}</br>{ 0, 0, 0, 0, 0}</br>{ 0, 0, 2, 0, 0} - GAUSSIAN_BLUR </br>{ 0, 0, 0, 0, 0}</br>{ 0, 0, 0, 0, 0}</br>

### Error Diffusion Algorithms
| ALGORITHM | Description |
| :-- | :-- |
| LINEAR | {X, 1} |
| FLOYD_STEINBERG | {0, X, 7}</br>{3, 5, 1}</br>(1 / 16)|
| JARVICE_JUDICE_NINKE | {0, 0, X, 7, 5}</br>{3, 5, 7, 5, 3}</br>{1, 3, 5, 3, 1}</br>(1 / 48) |
| STUCKI |  |
| ATKINSON |  |
| BURKES |  |
| SIERRA |  |
| SIERRA_TWO_ROW |  |
| SIERRA_LITE |  |

### Ordered Threshold Matrixes


## To Compile
Requirements: GNU C++ compiler, make<br />
Open a terminal in the root directory and enter the command
```bash
> make
```

## Algorithms
Blue Noise - [Void and Cluster](docs/1993-void-cluster.pdf)

## Libraries
[FFTW](https://www.fftw.org/) - provides Fourier transform functions<br/>
[LodePNG](https://lodev.org/lodepng/) - provides png encoding/decoding functions
