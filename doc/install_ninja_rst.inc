It is highly recommended to use `Ninja` as the make system under
Windows. Not only has `Ninja` been observed to be faster than MSVC `nmake`, it also
works around an XMOS tools bug where certain path names may cause an issue with
the XMOS compiler under Windows.  This firmware has been tested with `Ninja` version v1.11.1.

To install `Ninja`, follow these steps:

-  Download ``ninja.exe`` from `the Ninja release page <https://github.com/ninja-build/ninja/releases>`_ and save 
   to a suitable location such as ``C:\Program Files\ninja``.
-  Ensure `Ninja` is on the command line path. It may be added to the system path using the following command:
   ``set PATH=%PATH%;C:\Program Files\ninja``. This will work for the lifetime of the current command prompt.
-  The `Ninja` path may be permanently added to the system path using ``setx`` (use with caution) or via the Windows GUI:
   `Control panel -> Advanced system settings -> Environment variables -> System variables -> Path`


The correct installation of `Ninja` can be tested by typing ``ninja`` under an XTC Tools Command Prompt. The following error will be seen verifying that `Ninja` is correctly on the path but run from a directory not containing any document source files::

    ninja: error: loading 'build.ninja': The system cannot find the file specified.

