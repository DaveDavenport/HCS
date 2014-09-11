# Installation guide

## DEPENDENCY

### For building:

* C compiler that supports the c99 standard. (gcc or clang)
* make
* autoconf
* automake

## Install a checkout from git

Generate build system:

```
autoreconf -i
```

Create a build directory:

```
mkdir build
```

Check dependencies and configure build system:

```
../configure
```

Build rofi:

```
make
```

The actual install, execute as root (if needed):

```
make install
```


## Options for configure

When you run the configure step there are several you can configure. (To see the full list type
`./configure --help` ).

The most useful one to set the installation prefix:

```
./configure --prefix=<installation path>
```

f.e.

```
./configure --prefix=/usr/
```

### Install locally

or to install locally:

```
./configure --prefix=${HOME}/.local/
```

