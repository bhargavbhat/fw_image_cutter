# FW Image Cutter
Simple tool for generating FoTA compatible images from toolchain output

### Building
On linux, `make all`

### Generating FoTA Compatible Images
- To generate image for Upper/Lower sections: `make upper` or `make lower`
- To generate images without metadata (for test/comparison): `make upper_nometa` or `make lower_nometa`

### Tweaking the Tool
- All addresses/constants in `Makefile`
- `SECTION_SIZE` -> Size of each FoTA image (must exactly match the upper/lower partition size in linker script)
- `UPPER_SECTION_ADDRESS` and `LOWER_SECTION_ADDRESS` -> Start address for upper and lower images respectively
- `_SIG`, `_MAJOR_VER` and `_MINOR_VER` -> Metadata for respective images
- CRC routine for the board is `crc32.h`
