@Library('xmos_jenkins_shared_library@v0.16.2') _
getApproval()

pipeline {
  agent none

  parameters {
    booleanParam(name: 'FULL_TEST_OVERRIDE',
                 defaultValue: false,
                 description: 'Force a full test.')
  }
  environment {
    REPO = 'sw_avona'
    VIEW = getViewName(REPO)
    FULL_TEST = """${(params.FULL_TEST_OVERRIDE
                    || env.BRANCH_NAME == 'develop'
                    || env.BRANCH_NAME == 'main') ? 1 : 0}"""
  }
  options {
    skipDefaultCheckout()
  }
  stages {
    stage('xcore.ai executables build') {
      agent {
        label 'x86_64 && brew'
      }
      stages {
        stage('Get view') {
          steps {
            xcorePrepareSandbox("${VIEW}", "${REPO}")
            dir("${REPO}") {
              viewEnv() {
                withVenv {
                  sh "pip install -e ${env.WORKSPACE}/xtagctl"
                  sh "pip install -e ${env.WORKSPACE}/xscope_fileio"
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
            dir("${REPO}/build") {
              viewEnv() {
                withVenv {
                  sh "cmake --version"
                  script {
                      if (env.FULL_TEST == "1") {
                        sh 'cmake -S.. -DCMAKE_TOOLCHAIN_FILE=../etc/xmos_toolchain.cmake -G"Unix Makefiles" -DPython3_FIND_VIRTUALENV="ONLY"'
                      }
                      else {
                        sh 'cmake -S.. -DCMAKE_TOOLCHAIN_FILE=../etc/xmos_toolchain.cmake -G"Unix Makefiles" -DPython3_FIND_VIRTUALENV="ONLY" -DAEC_UNIT_TESTS_SPEEDUP_FACTOR=4'
                      }
                  }
                  sh "make -j4"
                  sh 'rm CMakeCache.txt'
                  sh 'cmake -S.. -DPython3_FIND_VIRTUALENV="ONLY" -DTEST_WAV_AEC_BUILD_CONFIG="1 2 2 10 5"'
                  sh "make -j4"
                }
              }
            }
            dir("${REPO}") {
              stash name: 'cmake_build', includes: 'build/**/*.xe, build/**/conftest.py, build/**/test_wav_aec_c_app'
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
                  sh "pip install -e ${env.WORKSPACE}/xscope_fileio"
                  unstash 'cmake_build'
                }
              }
            }
          }
        }
        stage('Reset XTAGs'){
          steps{
            dir("${REPO}") {
              sh 'rm -f ~/.xtag/acquired' //Hacky but ensure it always works even when previous failed run left lock file present
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
                  sh "python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/aec_1_thread/bin/aec_1_thread_example.xe"
                }
              }
            }
            dir("${REPO}/examples/bare-metal/aec_2_threads") {
              viewEnv() {
                withVenv {
                  sh "python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/aec_2_threads/bin/aec_2_threads_example.xe"
                  // Make sure 1 thread and 2 threads output is bitexact
                  sh "diff output.wav ../aec_1_thread/output.wav"
                }
              }
            }
          }
        }
        stage('Meta Data tests') {
          steps {
            dir("${REPO}/test/lib_meta_data") {
              viewEnv() {
                withVenv {
                  sh "pytest -n 1 --junitxml=pytest_result.xml"
                  junit "pytest_result.xml"
                }
              }
            }
          }
        }
        stage('NS sup_unit_tests') {
          steps {
            dir("${REPO}/build/test/lib_noise_supression/sup_unit_tests") {
              viewEnv() {
                withVenv {
                  sh "pwd"
                  sh "ls -l *"
                  sh "xrun --xscope --id 0 sup_unit_tests.xe"
                  sh "pytest -n 1 --junitxml=pytest_result.xml"

                  junit "pytest_result.xml"
                }
              }
            }
          }
        }
        stage('NS ns_unit_tests') {
          steps {
            dir("${REPO}/test/lib_noise_supression/ns_unit_tests") {
              viewEnv() {
                withVenv {
                  sh "pytest -n 1 --junitxml=pytest_result.xml"
                  junit "pytest_result.xml"
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
        stage('AEC test_delay_estimator') {
          steps {
            dir("${REPO}/test/lib_aec/test_delay_estimator") {
              viewEnv() {
                withVenv {
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
        stage('AEC test_aec_profile') {
          steps {
            dir("${REPO}/test/lib_aec/test_aec_profile") {
              viewEnv() {
                withVenv {
                  sh "pytest -n 1 --junitxml=pytest_result.xml"
                  junit "pytest_result.xml"
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
      }//stages
      post {
        always {
          archiveArtifacts artifacts: "${REPO}/build/**/*", fingerprint: true
          archiveArtifacts artifacts: "${REPO}/test/lib_aec/test_aec_profile/**/aec_prof*.log", fingerprint: true
          archiveArtifacts artifacts: "${REPO}/test/lib_aec/test_aec_profile/**/profile_index_to_tag_mapping.log", fingerprint: true
        }
        //cleanup {
        //  cleanWs()
        //}
      }
    }//stage xcore.ai Verification
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
