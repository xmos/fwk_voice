@Library('xmos_jenkins_shared_library@v0.19.0') _
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
            dir("${REPO}") {
              sh "mkdir build"
            }
            // Do x86 versions first because it's hard to glob just for extensionless files
            dir("${REPO}/build") {
              viewEnv() {
                withVenv {
                  sh "cmake --version"
                  sh 'cmake -S.. -DPython3_FIND_VIRTUALENV="ONLY" -DTEST_WAV_ADEC_BUILD_CONFIG="1 2 2 10 5" -DFWK_VOICE_BUILD_TESTS=ON'
                  sh "make -j8"
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
            dir("${REPO}") {
              stash name: 'cmake_build_x86_examples', includes: 'build/**/fwk_voice_example_bare_metal_*'
              // We are archveing the x86 version. Be careful - these have the same file name as the xcore versions but the linker should warn at least in this case
              stash name: 'cmake_build_x86_libs', includes: 'build/**/*.a'
              archiveArtifacts artifacts: "build/**/fwk_voice_example_bare_metal_*", fingerprint: true
              stash name: 'vnr_py_c_feature_compare', includes: 'test/lib_vnr/py_c_feature_compare/build/**'
            }
            // Now do xcore files
            dir("${REPO}/build") {
              viewEnv() {
                withVenv {
                  sh 'rm CMakeCache.txt'
                  script {
                      if (env.FULL_TEST == "1") {
                        sh 'cmake -S.. -DCMAKE_TOOLCHAIN_FILE=../xmos_cmake_toolchain/xs3a.cmake -DPython3_VIRTUALENV_FIND="ONLY" -DFWK_VOICE_BUILD_TESTS=ON'
                      }
                      else {
                        sh 'cmake -S.. -DCMAKE_TOOLCHAIN_FILE=../xmos_cmake_toolchain/xs3a.cmake -DPython3_VIRTUALENV_FIND="ONLY" -DTEST_SPEEDUP_FACTOR=4 -DFWK_VOICE_BUILD_TESTS=ON'
                      }
                  }
                  sh "make -j8"
                }
              }
            }
            dir("${REPO}") {
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
                }
              }
            }
          }
        }
        stage('Make/get bins and libs'){
          steps {
            dir("${REPO}") {
              sh "mkdir build"
            }
            // Build x86 versions locally as we had problems with moving bins and libs over from previous build due to brew
            dir("${REPO}/build") {
              viewEnv() {
                withVenv {
                  sh "cmake --version"
                  sh 'cmake -S.. -DPython3_FIND_VIRTUALENV="ONLY" -DTEST_WAV_ADEC_BUILD_CONFIG="1 2 2 10 5" -DFWK_VOICE_BUILD_TESTS=ON'
                  sh "make -j8"

                  // We need to put this here because it is not fetched until we build
                  sh "pip install -e fwk_voice_deps/xscope_fileio"

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
        /*stage('Examples') {
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
              archiveArtifacts artifacts: "ic_example_output.wav", fingerprint: true
            }
            dir("${REPO}/examples/bare-metal/vad") {
              viewEnv() {
                withVenv {
                  sh "python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/vad/bin/fwk_voice_example_bare_metal_vad.xe"
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
        }*/
        /*stage('VNR test_wav_vnr') {
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
        }*/
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
        /*stage('VNR Python C feature extraction equivalence') {
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
        }*/
        /*stage('NS profile test') {
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
        }*/
        //stage('NS performance tests') {
          //steps {
            //dir("${REPO}/test/lib_ns/compare_c_xc") {
              //copyArtifacts filter: '**/*.xe', fingerprintArtifacts: true, projectName: '../lib_noise_suppression/develop', selector: lastSuccessful()
              //viewEnv() {
                //withVenv {
                  //sh "pytest -n 2 --junitxml=pytest_result.xml"
                  //junit "pytest_result.xml"
                //}
              //}
            //}
          //}
        //}
        /*stage('NS ns_unit_tests') {
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
        }*/
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
