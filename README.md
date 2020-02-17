# Light Field Image Format
Light Field Image Format (LFIF) is an implementation of a lossy light field image compression method which compresses the light field as a four-dimensional volume to exploit the correlation between adjacent views. The method is based on JPEG baseline encoder. The LFIF library can handle a variety of data and is able to compress a light field as a series of (two or three)-dimensional images or as one four-dimensional image. This project also contains a minimalistic PPM library for reading and writing PPM files and set of tools for light field image compression and decompression.
Evaluation of this encoder was performed in the paper (Barina, 2019). The method is described in the paper (Dlabaja, 2019).

## Install
The project depends on ``GCC`` with C++ 17 implemented. The compilation is organized with the ``make`` tool.
To compile the whole project, simply write

    make

in its root folder. To compile only part of the project, you can specify which part by writing

    make <FOLDER>

where ``<FOLDER>`` can be ``liblfif`` for LFIF library only, ``libppm`` for PPM library only. The ``tools`` specifier will compile the whole project because tools depend on it. Compilation outputs its binaries in ``bin`` subfolder of each folder.

The documentation can be generated with ``Doxygen`` tool with the command

    make doc

The project can be cleaned with ``make clean``.

## Usage
The tools are able to compress a light field image which exists as a set of ppm images representing individual views.
The image files must be sorted in row or column order and the view indices must be part of file names.
To compress a light field image, use the compression tool like this:

    ./tools/bin/lfif{2,3,4}d_compress -i path/to/##/input/###.ppm -o path/to/output.lf -q <quality> [-p] [-s] -- [<x> <y> ...]

The ``#`` characters can be anywhere in the path and are expanded with sequential numbers. This means that a path ``##/file#.ppm`` will be expanded to files ``00/file0.ppm``, ``00/file1.ppm``, ..., ``99/file9.ppm``. The properties of all the images must be the same. The images must together resemble a square of views.
The ``<quality>`` parameters specify a compression quality. The tools consume any floating-point number from 1.0 to 100.0. The higher the number, the better the visual quality, but worse compression ratio.
The ``-p`` switch will turn on the intra prediction. This will take some extra time to compute.
The ``-s`` switch will try to find a disparity of the views and compensate for it. This often leads to better compression ratios and it is recommended.
At the end of the switches and parameters, the compression block size can be specified with d positive integer numbers, where d is the dimensionality of the compression. By default, the block size will be a d-dimensional cube with a side equal to the square root of view count.

The decompression tool can decompress images compressed with corresponding compression tools. The tools will output the individual views as a set of ppm files with names specified by a mask. To decompress a light filed image, use decompression tools like this:

    ./tools/bin/lfif{2,3,4}d_decompress -i path/to/input.lf -o path/to/##/output/###.ppm

The ``#`` characters will be expanded in a similar way as with compression tools.

## License
See [LICENSE](https://github.com/xdlaba02/light-field-image-format/blob/master/LICENSE) for license and copyright information.

## References
Barina, D.; Chlubna, T.; Solony, M.; aj.: Evaluation of 4D Light Field Compression Methods. In International Conference in Central Europe on Computer Graphics, Visualization and Computer Vision (WSCG), 2019.

Dlabaja, D.: 4D-DCT Based Light Field Image Compression. In Excel\@FIT, 2019.
