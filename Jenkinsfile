@Library('xmos_jenkins_shared_library@v0.20.0') _
getApproval()

pipeline {
  agent none

  parameters {
    booleanParam(name: 'FULL_TEST_OVERRIDE',
                 defaultValue: false,
                 description: 'Force a full test. This increases the number of iterations/scope in some tests')
    booleanParam(name: 'PIPELINE_FULL_RUN',
                 defaultValue: false,
                 description: 'Enables pipelines characterisation test which takes 5.0hrs by itself. Normally run nightly')
  }
  environment {
    REPO = 'sw_avona'
    VIEW = getViewName(REPO)
    FULL_TEST = """${(params.FULL_TEST_OVERRIDE
                    || env.BRANCH_NAME == 'develop'
                    || env.BRANCH_NAME == 'main'
                    || env.BRANCH_NAME ==~ 'release/.*') ? 1 : 0}"""
    PIPELINE_FULL_RUN = """${params.PIPELINE_FULL_RUN ? 1 : 0}"""
  }
  options {
    skipDefaultCheckout()
    timestamps()
    // On develop discard builds after a certain number else keep forever
    buildDiscarder(logRotator(
        numToKeepStr:         env.BRANCH_NAME ==~ /develop/ ? '50' : '',
        artifactNumToKeepStr: env.BRANCH_NAME ==~ /develop/ ? '50' : ''
    ))
  }
  stages {
    stage('xcore.ai executables build') {
      agent {
        label 'x86_64 && fistrick'
      }
      environment {
        XCORE_SDK_PATH = "${WORKSPACE}/xcore_sdk"
      }
      stages {
        stage('Get view') {
          steps {
            xcorePrepareSandbox("${VIEW}", "${REPO}")
            dir("${REPO}") {
              viewEnv() {
                withVenv {
                  sh "git submodule update --init --recursive --jobs 4"
                }
              }
            }
          }
        }
        stage('CMake') {
          steps {
            // Do xcore files
            dir("${REPO}/build") {
              viewEnv() {
                withVenv {
                  script {
                      if (env.FULL_TEST == "1") {
                        sh 'cmake -S.. -DCMAKE_TOOLCHAIN_FILE=../xmos_cmake_toolchain/xs3a.cmake -DPython3_VIRTUALENV_FIND="ONLY" -DFWK_VOICE_BUILD_TESTS=ON -DFETCHCONTENT_UPDATES_DISCONNECTED=ON'
                      }
                      else {
                        sh 'cmake -S.. -DCMAKE_TOOLCHAIN_FILE=../xmos_cmake_toolchain/xs3a.cmake -DPython3_VIRTUALENV_FIND="ONLY" -DTEST_SPEEDUP_FACTOR=4 -DFWK_VOICE_BUILD_TESTS=ON -DFETCHCONTENT_UPDATES_DISCONNECTED=ON'
                      }
                  }
                  sh "make -j8"
                }
              }
            }
            dir("${REPO}") {
              // Stash all executables and xscope_fileio
              stash name: 'cmake_build_xcore', includes: 'build/**/*.xe, build/**/conftest.py, build/**/xscope_fileio/**'
            }
          }
        }
      }
      post {
        cleanup {
          cleanWs()
        }
      }
    }
    stage('xcore.ai Verification') {
      agent {
        label 'xcore.ai'
      }
      stages{
        stage('Get View') {
          steps {
            xcorePrepareSandbox("${VIEW}", "${REPO}")
            dir("${REPO}") {
              viewEnv() {
                withVenv {
                  sh "git submodule update --init --recursive --jobs 4"

                  // Note xscopefileio is fetched by build so install in next stage
                  sh "pip install -e ${env.WORKSPACE}/xtagctl"
                  // For IC characterisation we need some additional modules
                  sh "pip install pyroomacoustics"
		              // For IC test_bad_state
		              sh "pip install -e ${env.WORKSPACE}/room_acoustic_pipeline"
		              sh "pip install -e ${env.WORKSPACE}/acoustic_performance_tests"
                }
              }
            }
          }
        }
        stage('Make/get bins and libs'){
          steps {
            // Build x86 versions locally as we had problems with moving bins and libs over from previous build due to brew
            dir("${REPO}/build") {
              viewEnv() {
                withVenv {
                  sh "cmake --version"
                  sh 'cmake -S.. -DPython3_FIND_VIRTUALENV="ONLY" -DTEST_WAV_ADEC_BUILD_CONFIG="1 2 2 10 5" -DFWK_VOICE_BUILD_TESTS=ON -DFETCHCONTENT_UPDATES_DISCONNECTED=ON'
                  sh "make -j8"

                  // We need to put this here because it is not fetched until we build
                  sh "pip install -e fwk_voice_deps/xscope_fileio"

                }
              }
            }
            // We do this again on the NUCs for verification later, but this just checks we have no build error
            dir("${REPO}/test/lib_ic/py_c_frame_compare") {
              viewEnv() {
                withVenv {
                  runPython("python build_ic_frame_proc.py")
                }
              }
            }
            // We do this again on the NUCs for verification later, but this just checks we have no build error
            dir("${REPO}/test/lib_vnr/py_c_feature_compare") {
              viewEnv() {
                withVenv {
                  runPython("python build_vnr_feature_extraction.py")
                }
              }
            }
            dir("${REPO}/test/stage_b") {
              viewEnv() {
                withVenv {
                  runPython("python build_c_code.py")
                }
              }
            }
            dir("${REPO}") {
             unstash 'cmake_build_xcore'
            }
          }
        }
        stage('Reset XTAGs'){
          steps{
            dir("${REPO}") {
              sh 'rm -f ~/.xtag/acquired' // Hacky but ensure it always works even when previous failed run left lock file present
              viewEnv() {
                withVenv{
                  sh "pip install -e ${env.WORKSPACE}/xtagctl"
                  sh "xtagctl reset_all XCORE-AI-EXPLORER"
                }
              }
            }
          }
        }
        stage('Examples') {
          steps {
            dir("${REPO}/examples/bare-metal/aec_1_thread") {
              viewEnv() {
                withVenv {
                  sh "python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/aec_1_thread/bin/fwk_voice_example_bare_metal_aec_1_thread.xe --input ../shared_src/test_streams/aec_example_input.wav"
                }
              }
            }
            dir("${REPO}/examples/bare-metal/aec_2_threads") {
              viewEnv() {
                withVenv {
                  sh "python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/aec_2_threads/bin/fwk_voice_example_bare_metal_aec_2_thread.xe --input ../shared_src/test_streams/aec_example_input.wav"
                  // Make sure 1 thread and 2 threads output is bitexact
                  sh "diff output.wav ../aec_1_thread/output.wav"
                }
              }
            }
            dir("${REPO}/examples/bare-metal/ic") {
              viewEnv() {
                withVenv {
                  sh "python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/ic/bin/fwk_voice_example_bare_metal_ic.xe"
                  sh "mv output.wav ic_example_output.wav"
                }
              }
            }
            dir("${REPO}/examples/bare-metal/pipeline_single_threaded") {
              viewEnv() {
                withVenv {
                  sh "python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/pipeline_single_threaded/bin/fwk_voice_example_bare_metal_pipeline_single_thread.xe --input ../shared_src/test_streams/pipeline_example_input.wav"
                }
              }
            }
            dir("${REPO}/examples/bare-metal/pipeline_multi_threaded") {
              viewEnv() {
                withVenv {
                  sh "python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/pipeline_multi_threaded/bin/fwk_voice_example_bare_metal_pipeline_multi_thread.xe --input ../shared_src/test_streams/pipeline_example_input.wav"
                  // Make sure single thread and multi threads pipeline output is bitexact
                  sh "diff output.wav ../pipeline_single_threaded/output.wav"
                }
              }
            }
            dir("${REPO}/examples/bare-metal/pipeline_alt_arch") {
              viewEnv() {
                withVenv {
                  sh "python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/pipeline_alt_arch/bin/fwk_voice_example_bare_metal_pipeline_alt_arch_st.xe --input ../shared_src/test_streams/pipeline_example_input.wav"
                  sh "mv output.wav output_st.wav"

                  sh "python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/pipeline_alt_arch/bin/fwk_voice_example_bare_metal_pipeline_alt_arch_mt.xe --input ../shared_src/test_streams/pipeline_example_input.wav"
                  sh "mv output.wav output_mt.wav"
                  sh "diff output_st.wav output_mt.wav"
                }
              }
            }
            dir("${REPO}/examples/bare-metal/agc") {
              viewEnv() {
                withVenv {
                  sh "python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/agc/bin/fwk_voice_example_bare_metal_agc.xe --input ../shared_src/test_streams/agc_example_input.wav"
                }
              }
            }
            dir("${REPO}/examples/bare-metal/vnr") {
              viewEnv() {
                withVenv {
                  sh "python host_app.py test_stream_1.wav vnr_out2.bin --run-with-xscope-fileio" // With xscope host in lib xscope_fileio
                  sh "python host_app.py test_stream_1.wav vnr_out1.bin" // With xscope host in python
                  sh "diff vnr_out1.bin vnr_out2.bin"
                }
              }
            }
          }
        }
        stage('VNR test_wav_vnr') {
          steps {
            dir("${REPO}/test/lib_vnr/test_wav_vnr") {
              viewEnv() {
                withVenv {
                  withMounts([["projects", "projects/hydra_audio", "hydra_audio_vnr_tests"]]) {
                    withEnv(["hydra_audio_PATH=$hydra_audio_vnr_tests_PATH"]) {
                        sh "pytest -n 1 --junitxml=pytest_result.xml"
                    }
                  }
                }
              }
            }
          }
        }
        stage('VNR vnr_unit_tests') {
          steps {
            dir("${REPO}/test/lib_vnr/vnr_unit_tests") {
              viewEnv() {
                withVenv {
                    sh "pytest -n 2 --junitxml=pytest_result.xml"
                }
              }
            }
          }
        }
        stage('VNR Python C feature extraction equivalence') {
          steps {
            dir("${REPO}/test/lib_vnr/py_c_feature_compare") {
              viewEnv() {
                withVenv {
                  runPython("python build_vnr_feature_extraction.py")
                  sh "pytest -s --junitxml=pytest_result.xml"
                  junit "pytest_result.xml"
                }
              }
            }
          }
        }
        stage('NS profile test') {
          steps {
            dir("${REPO}/test/lib_ns/test_ns_profile") {
              viewEnv() {
                withVenv {
                  sh "pytest -n 1 --junitxml=pytest_result.xml"
                  junit "pytest_result.xml"
                }
              }
            }
          }
        }
        stage('NS performance tests') {
          steps {
            dir("${REPO}/test/lib_ns/compare_c_xc") {
              copyArtifacts filter: '**/*.xe', fingerprintArtifacts: true, projectName: '../lib_noise_suppression/develop', selector: lastSuccessful()
              viewEnv() {
                withVenv {
                  sh "pytest -n 2 --junitxml=pytest_result.xml"
                  junit "pytest_result.xml"
                }
              }
            }
          }
        }
        stage('NS ns_unit_tests') {
          steps {
            dir("${REPO}/test/lib_ns/ns_unit_tests") {
              viewEnv() {
                withVenv {
                  sh "pytest -n 1 --junitxml=pytest_result.xml"
                  junit "pytest_result.xml"
                }
              }
            }
          }
        }
        stage('IC ic_unit_tests') {
          steps {
            dir("${REPO}/test/lib_ic/ic_unit_tests") {
              viewEnv() {
                withVenv {
                  sh "pytest -n 2 --junitxml=pytest_result.xml"
                  junit "pytest_result.xml"
                }
              }
            }
          }
        }
        stage('IC Python C equivalence') {
          steps {
            dir("${REPO}/test/lib_ic/py_c_frame_compare") {
              viewEnv() {
                withVenv {
                  runPython("python build_ic_frame_proc.py")
                  sh "pytest -s --junitxml=pytest_result.xml"
                  junit "pytest_result.xml"
                }
              }
            }
          }
        }
        stage('IC test profile') {
          steps {
            dir("${REPO}/test/lib_ic/test_ic_profile") {
              viewEnv() {
                withVenv {
                  sh "pytest --junitxml=pytest_result.xml"
                  junit "pytest_result.xml"
                }
              }
            }
          }
        }
        stage('IC test specification') {
          steps {
            dir("${REPO}/test/lib_ic/test_ic_spec") {
              viewEnv() {
                withVenv {
                  // This test compares the model and C implementation over a range of scenarious for:
                  // convergence_time, db_suppression, maximum noise added to input (to test for stability)
                  // and expected group delay. It will fail if these are not met.
                  sh "pytest -n 2 --junitxml=pytest_result.xml"
                  junit "pytest_result.xml"
                  sh "python print_stats.py > ic_spec_summary.txt"
                  // This script generates a number of polar plots of attenuation vs null point angle vs freq
                  // It currently only uses the python model to do this. It takes about 40 mins for all plots
                  // and generates a series of IC_performance_xxxHz.svg files which could be archived
                  //sh "python plot_ic.py"
                }
              }
            }
          }
        }
        stage('IC characterisation') {
          steps {
            dir("${REPO}/test/lib_ic/characterise_c_py") {
              viewEnv() {
                withVenv {
                  // This test compares the suppression performance across angles between model and C implementation
                  // and fails if they differ significantly. It requires that the C implementation run with fixed mu
                  sh "pytest -s --junitxml=pytest_result.xml" // -n 2 fails often so run single threaded and also print result
                  junit "pytest_result.xml"
                  // This script sweeps the y_delay value to find what the optimum suppression is across RT60 and angle.
                  // It's more of a model develpment tool than testing the implementation so not run. It take a few minutes.
                  //sh "python sweep_ic_delay.py"
                }
              }
            }
          }
        }
        stage('IC test_calc_vnr_pred') {
          steps {
            dir("${REPO}/test/lib_ic/test_calc_vnr_pred") {
              viewEnv() {
                withVenv {
                  // This is a unit test for ic_calc_vnr_pred function.
                  // It compares the avona output with py_ic model output
                  sh "pytest -n1 --junitxml=pytest_result.xml"
                }
              }
            }
          }
        }
        stage('IC test_bad_state') {
          steps {
            dir("${REPO}/test/lib_ic/test_bad_state") {
              viewEnv() {
                withVenv {
                  withMounts([["projects", "projects/hydra_audio", "hydra_audio_bad_state"]]) {
                    withEnv(["hydra_audio_PATH=$hydra_audio_bad_state_PATH", "sensory_PATH=sensory_sdk"]) {
                      sh "pytest -s --junitxml=pytest_result.xml"
                      junit "pytest_result.xml"
                    }
                  }
                }
              }
            }
          }
        }
        stage('Stage B tests') {
          steps {
            dir("${REPO}/test/stage_b") {
              viewEnv() {
                withVenv {
                  withMounts([["projects", "projects/hydra_audio", "hydra_audio_stage_b_tests"]]) {
                    withEnv(["hydra_audio_PATH=$hydra_audio_stage_b_tests_PATH"]) {
                      sh "pytest -n1"
                    }
                  }
                }
              }
            }
          }
        }
        stage('ADEC de_unit_tests') {
          steps {
            dir("${REPO}/test/lib_adec/de_unit_tests") {
              viewEnv() {
                withVenv {
                  sh "pytest -n 2 --junitxml=pytest_result.xml"
                  junit "pytest_result.xml"
                }
              }
            }
          }
        }
        stage('ADEC test_delay_estimator') {
          steps {
            dir("${REPO}/test/lib_adec/test_delay_estimator") {
              viewEnv() {
                withVenv {
                  withMounts([["projects", "projects/hydra_audio", "hydra_audio_test_de"]]) {
                    withEnv(["hydra_audio_PATH=$hydra_audio_test_de_PATH"]) {
                      sh 'mkdir -p ./input_wavs/'
                      sh 'mkdir -p ./output_files/'
                      sh "pytest -n 2 --junitxml=pytest_result.xml"
                      junit "pytest_result.xml"
                      runPython("python print_stats.py")
                    }
                  }
                }
              }
            }
          }
        }
        stage('ADEC Initial DE startup time test') {
          steps {
            dir("${REPO}/test/lib_adec/test_adec_startup") {
              viewEnv() {
                withVenv {
                  withMounts([["projects", "projects/hydra_audio", "hydra_audio_test_de"]]) {
                    withEnv(["hydra_audio_PATH=$hydra_audio_test_de_PATH"]) {
                      sh "pytest -n 2 --junitxml=pytest_result.xml"
                    }
                  }
                }
              }
            }
          }
        }
        stage('ADEC test_adec') {
          steps {
            dir("${REPO}/test/lib_adec/test_adec") {
              viewEnv() {
                withVenv {
                  withMounts([["projects", "projects/hydra_audio", "hydra_audio_adec_tests"]]) {
                    withEnv(["hydra_audio_PATH=$hydra_audio_adec_tests_PATH"]) {
                      sh "pytest -n 2 --junitxml=pytest_result.xml"
                      junit "pytest_result.xml"
                    }
                  }
                }
              }
            }
          }
        }
        stage('ADEC test_adec_profile') {
          steps {
            dir("${REPO}/test/lib_adec/test_adec_profile") {
              viewEnv() {
                withVenv {
                  withMounts([["projects", "projects/hydra_audio", "hydra_audio_adec_tests"]]) {
                    withEnv(["hydra_audio_PATH=$hydra_audio_adec_tests_PATH"]) {
                      sh "pytest -n 1 --junitxml=pytest_result.xml"
                      junit "pytest_result.xml"
                    }
                  }
                }
              }
            }
          }
        }
        stage('AEC test_aec_enhancements') {
          steps {
            dir("${REPO}/test/lib_aec/test_aec_enhancements") {
              viewEnv() {
                withVenv {
                  withMounts([["projects", "projects/hydra_audio", "hydra_audio_test_skype"]]) {
                    withEnv(["hydra_audio_PATH=$hydra_audio_test_skype_PATH"]) {
                      sh "./make_dirs.sh"
                      sh "pytest -n 2 --junitxml=pytest_result.xml"
                      junit "pytest_result.xml"
                    }
                  }
                }
              }
            }
          }
        }
        stage('AEC aec_unit_tests') {
          steps {
            dir("${REPO}/test/lib_aec/aec_unit_tests") {
              viewEnv() {
                withVenv {
                  sh "pytest -n 2 --junitxml=pytest_result.xml"
                  junit "pytest_result.xml"
                }
              }
            }
          }
        }
        stage('AEC test_aec_spec') {
          steps {
            dir("${REPO}/test/lib_aec/test_aec_spec") {
              viewEnv {
                withVenv {
                  sh "./make_dirs.sh"
                  script {
                    if (env.FULL_TEST == "0") {
                      sh 'mv excluded_tests_quick.txt excluded_tests.txt'
                    }
                  }
                  sh "python generate_audio.py"
                  sh "pytest -n 2 --junitxml=pytest_result.xml test_process_audio.py"
                  sh "cp pytest_result.xml results_process.xml"
                  catchError {
                    sh "pytest --junitxml=pytest_result.xml test_check_output.py"
                  }
                  sh "cp pytest_result.xml results_check.xml"
                  sh "python parse_results.py"
                  sh "pytest --junitxml=pytest_results.xml test_evaluate_results.py"
                  sh "cp pytest_result.xml results_final.xml"
                  junit "results_final.xml"
                }
              }
            }
          }
        }
        stage('AGC tests') {
          steps {
            dir("${REPO}/test/lib_agc/test_process_frame") {
              viewEnv() {
                withVenv {
                  sh "pytest -n 2 --junitxml=pytest_result.xml"
                  junit "pytest_result.xml"
                }
              }
            }
          }
        }
        stage('HPF test') {
          steps {
            dir("${REPO}/test/test_hpf") {
              viewEnv() {
                withVenv {
                  sh "pytest --junitxml=pytest_result.xml"
                  junit "pytest_result.xml"
                }
              }
            }
          }
        }
        stage('Pipeline tests') {
          steps {
            dir("${REPO}/test/pipeline") {
              withMounts(["projects", "projects/hydra_audio", "hydra_audio_pipeline_sim"]) {
                withEnv(["PIPELINE_FULL_RUN=${PIPELINE_FULL_RUN}", "SENSORY_PATH=${env.WORKSPACE}/sensory_sdk/", "AMAZON_WWE_PATH=${env.WORKSPACE}/amazon_wwe/", "hydra_audio_PATH=$hydra_audio_pipeline_sim_PATH"]) {
                  viewEnv {
                    withVenv {
                      echo "PIPELINE_FULL_RUN set as " + env.PIPELINE_FULL_RUN

                      // Note we have 2 xcore targets and we can run x86 threads too. But in case we have only xcore jobs in the config, limit to 4 so we don't timeout waiting for xtags
                      sh "pytest -n 4 --junitxml=pytest_result.xml -vv"
                      junit "pytest_result.xml"
                      sh "python compare_keywords.py results_Avona_aec_ic_prev_arch_xcore.csv results_Avona_aec_ic_prev_arch_python.csv --pass-threshold=1"
                    }
                  }
                }
              }
            }
          }
        }
        stage('Benchmark Pipeline test results') {
          when {
            expression { env.PIPELINE_FULL_RUN == "1" }
          }
          steps {
            dir("${REPO}/test/pipeline") {
              viewEnv {
                withVenv {
                  copyArtifacts filter: '**/results_*.csv', fingerprintArtifacts: true, projectName: '../lib_audio_pipelines/master', selector: lastSuccessful()
                  runPython("python plot_results.py lib_audio_pipelines/tests/pipelines/results_lib_ap_prev_arch_xcore.csv results_Avona_prev_arch_xcore.csv --single-plot --ww-column='0_2 1_2' --figname=results_benchmark_prev_arch")
                  runPython("python plot_results.py lib_audio_pipelines/tests/pipelines/results_lib_ap_alt_arch_xcore.csv results_Avona_alt_arch_xcore.csv --single-plot --ww-column='0_2 1_2' --figname=results_benchmark_alt_arch")                    
                }
              }
            }
          }
        }
      }// stages
      post {
        always {
          // Examples artifacts
          archiveArtifacts artifacts: "${REPO}/build/**/fwk_voice_example_bare_metal_*", fingerprint: true
          // AEC aretfacts
          archiveArtifacts artifacts: "${REPO}/test/lib_adec/test_adec_profile/**/adec_prof*.log", fingerprint: true
          // IC artefacts
          archiveArtifacts artifacts: "${REPO}/test/lib_ic/test_ic_profile/ic_prof.log", fingerprint: true
          archiveArtifacts artifacts: "${REPO}/test/lib_ic/test_ic_spec/ic_spec_summary.txt", fingerprint: true
          // NS artefacts
          archiveArtifacts artifacts: "${REPO}/test/lib_ns/test_ns_profile/ns_prof.log", fingerprint: true
          // VNR artifacts
          archiveArtifacts artifacts: "${REPO}/test/lib_vnr/test_wav_vnr/*.png", fingerprint: true
          archiveArtifacts artifacts: "${REPO}/test/lib_vnr/test_wav_vnr/*.csv", fingerprint: true
          archiveArtifacts artifacts: "${REPO}/examples/bare-metal/vnr/*.png", fingerprint: true
          archiveArtifacts artifacts: "${REPO}/examples/bare-metal/vnr/vnr_prof.log", fingerprint: true
          // Pipelines tests
          archiveArtifacts artifacts: "${REPO}/test/pipeline/**/results_*.csv", fingerprint: true
          archiveArtifacts artifacts: "${REPO}/test/pipeline/**/results_*.png", fingerprint: true, allowEmptyArchive: true
          archiveArtifacts artifacts: "${REPO}/test/pipeline/keyword_input_*/*.wav", fingerprint: true
          archiveArtifacts artifacts: "${REPO}/test/pipeline/keyword_input_*/*.npy", fingerprint: true, allowEmptyArchive: true
        }
        cleanup {
          cleanWs()
        }
      }
    }// stage xcore.ai Verification
    stage('Update view files') {
      agent {
        label 'x86_64&&brew'
      }
      when {
        expression { return currentBuild.currentResult == "SUCCESS" }
      }
      steps {
        updateViewfiles()
      }
    }
  }
}
