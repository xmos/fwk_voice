XMOS Voice Framework Change Log
===============================

0.5.1
-----
  
  * ADDED: Windows documentation
  * REMOVED: VAD module
  * CHANGED: Git hash at which lib_tflite_micro is fetched during CMake FetchContent

0.5.0
-----

  * ADDED: Support for VNR
  * CHANGED: VNR input based IC control system (the API is not backwards compatible)
  * CHANGED: VNR input based AGC in pipeline examples
  * ADDED: Amazon based wake word engine testing in piplines tests

0.4.0
-----

  * CHANGED: Increased ASR AGC amplitude target
  * ADDED: -Os compile option for modules, examples and tests

0.3.0
-----

  * ADDED: Support for VAD.
  * CHANGED: xcore_sdk no longer a submodule of avona.

0.2.0
-----

  * ADDED: Support for IC, NS and ADEC.
  * CHANGED: CMake files cleanup.

0.1.0
-----

  * Initial version with support for AEC and AGC libraries.
