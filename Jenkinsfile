@Library('xmos_jenkins_shared_library@v0.43.0') _

def runningOn(machine) {
  println "Stage running on:"
  println machine
}

getApproval()

pipeline {
  agent none

  parameters {
    string(
      name: 'TOOLS_VERSION',
      defaultValue: '15.3.1',
      description: 'The XTC tools version'
    )
    string(
      name: 'XMOSDOC_VERSION',
      defaultValue: 'v8.0.0',
      description: 'The xmosdoc version'
    )
    booleanParam(name: 'FULL_TEST_OVERRIDE',
                 defaultValue: false,
                 description: 'Force a full test. This increases the number of iterations/scope in some tests')
    booleanParam(name: 'PIPELINE_FULL_RUN',
                 defaultValue: false,
                 description: 'Enables pipelines characterisation test which takes 5.0hrs by itself. Normally run nightly')
  }
  environment {
    REPO = 'fwk_voice'
    FULL_TEST = """${(params.FULL_TEST_OVERRIDE
                    || env.BRANCH_NAME == 'develop'
                    || env.BRANCH_NAME == 'main'
                    || env.BRANCH_NAME ==~ 'release/.*') ? 1 : 0}"""
    PIPELINE_FULL_RUN = """${params.PIPELINE_FULL_RUN ? 1 : 0}"""
  }
  options {
    skipDefaultCheckout()
    timestamps()
    buildDiscarder(xmosDiscardBuildSettings(onlyArtifacts=false))
  }
  stages {
    stage('Build and Docs') {
      parallel {
        stage('Build Docs') {
          agent {
            label "documentation"
          }
          steps {
            checkout scm
            warnError("Docs") {
              buildDocs()
            }
          }
          post {
            cleanup {
              xcoreCleanSandbox()
            }
          }
        }
        stage('xcore.ai executables build') {
          when {
            expression { !env.GH_LABEL_DOC_ONLY.toBoolean() }
          }
          agent {
            label 'x86_64&&linux'
          }
          stages {
            stage('Get view') {
              steps {
                runningOn(env.NODE_NAME)

                sh "git clone --depth 1 --branch v2.5.2 git@github.com:ThrowTheSwitch/Unity.git"

                dir("${REPO}") {
                  checkout scm
                  sh "git submodule update --init --recursive --jobs 4"

                  // need ai_tools for the build
                  // need numpy to generate aec tests, will get in from ai_tools
                  createVenv(reqFile: "requirements.txt")
                }
              }
            }
            stage('CMake') {
              steps {
                // Do xcore files
                dir("${REPO}/build") {
                  withTools(params.TOOLS_VERSION) {
                    withVenv {
                      script {
                          if (env.FULL_TEST == "1") {
                            sh 'cmake -S.. --toolchain=../xmos_cmake_toolchain/xs3a.cmake -DFWK_VOICE_BUILD_TESTS=ON'
                          }
                          else {
                            sh 'cmake -S.. --toolchain=../xmos_cmake_toolchain/xs3a.cmake -DTEST_SPEEDUP_FACTOR=4 -DFWK_VOICE_BUILD_TESTS=ON'
                          }
                      }
                      sh 'make -j$(nproc)'
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
              xcoreCleanSandbox()
            }
          }
        }
      }
    }
    stage('xcore.ai Verification') {
      when {
        expression { !env.GH_LABEL_DOC_ONLY.toBoolean() }
      }
      agent {
        label 'xcore.ai'
      }
      stages{
        stage('Get View') {
          steps {
            runningOn(env.NODE_NAME)

            sh "git clone --depth 1 --branch v2.5.2 git@github.com:ThrowTheSwitch/Unity.git"
            sh "git clone --depth 1 --branch v3.0.0 git@github0.xmos.com:xmos-int/xtagctl.git"
            sh "git clone --depth 1 --branch v4.6.0 git@github.com:xmos/audio_test_tools.git"
            sh "git clone --depth 1 --branch v1.1.0 git@github.com:xmos/py_voice.git"
            sh "git clone --depth 1 --branch main git@github.com:xmos/amazon_wwe.git"
            sh "git clone --depth 1 --branch master git@github.com:xmos/sensory_sdk.git"

            dir("${REPO}") {
              checkout scm
              sh "git submodule update --init --recursive --jobs 4"

              createVenv(reqFile: "requirements_test.txt")
              withVenv {
                // Note xscope_fileio is fetched by build so install in next stage
                sh "pip install -e ${env.WORKSPACE}/xtagctl"
              }
            }
          }
        }
        stage('Make/get bins and libs'){
          steps {
            dir("${REPO}") {
              withTools(params.TOOLS_VERSION) {
                withVenv {
                  // Build x86 versions locally as we had problems with moving bins and libs over from previous build due to brew
                  dir("build") {
                    sh "cmake --version"
                    sh 'cmake -S.. -DTEST_WAV_ADEC_BUILD_CONFIG="1 2 2 10 5" -DFWK_VOICE_BUILD_TESTS=ON'
                    sh 'make -j$(nproc)'

                    // We need to put this here because it is not fetched until we build
                    sh "pip install -e fwk_voice_deps/xscope_fileio"
                  }
                  // We do this again on the NUCs for verification later, but this just checks we have no build error
                  dir("test/lib_ic/py_c_frame_compare") {
                    sh "python build_ic_frame_proc.py"
                  }
                  // We do this again on the NUCs for verification later, but this just checks we have no build error
                  dir("test/lib_vnr/test_vnr_cffi") {
                    sh "python build_vnr_cffi.py"
                  }
                  dir("test/stage_b") {
                    sh "python build_c_code.py"
                  }
                  // test VNR xcommon_cmake build
                  dir("test/lib_vnr/test_vnr_xccm") {
                    xcoreBuild(archiveBins: false)
                  }
                  // test NS xcommon_cmake build
                  dir("test/lib_ns/test_ns_xccm") {
                    xcoreBuild(archiveBins: false)
                  }
                  // test AGC xcommon_cmake build
                  dir("test/lib_agc/test_agc_xccm") {
                    xcoreBuild(archiveBins: false)
                  }
                  dir("test/lib_aec/test_aec_xccm") {
                    xcoreBuild(archiveBins: false)
                  }
                  dir("test/lib_ic/test_ic_xccm") {
                    xcoreBuild(archiveBins: false)
                  }
                  dir("test/lib_adec/test_stage1_xccm") {
                    xcoreBuild(archiveBins: false)
                  }
                  unstash 'cmake_build_xcore'
                }
              }
            }
          }
        }
        stage('Reset XTAGs'){
          steps{
            dir("${REPO}") {
              sh 'rm -f ~/.xtag/acquired' // Hacky but ensure it always works even when previous failed run left lock file present
              withTools(params.TOOLS_VERSION) {
                withVenv{
                  sh "xtagctl reset_all XCORE-AI-EXPLORER"
                }
              }
            }
          }
        }

        stage('VNR tests') {
          steps {
            catchError(stageResult: 'FAILURE', catchInterruptions: false){
              dir("${REPO}/test/lib_vnr") {
                withTools(params.TOOLS_VERSION) {
                  withVenv {
                    withEnv(["hydra_audio_PATH=/projects/hydra_audio"]) {
                      dir("vnr_unit_tests") {
                        sh "pytest -n 2 --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                      }
                      dir("test_vnr_cffi") {
                        sh "python build_vnr_cffi.py"
                        sh "pytest -n 4 --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                      }
                      dir("test_vnr_profile") {
                        sh "pytest -s --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                      }
                    }
                  }
                }
              }
            }
          }
        }

        stage('NS tests') {
          steps {
            catchError(stageResult: 'FAILURE', catchInterruptions: false){
              dir("${REPO}/test/lib_ns") {
                withTools(params.TOOLS_VERSION) {
                  withVenv {
                    withEnv(["hydra_audio_PATH=/projects/hydra_audio"]) {
                      dir("test_ns_profile"){
                        sh "pytest -n 1 --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                      }
                      dir("compare_c_py"){
                        sh "pytest -n 2 --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                      }
                      dir("ns_unit_tests"){
                        sh "pytest -n 1 --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                      }
                    }
                  }
                }
              }
            }
          }
        }

        stage('IC tests') {
          steps {
            catchError(stageResult: 'FAILURE', catchInterruptions: false){
              dir("${REPO}/test/lib_ic") {
                withTools(params.TOOLS_VERSION) {
                  withVenv {
                    withEnv(["hydra_audio_PATH=/projects/hydra_audio"]) {
                      dir("ic_unit_tests"){
                        sh "pytest -n 2 --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                      }
                      dir("py_c_frame_compare"){
                        sh "python build_ic_frame_proc.py"
                        sh "pytest -s --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                      }
                      dir("test_ic_profile"){
                        sh "pytest -s --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                      }
                      dir("test_ic_spec"){
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
                      dir("characterise_c_py"){
                        // This test compares the suppression performance across angles between model and C implementation
                        // and fails if they differ significantly. It requires that the C implementation run with fixed mu
                        sh "pytest -s --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                        // This script sweeps the y_delay value to find what the optimum suppression is across RT60 and angle.
                        // It's more of a model develpment tool than testing the implementation so not run. It take a few minutes.
                        //sh "python sweep_ic_delay.py"
                      }
                      dir("test_calc_vnr_pred"){
                        // This is a unit test for ic_calc_vnr_pred function.
                        sh "pytest -n 2 --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                      }
                      dir("test_bad_state"){
                        sh "pytest -s --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                      }
                    }
                  }
                }
              }
            }
          }
        }

        stage('Stage B tests') {
          steps {
            catchError(stageResult: 'FAILURE', catchInterruptions: false){
              dir("${REPO}/test/stage_b") {
                withTools(params.TOOLS_VERSION) {
                  withVenv {
                    withEnv(["hydra_audio_PATH=/projects/hydra_audio"]) {
                      sh "pytest -n 1 --junitxml=pytest_result.xml"
                      junit "pytest_result.xml"
                    }
                  }
                }
              }
            }
          }
        }

        stage('ADEC tests') {
          steps {
            catchError(stageResult: 'FAILURE', catchInterruptions: false){
              dir("${REPO}/test/lib_adec") {
                withTools(params.TOOLS_VERSION) {
                  withVenv {
                    withEnv(["hydra_audio_PATH=/projects/hydra_audio"]) {
                      dir("de_unit_tests") {
                        sh "pytest -n 2 --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                      }
                      dir("test_delay_estimator") {
                        sh 'mkdir -p ./input_wavs/'
                        sh 'mkdir -p ./output_files/'
                        sh "pytest -n 2 --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                        sh "python print_stats.py"
                      }
                      dir("test_adec_startup") {
                        sh "pytest -n 2 --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                      }
                      dir("test_adec") {
                        sh "pytest -n 2 --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                      }
                      dir("test_adec_profile") {
                        sh "pytest -n 2 --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                        // Testing bit exactness of the AEC scheduling
                        sh "diff output_1_2_2_10_5.wav output_2_2_2_10_5.wav"
                      }
                    }
                  }
                }
              }
            }
          }
        }

        stage('AEC tests') {
          steps {
            catchError(stageResult: 'FAILURE', catchInterruptions: false){
              dir("${REPO}/test/lib_aec") {
                withTools(params.TOOLS_VERSION) {
                  withVenv {
                    withEnv(["hydra_audio_PATH=/projects/hydra_audio"]) {
                      dir("test_aec_enhancements") {
                        sh "./make_dirs.sh"
                        sh "pytest -n 2 --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                      }
                      dir("aec_unit_tests") {
                        sh "pytest -n 2 --junitxml=pytest_result.xml"
                        junit "pytest_result.xml"
                      }
                      dir("test_aec_spec") {
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
            }
          }
        }

        stage('AGC tests') {
          steps {
            catchError(stageResult: 'FAILURE', catchInterruptions: false){
              dir("${REPO}/test/lib_agc/test_process_frame") {
                withTools(params.TOOLS_VERSION) {
                  withVenv {
                    sh "pytest -n 2 --junitxml=pytest_result.xml"
                    junit "pytest_result.xml"
                  }
                }
              }
            }
          }
        }
        stage('Pipeline tests') {
          steps {
            catchError(stageResult: 'FAILURE', catchInterruptions: false){
              dir("${REPO}/test/pipeline") {
                withEnv(["hydra_audio_PATH=/projects/hydra_audio"]) {
                  withEnv(["PIPELINE_FULL_RUN=${PIPELINE_FULL_RUN}", "SENSORY_PATH=${env.WORKSPACE}/sensory_sdk/", "AMAZON_WWE_PATH=${env.WORKSPACE}/amazon_wwe/"]) {
                    withTools(params.TOOLS_VERSION) {
                      withVenv {
                        echo "PIPELINE_FULL_RUN set as " + env.PIPELINE_FULL_RUN

                        // Note we have 2 xcore targets and we can run x86 threads too. But in case we have only xcore jobs in the config, limit to 4 so we don't timeout waiting for xtags
                        sh "pytest -n 4 --junitxml=pytest_result.xml -vv"
                        junit "pytest_result.xml"
                        sh "python compare_keywords.py results_Avona_aec_ic_ns_agc_prev_arch_xcore.csv results_Avona_aec_ic_ns_agc_prev_arch_python.csv --pass-threshold=1"
                      }
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
              withTools(params.TOOLS_VERSION) {
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
          archiveArtifacts artifacts: "${REPO}/build/**/fwk_voice_example_*", fingerprint: true
          // AEC aretfacts
          archiveArtifacts artifacts: "${REPO}/test/lib_adec/test_adec_profile/**/adec_prof*.log", fingerprint: true
          // IC artefacts
          archiveArtifacts artifacts: "${REPO}/test/lib_ic/test_ic_profile/ic_prof.log", fingerprint: true
          archiveArtifacts artifacts: "${REPO}/test/lib_ic/test_ic_spec/ic_spec_summary.txt", fingerprint: true
          // NS artefacts
          archiveArtifacts artifacts: "${REPO}/test/lib_ns/test_ns_profile/ns_prof.log", fingerprint: true
          // VNR artifacts
          archiveArtifacts artifacts: "${REPO}/test/lib_vnr/test_vnr_profile/*.png", fingerprint: true
          archiveArtifacts artifacts: "${REPO}/test/lib_vnr/test_vnr_profile/vnr_prof.log", fingerprint: true
          // Pipelines tests
          archiveArtifacts artifacts: "${REPO}/test/pipeline/**/results_*.csv", fingerprint: true
          archiveArtifacts artifacts: "${REPO}/test/pipeline/**/results_*.png", fingerprint: true, allowEmptyArchive: true
          archiveArtifacts artifacts: "${REPO}/test/pipeline/keyword_input_*/*.npy", fingerprint: true, allowEmptyArchive: true
        }
        failure {
          // archive wavs on failure only
          archiveArtifacts artifacts: "${REPO}/test/pipeline/keyword_input_*/*.wav", fingerprint: true
        }
        cleanup {
          xcoreCleanSandbox()
        }
      }
    }// stage xcore.ai Verification
  }
}
