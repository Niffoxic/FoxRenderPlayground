import os
import subprocess
from pathlib import Path
import argparse

class ShaderBatchCompiler:
    def __init__(self):
        self.input_dir = None
        self.output_dir = None
        self.supported_extensions = [".vert", ".frag"]
        self.vulkan_sdk_path = None
        self.glslc_path = None
        super().__init__()

    def set_input_path(self, input_path: str):
        self.input_dir = Path(input_path).resolve()

    def set_output_path(self, output_path: str):
        self.output_dir = Path(output_path).resolve()

    def set_vulkan_sdk_path(self, vulkan_sdk_path: str):
        self.vulkan_sdk_path = Path(vulkan_sdk_path).resolve()

        bin_path = self.vulkan_sdk_path / "Bin" / "glslc.exe"
        bin32_path = self.vulkan_sdk_path / "Bin32" / "glslc.exe"

        if bin_path.exists():
            self.glslc_path = bin_path
        elif bin32_path.exists():
            self.glslc_path = bin32_path
        else:
            self.glslc_path = None
            self.log("glslc.exe not found in either Bin or Bin32. Vulkan SDK might be invalid.")

    def log(self, message: str):
        print(f"[ShaderBatchCompiler] {message}")

    def is_shader_file(self, path: Path):
        return path.suffix in self.supported_extensions

    def compile_all_shaders(self):
        if not self.input_dir.exists():
            self.log(f"Input directory does not exist: {self.input_dir}")
            return False

        if not self.glslc_path.exists():
            self.log(f"glslc not found at: {self.glslc_path}")
            return False

        success = True
        for file in self.input_dir.rglob("*"):
            if file.is_file() and self.is_shader_file(file):
                rel_path = file.relative_to(self.input_dir)
                out_filename = rel_path.name.replace(".", "-") + ".spv"
                out_path = self.output_dir / rel_path.parent / out_filename
                os.makedirs(out_path.parent, exist_ok=True)
                if not self.compile_shader(file, out_path):
                    success = False

        return success

    def compile_shader(self, shader_path: Path, output_path: Path):
        self.log(f"Compiling: {shader_path} to {output_path}")
        try:
            subprocess.run(
                [str(self.glslc_path), str(shader_path), "-o", str(output_path)],
                capture_output=True,
                text=True,
                check=True
            )
            self.log(f"Compiled: {output_path.name}")
            return True
        except subprocess.CalledProcessError as e:
            self.log(f"Failed to compile {shader_path.name}:\n{e.stderr}")
            return False

def main():
    parser = argparse.ArgumentParser(description="Batch compile Vulkan GLSL shaders (.vert/.frag) to SPIR-V.")
    parser.add_argument("--input", default="../shaders", help="Input folder containing shaders.")
    parser.add_argument("--output", default="../compiled_shaders", help="Output folder for compiled SPIR-V files.")

    default_vulkan_sdk = os.environ.get("VULKAN_SDK")
    parser.add_argument(
        "--vulkan",
        required=default_vulkan_sdk is None,
        default=default_vulkan_sdk,
        help="Path to Vulkan SDK root directory. Defaults to the VULKAN_SDK environment variable."
    )

    args = parser.parse_args()

    compiler = ShaderBatchCompiler()
    compiler.set_vulkan_sdk_path(args.vulkan)
    compiler.set_input_path(args.input)
    compiler.set_output_path(args.output)
    success = compiler.compile_all_shaders()

    if not success:
        print("Some shaders failed to compile.")
    else:
        print("All shaders compiled successfully.")


if __name__ == "__main__":
    main()
