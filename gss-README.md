# Building a Debian Package

The repository has been modified from the upstream repository to support packaging a debian from the source code.

To build the debian for inclusion in a project, do:
```bash
mkdir build
cd build
cmake ..
make package
```

The resulting file can then be added to a project's `dependencies` folder as needed.
