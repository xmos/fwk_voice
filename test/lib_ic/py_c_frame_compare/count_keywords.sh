sox output.wav -b 16 output_py.wav gain 50 remix 1
sox output.wav -b 16 output_c.wav gain 50 remix 2

echo "Python:"
../../../../sensory_sdk/spot_eval_exe/spot-eval_x86_64-apple-darwin -t ../../../../sensory_sdk/model/spot-alexa-rpi-31000.snsr output_py.wav | wc -l
echo "C:"
../../../../sensory_sdk/spot_eval_exe/spot-eval_x86_64-apple-darwin -t ../../../../sensory_sdk/model/spot-alexa-rpi-31000.snsr output_c.wav  | wc -l
