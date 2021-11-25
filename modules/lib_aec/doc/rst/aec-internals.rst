The AEC comprises ....

Input requirements
..................

The microphone (Y) inputs are assumed to have been filtered to remove significant signals at or near 
the nyquist frequency. This will normally have been done as a part of a PDM to PCM conversion for 
MEMS microphone based operation.  

Simple usage
............

More usage
..........

API
...

Configuration Defines for AEC
'''''''''''''''''''''''''''''

The is configured by means of a separate file, ``aec_config``, in the
application directory. This file is processed by the build system (execute
``lib_aec/src/create_configuration.py`` from the application directory),
and it produces ``.h`` and ``.xc`` files. These files produce a series of
defines that hard-code the structure of the AEC in the source code.

The ``aec_config`` file defines the following 6 parameters by means of simple
equals statements:

**THREADS**

    Sets the number of threads to be used. Should be between 1
    and 8 inclusive.

**MICROPHONES**

     Sets the number of microphone channels to operate on. Typically between
     1 and 8.

**SPEAKERS**

     Sets the number of speaker channels to operate on. Must be 1.

**TAIL**

     Sets the length of the tail to cancel in 15 ms blocks. This must be an
     even number.

**STARTFREQ**

     Sets the first frequency to operate on; should be less than
     **STOPFREQ**. Any frequencies below **STARTFREQ** are wiped, implementing a
     band pass filter in conjunction with **STOPFREQ**

**STOPFREQ**

     Sets the last frequency to operate on; should be less than 256, and more
     than **STARTFREQ**. Any frequencies above **STOPFREQ** are wiped, implementing a
     band pass filter in conjunction with **STARTFREQ**


For example, the ``aec_config`` file may read::
  
    THREADS=2
    MICROPHONES=4
    SPEAKERS=1
    TAIL=10
    STARTFREQ=60
    STOPFREQ=7500

The files generated are ``src/aec_schedule_parameters.h`` and ``.xc``. The
``.h`` file defines, amongst other, the following constants for use by
other parts of the system (note that these parameters should never be
overridden, but always be changed by changing the AEC_config file above):

**AEC_PROC_FRAME_LENGTH_LOG2**

    This defines the size of the frame on which the echo canceller operates.
    The size must be a power of two, and the value specified is the power
    of 2. This defaults to 9, meaning 512-sample input frames. You must
    also define AEC_FFT_SINE_LUT and AEC_FFT_SINE_LUT_HALF appropriately (to
    be changed)

**AEC_Y_CHANNELS**

    This defines the number of near end (microphone) channels to the echo
    canceller.

**AEC_X_CHANNELS**

    This defines the number of far end (speaker) channels to the echo
    canceller.

**AEC_FRAME_ADVANCE**

    This defines the frame advance of the input frame to the echo canceller
    Defaults to an advance of 240 samples.

The ``aec_schedule_paratemers.xc`` file defines a schedule of execution;
that is, it defines the parallelisation of the various tasks in order to
maximise the performance.
    
Creating a AEC instance
'''''''''''''''''''''''

.. doxygenfunction:: aec_init

|newpage|

Controlling an AEC instance
'''''''''''''''''''''''''''

.. doxygenfunction:: aec_set_adaption_config
.. doxygenfunction:: aec_set_mu_scalar
.. doxygenfunction:: aec_set_mu_limits
.. doxygenfunction:: aec_set_sigma_alphas

|newpage|

Processing time domain data
'''''''''''''''''''''''''''

.. doxygenfunction:: aec_process_td_frame
.. doxygenfunction:: aec_frame_adapt

|newpage|
