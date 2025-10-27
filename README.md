
# innoextract - A tool to unpack installers created by Inno Setup

[Inno Setup](https://jrsoftware.org/isinfo.php) is a tool to create installers for Microsoft Windows applications. innoextract allows to extract such installers under non-Windows systems without running the actual installer using wine. innoextract currently supports installers created by Inno Setup 1.2.10 to 6.3.3.

In addition to standard Inno Setup installers, innoextract also supports some modified Inno Setup variants including Martijn Laan's My Inno Setup Extensions 1.3.10 to 3.0.6.1 as well as GOG.com's Inno Setup-based game installers. innoextract is able to unpack Wadjet Eye Games installers (to play with AGS), Arx Fatalis patches (for use with Arx Libertatis) as well as various other Inno Setup executables.

innoextract is available under the ZLIB license - see the LICENSE file.

See the website for [Linux packages](https://constexpr.org/innoextract/#packages).

## Contact

[Website](https://constexpr.org/innoextract/)

Author: [Daniel Scharrer](https://constexpr.org/)

## Dependencies

* **[Boost](https://www.boost.org/) 1.37** or newer
* **liblzma** from [xz-utils](https://tukaani.org/xz/) *(optional)*
* **iconv** (*optional*, either as part of the system libc, as is the case with [glibc](https://www.gnu.org/software/libc/) and [uClibc](https://uclibc.org/), or as a separate [libiconv](https://www.gnu.org/software/libiconv/))

For Boost you will need the headers as well as the `iostreams`, `filesystem`, `date_time`, `system` and `program_options` libraries. Older Boost version may work but are not actively supported. The boost `iostreams` library needs to be build with zlib and bzip2 support.

While innoextract can be built without liblzma by manually setting `-DUSE_LZMA=OFF`, it is highly recommended and you won't be able to extract most installers created by newer Inno Setup versions without it.

To build innoextract you will also need **[CMake](https://cmake.org/) 2.8** and a working C++ compiler, as well as the development headers for liblzma and boost.

See the Website for [operating system-specific instructions](https://constexpr.org/innoextract/install).

## Compile and install

To compile innoextract, run:

    $ mkdir -p build && cd build
    $ cmake ..
    $ make

To install the binaries system-wide, run as root:

    # make install

The default build settings are tuned for users - if you plan to make changes to Arx Libertatis you should append the `-DDEVELOPER=1` option to the `cmake` command to enable debug output and fast incremental builds.

### Build options:

| Option                    | Default   | Description |
|:------------------------- |:---------:|:----------- |
| `BUILD_DECRYPTION`        | `ON`      | Build decryption support.
| `USE_LZMA`                | `ON`      | Use `liblzma`.
| `WITH_CONV`               | *not set* | The charset conversion library to use. Valid values are `iconv`, `win32` and `builtin`¹. If not set, a library appropriate for the target platform will be chosen.
| `CMAKE_BUILD_TYPE`        | `Release` | Set to `Debug` to enable debug output.
| `DEBUG`                   | `OFF`²    | Enable debug output and runtime checks.
| `DEBUG_EXTRA`             | `OFF`     | Expensive debug options.
| `SET_WARNING_FLAGS`       | `ON`      | Adjust compiler warning flags. This should not affect the produced binaries but is useful to catch potential problems.
| `SET_NOISY_WARNING_FLAGS` | `OFF`     | Enable warnings with false positives many cases that still need to be fixed.
| `SET_OPTIMIZATION_FLAGS`  | `ON`      | Adjust compiler optimization flags.
| `CXX_STD_VERSION`         | `2017`    | Maximum C++ standard version to enable.
| `USE_DYNAMIC_UTIMENSAT`   | `OFF`     | Dynamically load utimensat(2) if not available at compile time.
| `USE_STATIC_LIBS`         | `OFF`³    | Turns on static linking for all libraries, including `-static-libgcc` and `-static-libstdc++`. You can also use the individual options below:
| `LZMA_USE_STATIC_LIBS`    | `OFF`⁴    | Statically link `liblzma`.
| `Boost_USE_STATIC_LIBS`   | `OFF`⁴    | Statically link Boost. See also `FindBoost.cmake`.
| `ZLIB_USE_STATIC_LIBS`    | `OFF`⁴    | Statically link `libz`. (used via Boost)
| `BZip2_USE_STATIC_LIBS`   | `OFF`⁴    | Statically link `libbz2`. (used via Boost)
| `iconv_USE_STATIC_LIBS`   | `OFF`⁴    | Statically link `libiconv`.
| `STRICT_USE`              | `OFF`     | Abort if there are missing optional dependencies.
| `DEVELOPER`               | `OFF`     | Enable build options suitable for developers⁵.
| `FASTLINK`                | `OFF`⁶    | Optimize for link speed.
| `USE_LTO`                 | `ON`²     | Use link-time code generation.
| `USE_LD`                  | `best`⁸   | Linker to use - `default`, `mold`, `lld`, `gold`, `bfd` or `best`
| `BUILD_TESTS`             | `OFF`⁶    | Build unit tests that can be run using `make check`
| `RUN_TESTS`               | `OFF`⁷    | Automatically run tests
| `RUN_TARGET`              | (none)    | Wrapper to run binaries produced in the build process
1. The builtin charset conversion only supports Windows-1252 and UTF-16LE. This is normally enough for filenames, but custom message strings (which can be included in filenames) may use arbitrary encodings.
2. Enabled automatically if `CMAKE_BUILD_TYPE` is set to `Debug`.
3. Under Windows, the default is `ON`.
4. Default is `ON` if `USE_STATIC_LIBS` is enabled.
5. Currently this and enables `DEBUG`, `BUILD_TESTS`, `RUN_TESTS` and `FASTLINK` for faster incremental builds and improved debug output, unless those options have been explicitly specified by the user.
6. Enabled automatically if `DEVELOPER` is enabled.
7. Enabled automatically if `DEVELOPER` is enabled unless cross-compiling without `RUN_TARGET` set
8. Disabled automatically (set to `default`) if both `SET_OPTIMIZATION_FLAGS` and `FASTLINK` are disabled. `best` will select the most suited linker based on availability and other settings such as `USE_LTO`.

Install options:

| Option                      | Default              | Description |
|:--------------------------- |:--------------------:|:----------- |
| `CMAKE_INSTALL_PREFIX`      | `/usr/local`         | Where to install innoextract.
| `CMAKE_INSTALL_BINDIR`      | `bin`                | Location for binaries (relative to prefix).
| `CMAKE_INSTALL_DATAROOTDIR` | `share`              | Location for data files (relative to prefix).
| `CMAKE_INSTALL_MANDIR`      | `${DATAROOTDIR}/man` | Location for man pages (relative to prefix).

Set options by passing `-D<option>=<value>` to cmake.

## Run

To extract a setup file to the current directory run:

    $ innoextract <file>

A list of available options can be retrieved using

    $ innoextract --help

Documentation is also available as a man page:

    $ man 1 innoextract

## Limitations

* There is no support for extracting individual components and limited support for filtering by name.

* Included scripts and checks are not executed.

* The mapping from Inno Setup variables like the application directory to subdirectories is hard-coded.

* Names for data slice/disk files in multi-file installers must follow the standard naming scheme.

A perhaps more complete, but Windows-only, tool to extract Inno Setup files is [innounp](http://innounp.sourceforge.net/).

Extracting Windows installer executables created by programs other than Inno Setup is out of the scope of this project. Some of these can be unpacked by the following programs:

* [cabextract](https://cabextract.org.uk/)

* [unshield](https://github.com/twogood/unshield)

## Disclaimer

This project is in no way associated with Inno Setup or [jrsoftware.org](https://jrsoftware.org/).
