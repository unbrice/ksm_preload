# Introduction

Enables legacy applications to leverage Linux's memory deduplication.
Only works on Linux ≥ 2.6.32.

If you have multiple processes generating similar data (like multiple
applications serving different vhosts), this easy to use tool may
allow you to spare memory by deduplicating it. If two pages (typically
consecutive blocks of 4k) are identical, they will be merged to reduce
memory usage.


# Building and installing

First, please make sure you have installed the requirements:
- Kernel headers (linux-headers under Ubuntu)
- a build toolchain (build-essentials under Ubuntu)
- cmake > 2.8 (earlier version may work, please tell me if so)

The quick and dirty way:
```bash
cmake .
make
sudo make install
```

# More

More information, howto, on [vleu.net/ksm_preload](http://vleu.net/ksm_preload/).
