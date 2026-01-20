XMOS Voice Framework Change Log
===============================

UNRELEASED
----------

  * ADDED: `xcommon_cmake` support for all modules
  * ADDED: `lib_stage1` module
  * ADDED: `aec_process_frame()` to `lib_aec`
  * ADDED: `vnr_process_frame()` to `lib_vnr`
  * ADDED: `ic_process_frame()` to `lib_ic`
  * ADDED: AEC memory pool structs (`aec_memory_pool_t` and `aec_shadow_filt_memory_pool_t`) to `lib_aec`
  * CHANGED: All bare metal examples have been moved from `examples/bare-metal` to `examples`
  * CHANGED: All examples have been rewritten to demonstrate the API only and any fileio support has been removed
  * CHANGED: Merged `aec_1_thread` and `aec_2_threads` examples into `aec` example with 2 build configs
  * CHNAGED: Merged `pipeline_alt_arch` and `pipeline_single_threaded` examples into `pipeline` with 2 configs
  * CHANGED: Merged `fwk_voice::vnr::features` and `fwk_voice::vnr::inference` cmake targets into `fwk_voice::vnr`
  * CHANGED: Moved the VNR model into `modules/lib_vnr/python/model/trained_model.tflite`
  * CHANGED: Moved the VNR MEL generation script into `modules/lib_vnr/python/gen_mel_filters.py`
  * CHANGED: `ic_calc_vnr_pred` now only calculates and outputs an input VNR prediction 
  * CHANGED: Updated AGC and Loss Control algorithms
  * CHANGED: Renamed all module top-level headers from `<module>_api.h` to `<module>.h`
  * REMOVED: IC example
  * REMOVED: AGC example
  * REMOVED: Multithreaded pipeline example


0.8.1
-----

  * FIXED: Added back missing documentation
  * CHANGED: Tools version from 15.3.0 to 15.3.1

0.8.0
-----

  * CHANGED: Tools version from 15.2.1 to 15.3.0
  * CHANGED: Updated xmos-ai-tools version from 0.1.8 to 1.3.1
  * CHANGED: Updated lib_xcore_math version from 2.1.1 to 2.4.0

0.7.0
-----

  * CHANGED: Tools version from 15.1.4 to 15.2.1
  * CHANGED: Example builds and docs use Ninja instead of nmake under Windows
  * CHANGED: Update xmos_xmake_toolchain to v1.0.0 from untagged commit
             3a19f0284c66a92dbb9d5adc9d3d5016aac22646

0.6.0
-----

  * CHANGED: Improved documentation style
  * CHANGED: Replace lib_xs3_math with the lib_xcore_math v2.1.1
  * CHANGED: Integrate new version of lib_tflite_micro in VNR module

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

