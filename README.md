Micro Fail2Ban (mf2b)
=====================

Micro Fail2Ban acts as a replacement to the well-known Fail2Ban daemon, but with embedded systems in mind. Therefore it has been written in pure C and doesn't depend on external libraries to be present on the target. Despite the similar concept, it's not a drop-in replacement as configuration syntax aligns more with that of logrotate.

Since the original fail2ban.org requires Python, I considered it to be quite bloaty, especially with use on an embedded device in mind. Micro Fail 2 Ban aims to fix this.

This is a mirror of project [http://sourceforge.net/projects/mf2b](http://sourceforge.net/projects/mf2b).

Building the Source
-------------------

In most cases, a simple 'make' should do the trick. In case the outcome is not
as expected, you may want to have a look at the top-level Makefile for details.

For cross compiling add env variable CROSS_COMPILE (like CROSS_COMPILE=arm-oe-linux-gnueabi-).

Installing the Binaries
-----------------------

Just call 'make install'. In case installation to a specific location is
desired, the Makefile understands the well-known environment variable DESTDIR.

Usage
-----

See man files for instalation and configuration ("man 8 mf2b" and "man 5 mf2b.conf").

License
-------

[GPLv3](LICENSE)
