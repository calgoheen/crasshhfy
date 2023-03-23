# CRASSHHFY

## Clone the repo

```
git clone https://github.com/calgoheen/crasshhfy.git --recurse-submodules
```

## Download the models
https://drive.google.com/file/d/1XIOoqXdSG2--scESA7fcVpZMYwt7tc_m/view?usp=share_link

Copy the files in the downloaded directory to `ort-builder`

Directory structure should look like
```
- crasshhfy
    - ort-builder
      - include
        - libs
            - macos-arm64_x86_64
                - onnxruntime.a
        - model
            - classifier.ort.c
            - classifier.ort.h
            - crash.ort.c
            - crash.ort.h
```

## Build the plugin
```
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Creating the Models
### Clone CRASH
`git clone https://github.com/simonrouard/CRASH.git`

### Export the models
#### Install requirements
```
python3 -m venv venv
source ./venv/bin/activate
pip install -r requirements.txt
pip install onnx
```
Then copy the export.py script from this repo to that one. 
Then run
`python export.py`

This will create `crash.onnx` and `classifier.onnx`


### Build models 
1. Create a directory to store the models inside `ort-builder`

    `mkdir ort-builder/onnx-models`

2. Then copy `crash.onnx` and `classifier.onnx` to `ort-builder/onnx-models`


3. 
```
cd ort-builder
git clone https://github.com/microsoft/onnxruntime.git
python3 -m venv venv
source ./venv/bin/activate
pip install -r requirements.txt
pip install onnx
python -m onnxruntime.tools.convert_onnx_models_to_ort onnx-models --enable_type_reduction
python -m bin2c -o ./model/crash.ort onne_models/crash.ort
python -m bin2c -o ./model/crash.ort onne_models/classifier.ort
```
4. Update `./build-mac.sh` such that 
`model.required_operators_and_types.config` becomes `required_operators_and_types.config` (2 places)

5. `./build-mac.sh`
6. Ensure output looks the same as in "Download the models"