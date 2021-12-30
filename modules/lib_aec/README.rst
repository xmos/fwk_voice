lib_aec
============

Library Info
############

Summary
-------

``lib_aec`` is a library which provides functions that can be put together to perform Automatic Echo Cancellation (AEC)
on input microphone signal by using the input refererence signal to model the room echo characteristics between each
microphone-loudspeaker pair. ``lib_aec`` library functions make use of functionality provided in ``lib_xs3_math`` to perform DSP operations.


Repository Structure
--------------------

* `lib_aec/ <https://github.com/xmos/sw_avona/tree/develop/modules/lib_aec/>`_ - The actual ``lib_aec`` library directory.

  * `api/ <https://github.com/xmos/sw_avona/tree/develop/modules/lib_aec/api/>`_ - Headers containing the public API for ``lib_aec``.
  * `doc/ <https://github.com/xmos/sw_avona/tree/develop/modules/lib_aec/doc/>`_ - Library documentation source (for non-embedded documentation) and build directory.
  * `src/ <https://github.com/xmos/sw_avona/tree/develop/modules/lib_aec/src/>`_ - Library source code.


Requirements
------------

``lib_aec`` is included as part of the `sw_avona <https://github.com/xmos/sw_avona/tree/develop/>`_ github repository
and all requirements for cloning and building ``sw_avona`` apply. ``lib_aec`` is compiled as a static library as part of
overall ``sw_avona`` build. It depends on `lib_xs3_math/
<https://github.com/xmos/sw_avona/tree/develop/modules/lib_xs3_math/>`_. 

API Structure
-------------

The API can be categorised into high level and low level functions.

High level API has fewer input arguments and is simpler. However, it provides limited options for calling functions in parallel
across multiple threads. Keeping API simplicity in mind, most of the high level API functions accept a pointer to the AEC state
structure as an input and modify the relevant part of the AEC state. API and example documentation provides more
details about the fields within the state modified when calling a given function. High level API functions allow
2 levels of parallelism:

* Single level of parallelism where for a given function, main and shadow filter processing can happen in parallel.
* Two levels of parallelism where a for a given function, processing across multiple channels as well as main and shadow filter can be done in parallel.

Low level API has more input arguments but allows more freedom for running in parallel across multiple threads. Low
level API function names begin with a ``aec_l2_`` prefix. 
Depending on the low level API used, functions can be run in parallel to work over a range of bins or a range of phases.
This API is still a work in progess and will be fully supported in the future.

Getting and Building
####################

This repo is got as part of the parent `sw_avona <https://github.com/xmos/sw_avona/tree/develop/>`_ repo clone. It is
compiled as a static library as part of ``sw_avona`` compilation process.

To include ``lib_aec`` in an application as a static library, the generated ``lib_aec.a`` can then be linked into the
application. Be sure to also add ``lib_aec/api`` as an include directory for the application.

Building Documentation
----------------------

This project currently uses Doxygen for library and API documentation. API functions include embedded documentation with their declarations in their corresponding header files.

 To build the stand-alone documentation as HTML a Doxygen install will be required. The documentation has been written against Doxygen version 1.8; your mileage may vary with other versions.

With Doxygen on your path, the documentation can be built by calling ``doxygen`` from within the `/lib_aec/doc/ <https://github.com/xmos/sw_avona/tree/develop/modules/lib_aec/doc/>`_ directory.

The documentation will be generated within the ``/lib_aec/doc/build/`` directory. To view the HTML version of the
documentation, open ``/lib_aec/doc/build/html/index.html`` in a browser.


