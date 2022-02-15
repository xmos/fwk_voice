
####################
Example Applications
####################

Several examples are provided to demonstrate processing of audio using the audio processing algorithms individually as
well as put together in a pipeline.

Building Examples
==================

After configuring the CMake project (with the ``BUILD_EXAMPLES`` enabled), all the examples can
be built by using the ``make`` command within the build directory.  Individual examples can be built
using ``make EXAMPLE_NAME``, where ``EXAMPLE_NAME`` is the example to build. 

Running Examples
==================

In order to access binary files on the host from the XCore device over xscope, the examples make use of the
xscope_fileio utility, which needs to be installed before running the example application. To install xscope_fileio, run
the following command from the `examples/bare-metal/` directory in a terminal where XMOS XTC tools are sourced.

::

    pip install -e shared_src/xscope_fileio/


.. toctree::
   :maxdepth: 1
   
   aec_1_thread
