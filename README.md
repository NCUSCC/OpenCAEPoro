# OpenCAEPoro

[![Build and publish doc](https://github.com/FaspDevTeam/OpenCAEPoro/actions/workflows/doc.yml/badge.svg)](https://github.com/FaspDevTeam/OpenCAEPoro/actions/workflows/doc.yml)

OpenCAEPoro or OCP is part of the OpenCAEPlus project written in C++. OCP
focuses on simulating multicomponent multiphase flows in porous media. For 
more information, please see 
[OCP repository](https://faspdevteam.github.io/OpenCAEPoro/).

## Get Started

Check manual and api (only class reference from doxygen) [website](https://porous.opencaeplus.org).

For full documentation generated by doyxgen, see [here](https://faspdevteam.github.io/OpenCAEPoro/).

## Install
There is a top level cmake configuration file to build the OpenCAEPoro lib 
and the associate test programs suite. You can use a cmake-style approach to 
compile the package; see [the official webpage](https://cmake.org) on how to
use cmake for your own operating system. 

Before building OpenCAEPoro, you need to make sure that BLAS, LAPACK, and FASP
are available. BLAS and LAPACK are ready on most systems. The FASP package (only 
the open-source part [faspsolver](https://github.com/FaspDevTeam/faspsolver) is 
required) can be downloaded from its GitHub repository. More solver options are
included in [fasp4blkoil](https://github.com/FaspDevTeam/fasp4blkoil).

The typical command for compiling OpenCAEPoro is:

Config the environment for building with cmake:
```bash
  >>> mkdir Build; cd Build; cmake ..
```

After succesfully configing, to make the library as well as examples without
installing them, just run:
```bash
  >>> make
```

To make the library and install it, run:
```bash
  >>> make install
```

Standard **uninstall** and **clean** targets are also provided. You may safely 
remove the **Build** directory as well. 

## Structure
The directory structure of OpenCAEPoro is designed as follows:
  - data/: Output files for comparison purposes
  - doc/: Documentation website
  - examples/: Input files for test examples 
  - include/: Header files
  - scripts/: Automation scripts to make life easier
  - src/: Source files
  - stash/: Files that are no longer needed, but keep for future references
  - main/: Main source code for executables 
  - CMakelist.txt: Main cmake script
  - CONTRIBUTE.md: Guidence for open source contributors
  - LICENSE: License agreement
  - README.md: This document
  - .npmrc, pnpm-lock.yaml, pnpm-workspace.yaml, package.json: Files for the OCP website
  - .clang-format: For automatic source code formatting

## License
This software is free software distributed under the Lesser General Public
License or LGPL, version 3.0 or any later versions. This software distributed
in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with OpenCAEPoro. If not, see <http://www.gnu.org/licenses/>.
