#!/bin/bash

# Make directories
echo "Creating directories..."
./make_dirs.sh clean
# Build
echo "Building..."
./build.sh
# Generate Audio
echo "Generating Audio..."
if [ -z $1 ]; then
    python generate_audio.py
else
    python generate_audio.py --sub-test $1
fi
# Process Audio
echo "Processing Audio..."
pytest -d -n=auto --junitxml=results_aec.xml test_process_audio.py
# Check Audio
echo "Checking AEC Output..."
pytest -d -n=auto --junitxml=results_check.xml test_check_output.py
# Parse Results
echo "Parsing Results..."
python parse_results.py
# Evaluating Results
echo "Evaluating Results..."
pytest -d -n=auto --junitxml=results_final.xml test_evaluate_results.py
echo "Done"
