import torch
import os
import torch
import torchaudio as T
from model import UNet
from pathlib import Path
import soundfile as sf

from inference import SDESampling2
from model_classifier_v2 import Classifier
from sde import VpSdeCos

T.set_audio_backend("sox_io")
device = torch.device("cpu")
model_dir = os.getcwd() + "/saved_weights/weights_vp.pt"


def load_ema_weights(model, model_dir):
    checkpoint = torch.load(model_dir, map_location=torch.device("cpu"))
    dic_ema = {}
    for key, tensor in zip(checkpoint["model"].keys(), checkpoint["ema_weights"]):
        dic_ema[key] = tensor
    model.load_state_dict(dic_ema)
    return model


def main():
    output_path = Path("onnx_output")

    model = UNet().to(device)
    model = load_ema_weights(model, model_dir)
    noise = torch.randn(10, 21000, device=device)

    print("Exporting UNET...")
    onnx_export(
        model,
        model_args=(noise, 0.99),
        output_path=output_path / "crash.onnx",
        ordered_input_names=["input", "sigma"],
        output_names=["output"],
        dynamic_axes={"input": {0: "batch_size"}, "output": {0: "batch_size"}},
    )
    print("Finished exporting UNET")

    classifier_dir = os.getcwd() + "/saved_weights/weights_classifier_v2.pt"
    checkpoint_classifier = torch.load(classifier_dir, map_location=torch.device("cpu"))
    classifier = Classifier()
    classifier.load_state_dict(checkpoint_classifier["model"])
    classifier.eval()

    print("Exporting Classifier...")
    onnx_export(
        classifier,
        model_args=(noise, 0.99),
        output_path=output_path / "classifier.onnx",
        ordered_input_names=["audio", "noise_scale"],
        output_names=["output"],
        dynamic_axes={"audio": {0: "batch_size"}, "output": {0: "batch_size"}},
        training=False,
    )

    sde = VpSdeCos()
    sampler = SDESampling2(model, sde)
    example_out = sampler.predict(noise, 10)
    print(classifier(example_out, 0.0))
    sf.write("example.wav", example_out.flatten(), 44100)
    print("Complete!")


def onnx_export(
    model,
    model_args: type,
    output_path: Path,
    ordered_input_names: list,
    output_names,
    dynamic_axes=None,
    training=False,
):
    torch.onnx.export(
        model,  # model being run
        model_args,  # model input (or a tuple for multiple inputs)
        output_path.as_posix(),  # where to save the model (can be a file or file-like object)
        export_params=True,  # store the trained parameter weights inside the model file
        opset_version=15,  # the ONNX version to export the model to
        input_names=ordered_input_names,  # the model's input names
        output_names=output_names,  # the model's output names
        dynamic_axes=dynamic_axes,
        training=torch.onnx.TrainingMode.TRAINING
        if training
        else torch.onnx.TrainingMode.EVAL,
        do_constant_folding=False if training else True,
    )


if __name__ == "__main__":
    main()
