Getting Started
===============

Overview
--------

``lib_aec`` is a library which provides functions that can be put together to perform Automatic Echo Cancellation (AEC)
on input mic data using the input reference data to model the room echo characteristics. ``lib_aec`` library functions
make use of functionality provided in ``lib_xs3_math`` to perform DSP operations. For more details refer to
:ref:`aec_overview`.

Repository Structure
--------------------

* ``modules/lib_aec`` - The actual ``lib_aec`` library directory within ``https://github.com/xmos/sw_avona/``. Within ``lib_aec``

  * ``api/`` - Headers containing the public API for ``lib_aec``.
  * ``doc/`` - Library documentation source (for non-embedded documentation) and build directory.
  * ``src/`` - Library source code.


Requirements
------------

``lib_aec`` is included as part of the ``sw_avona`` github repository
and all requirements for cloning and building ``sw_avona`` apply. ``lib_aec`` is compiled as a static library as part of
overall ``sw_avona`` build. It depends on `lib_xs3_math <https://github.com/xmos/lib_xs3_math/>`_.

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

This repo is got as part of the parent ``sw_avona`` repo clone. It is compiled as a static library as part of sw_avona
compilation process.

To include ``lib_aec`` in an application as a static library, the generated ``lib_aec.a`` can then be linked into the
application. Be sure to also add ``lib_aec/api`` as an include directory for the application.

