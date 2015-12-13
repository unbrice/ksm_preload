# Introduction

Enables legacy applications to leverage Linux's memory deduplication.
Only works on Linux ≥ 2.6.32.

If you have multiple processes generating similar data (like multiple
applications serving different vhosts), this easy to use tool may
allow you to spare memory by deduplicating it. If two pages (typically
consecutive blocks of 4k) are identical, they will be merged to reduce
memory usage.

# More

More information, howto, on [vleu.net/ksm_preload](http://vleu.net/ksm_preload/).


# Installing

The quick and dirty way:
```bash
cmake .
make
sudo make install
```