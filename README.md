# Text2Sample

## Clone the repo

```
$ git clone https://github.com/cgo435/Text2Sample.git --recurse-submodules
```

## Build the example model

```
$ cp models/model.onnx ort-builder
$ cd ort-builder
$ git clone https://github.com/microsoft/onnxruntime.git
$ python3 -m venv venv
$ source ./venv/bin/activate
$ pip install -r requirements.txt
$ pip install onnx
$ ./convert-model-to-ort.sh model.onnx
$ ./build-mac.sh
```

## Build the plugin

```
$ cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
$ cmake --build build
```
