# SAMIX - Samba for UNIX/X11


## Prerequisites

### MacOS

You need to install XQuartz, last found [here](https://www.xquartz.org/).
In addition, developer tools (e.g. Clang) will be needed though it should
ask you to install it when attempting to compile.

### Linux

The following packages should be installed using the appropriate package
manager (`apt`, `yum`, `dnf`, etc.):
```
libX11
libX11-devel
gcc
git
```

## Compilation and Execution

To compile:

```
git clone https://github.com/drmarkwslater/samix.git
cd samix
Prgm/compile         # builds samba/samix
Prgm/compile tango   # builds tango/ipaix
```

To run:
```
Acquis/executables/samix   # Runs Samba/samix
Acquis/executables/ipaix   # Runs Tango/ipaix
```

This has been tested on MacOS, Ubuntu, Debian and Fedora.
