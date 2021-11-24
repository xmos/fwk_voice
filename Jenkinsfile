@Library('xmos_jenkins_shared_library@v0.16.2') _
getApproval()

pipeline {
  agent none

  parameters {
    booleanParam(name: 'FULL_TEST_OVERRIDE',
                 defaultValue: false,
                 description: 'Force a full test.')
    string(name: 'TOOLS_VERSION',
           defaultValue: '15.1.3',
           description: 'The tools version with which to build and test')
  }

  environment {
    REPO = 'sw_avona'
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
      environment {
        TOOLS_PATH = "${WORKSPACE}/get_tools/tools/${params.TOOLS_VERSION}/XMOS/XTC/${params.TOOLS_VERSION}"
      }
      stages {
        stage('Checkout') {
          steps {
            dir("${REPO}") {
              checkout scm
              installPipfile(false)
            }
            dir("xcore_sdk") {
              git branch: "develop", url: "git@github.com:xmos/xcore_sdk"
//              sh "git submodule update --init"
              sh "git submodule update --init modules/lib_xs3_math modules/lib_dsp"
            }
            dir("get_tools") {
              git url: "git@github0.xmos.com:xmos-int/get_tools"
              sh "python get_tools.py " + params.TOOLS_VERSION
            }
          }
        }
        stage('CMake') {
          steps {
            dir("${REPO}/build") {
              toolsEnv(TOOLS_PATH) {
                withVenv {
                  withEnv(["XCORE_SDK_PATH=${WORKSPACE}/xcore_sdk"]) {
                    sh "cmake --version"
                    script {
                        if (env.FULL_TEST == "1") {
                          sh 'cmake -S.. -DCMAKE_TOOLCHAIN_FILE=../etc/xmos_toolchain.cmake -G"Unix Makefiles" -DPython3_FIND_VIRTUALENV=ONLY'
                        }
                        else {
                          sh 'cmake -S.. -DCMAKE_TOOLCHAIN_FILE=../etc/xmos_toolchain.cmake -G"Unix Makefiles" -DPython3_FIND_VIRTUALENV=ONLY -DAEC_UNIT_TESTS_SPEEDUP_FACTOR=4'
                        }
                    }
                    sh "make -j8"
                    sh 'rm CMakeCache.txt'
                    sh 'cmake -S.. -DPython3_FIND_VIRTUALENV="ONLY" -DTEST_WAV_AEC_BUILD_CONFIG="1 2 2 10 5"'
                    sh "make -j8"
                  }
                }
                stash name: 'cmake_build', includes: '**/*.xe, **/conftest.py, **/test_wav_aec_c_app'
              }
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
      environment {
        TOOLS_PATH = "${WORKSPACE}/get_tools/tools/${params.TOOLS_VERSION}/XMOS/XTC/${params.TOOLS_VERSION}"
      }
      stages{
        stage('Checkout') {
          steps {
            dir("${REPO}") {
              checkout scm
              dir("build") {
                unstash 'cmake_build'
              }
            }
            dir("get_tools") {
              git url: "git@github0.xmos.com:xmos-int/get_tools"
              sh "python get_tools.py " + params.TOOLS_VERSION
            }
            dir("xscope_fileio") {
              git url: "git@github.com:xmos/xscope_fileio"
              sh "git checkout v0.3.2"
            }
            dir("audio_test_tools") {
              git url: "git@github.com:xmos/audio_test_tools"
              sh "git checkout v4.5.1"
            }
            installPipfile(false)
            toolsEnv(TOOLS_PATH) {
              withVenv {
                dir("${REPO}") {
                  sh 'pip install -r test_requirements.txt'
                }
              }
            }
          }
        }
        stage('Reset XTAGs'){
          steps{
            sh 'rm -f ~/.xtag/acquired' //Hacky but ensure it always works even when previous failed run left lock file present
            toolsEnv(TOOLS_PATH) {
              withVenv{
                dir("xtagctl") {
                  git url: "git@github0.xmos.com:xmos-int/xtagctl"
                  sh "git checkout v1.5.0"
                }
                sh "pip install -e xtagctl"
                sh "xtagctl reset_all XCORE-AI-EXPLORER"
              }
            }
          }
        }
        stage('Meta Data tests') {
          steps {
            dir("${REPO}/build/test/lib_meta_data") {
              toolsEnv(TOOLS_PATH) {
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
              toolsEnv(TOOLS_PATH) {
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
              toolsEnv(TOOLS_PATH) {
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
              toolsEnv(TOOLS_PATH) {
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
              toolsEnv(TOOLS_PATH) {
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
              toolsEnv(TOOLS_PATH) {
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
        cleanup {
          cleanWs()
        }
      }
    }//stage xcore.ai Verification
  }
}
