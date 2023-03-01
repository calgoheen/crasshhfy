# Text2Sample
 
## Build a model

Place the .onnx file in ort-builder directory, then do:

```
$ cd ort-builder
$ git clone https://github.com/microsoft/onnxruntime.git
$ python3 -m venv venv
$ source ./venv/bin/activate
$ pip install -r requirements.txt
$ pip install onnx
$ ./convert-model-to-ort.sh <model-name>.onnx
$ ./build-mac.sh
```

## Build the plugin

```
$ cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
$ cmake --build build
```
