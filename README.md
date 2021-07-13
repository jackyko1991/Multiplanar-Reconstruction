# Multiplanar Reconstruction

A simple C++ software for curved multiplanar reconstruction (MPR).

## Usage
Example usage: `./Multiplanar-Reconstruction.exe <image-path> <centerline-path> <output-path>`

You should first follow https://github.com/jackyko1991/Vessel-Centerline-Extraction to compute centerline from vessel surface file.

Use `-h` command to list detail usage:

```bash
$ ./Multiplanar-Reconstruction.exe -h
Perform multiplanar reconstruction with given image and centerline.

  -h [ --help ]                      Displays this help.
  -i [ --input ] arg                 Image file to process.
  -c [ --centerline ] arg            Centerline file to process.
  -o [ --output ] arg                Output file path.
  -x [ --imageSize ] arg (=120)      Inplane image size for MPR output (default
                                     = 120)
  -r [ --imageSpacing ] arg (=0.25)  Inplane image spacing for MPR output
                                     (default = 0.25)
  -s [ --splineSpacing ] arg (=0.25) Subdivided centerline point spacing,
                                     equivalent to z direction resolution for
                                     MPR output (default = 0.25)
```

## Example Data
Sample dataset can be found from `./Data`. Corresponding centerlines are included in the same folder. The data is carotid artery with stenosis at CCA/ECA/ICA bifurcation. We would like perform MPR to "flatten" the vessel along centerline for better result visualization.

```bash
# reslice image
$ .\Multiplanar-Reconstruction.exe -i <repo-root>\Data\image.nii.gz -c <repo-root>\Data\centerline.vtp -o <repo-root>\Data\mpr.nii.gz
```

## Compile from source

We only tested with MSVC 2015 on Windows 10. You have to prepare following dependency first:
- VTK 8.0
- ITK 4.13
- VMTK 1.14
- [Boost > 1.66](https://www.boost.org/)

### CMake Boost Settings
We use [precompiled Boost C++ library](https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.zip) for convenience.
Uncompress the zipped file to arbitrary directory `<boost-dir>`.

```bash
Boost_INCLUD_DIR := <boost-dir>/boost_1_66_0
Boost_LIBRARY_DIR_RELEASE := D<boost-dir>/boost_1_66_0/lib64-msvc-14
Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE := D<boost-dir>/boost_1_66_0/lib64-msvc-14/boost_program_options-vc140-mt-x64-1_66.lib
```