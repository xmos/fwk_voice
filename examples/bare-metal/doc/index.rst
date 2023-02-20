
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
================

In order to access binary files on the host from the XCore device over xscope, the examples make use of the
xscope_fileio utility, which needs to be installed before running the example application. To install xscope_fileio, run
the following command from the top level `fwk_voice` directory in a terminal where XMOS XTC tools are sourced. Make sure that cmake
build step has been completed prior to this.

::

    pip install -e build/fwk_voice_deps/xscope_fileio/


.. toctree::
   :maxdepth: 3

   aec_1_thread
   aec_2_threads
   vnr
   ic
   agc
   pipeline_single_threaded
   pipeline_multi_threaded
   pipeline_alt_arch
