lib_aec
============

Library Info
############

Summary
-------

``lib_aec`` is a library which provides functions that can be put together to perform Automatic Echo Cancellation (AEC)
on input microphone signal by using the input refererence signal to model the room echo characteristics between each
microphone-loudspeaker pair.


Repository Structure
--------------------

* `lib_aec/ <https://github.com/xmos/sw_avona/tree/develop/modules/lib_aec/>`_ - The actual ``lib_aec`` library directory.

  * `api/ <https://github.com/xmos/sw_avona/tree/develop/modules/lib_aec/api/>`_ - Headers containing the public API for
  * ``lib_aec``.
  * `doc/ <https://github.com/xmos/sw_avona/tree/develop/modules/lib_aec/doc/>`_ - Library documentation source (for non-embedded documentation) and build directory.
  * `src/ <https://github.com/xmos/sw_avona/tree/develop/modules/lib_aec/src/>`_ - Library source code.


Requirements
------------

``lib_aec`` is included as part of the ``sw_avona`` github repository and doesn't have any any additional requirements
than the ``sw_avona`` repo.

API Structure
-------------

The API can be categorised into high level and low level functions.

High level API has fewer input arguments and is simpler. It however, provides limited options for scheduling jobs in parallel
across multiple threads. Keeping API simplicity in mind, most of the high level API functions accept a pointer to the AEC state
structure as an input and modify the relevant part of the AEC state. API and example documentation provides more
details about the fields within the state modified when calling a given function. High level API functions allow
2 levels of parallelism:

* Single level of parallelism where for a given function, main and shadow filter processing can happen in parallel.
* Two levels of parallelism where a for a given function, processing across multiple channels as well as main and shadow
filter can be done in parallel.

Low level API has more input arguments but allows more freedom for running in parallel across multiple threads.
Depending on the low level API used, functions can be run in parallel to work over a range of bins or a range of phases.
This API is still a work in progess and will be fully supported in the future.

Getting and Building
####################

This repo is got and built as part of the parent `sw_avona/ <https://github.com/xmos/sw_avona/tree/develop/>` repo clone and build.



