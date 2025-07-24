from .base import InstallerTask
from .install_gui import CTkFrame, CTkLabel, CTkComboBox
import subprocess
import os
import urllib.request
import shutil
import tempfile


class CMakeBuilder(InstallerTask):
    def __init__(self):
        self.build_type = "Debug"
        super().__init__()

    def set_option_widget(self, parent_frame: CTkFrame):
        row = CTkFrame(parent_frame, fg_color="transparent")
        row.pack(fill="x", padx=10, pady=2)

        label = CTkLabel(row, text="Build Type:")
        label.pack(side="left")

        combo = CTkComboBox(row, values=["Debug", "Release"], width=100)
        combo.set(self.build_type)
        combo.pack(side="left", padx=10)
        def on_change(choice):
            self.build_type = choice
        combo.configure(command=on_change)

    def _populate_items(self):
        # Always add it so we can "install" (which includes build)
        self.items_to_install["CMake"] = self._is_cmake_installed()
        self.items_to_install["Build"] = False

    def _is_cmake_installed(self) -> bool:
        try:
            subprocess.run(["cmake", "--version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
            return True
        except (FileNotFoundError, subprocess.CalledProcessError):
            return False

    def install(self, name: str) -> None:
        if name == "CMake":
            if not self._is_cmake_installed():
                self._log("CMake not detected. Installing...")
            if not self._download_cmake():
                self._log("Failed to install CMake. Cannot build.")
                return
            else:
                self._log("CMake already installed. Proceeding to build...")

        if name == "Build":
            self._build_project()

    def _download_cmake(self) -> bool:
        url = "https://github.com/Kitware/CMake/releases/download/v4.1.0-rc2/cmake-4.1.0-rc2-windows-x86_64.zip"
        zip_path = os.path.join(tempfile.gettempdir(), "cmake.zip")
        install_dir = os.path.join("C:\\Tools", "CMake")

        try:
            self._log("Downloading CMake portable ZIP...")
            urllib.request.urlretrieve(url, zip_path)

            self._log("Extracting CMake...")
            shutil.unpack_archive(zip_path, install_dir)

            # Add to PATH
            cmake_bin = os.path.join(install_dir, "bin")
            os.environ["PATH"] = cmake_bin + os.pathsep + os.environ["PATH"]

            self.items_to_install["CMake"] = True
            self._log("CMake installed and added to PATH.")
            return True

        except Exception as e:
            self._log(f"Error installing CMake: {e}")
            return False

        finally:
            if os.path.exists(zip_path):
                os.remove(zip_path)

    def _build_project(self):
        project_root = os.path.abspath(os.path.join(os.getcwd(), ".."))
        build_dir = os.path.join(project_root, "build")
        build_type = getattr(self, "build_type", "Debug")

        try:
            self._log("Starting project build...")
            os.makedirs(build_dir, exist_ok=True)

            # Step 1: Configure
            self._log(f"Configuring CMake with build type: {build_type}")
            cmake_config_cmd = [
                "cmake",
                "..",
                "-G", "Visual Studio 17 2022",
                f"-DCMAKE_BUILD_TYPE={build_type}"
            ]
            self._log(f"Running: {' '.join(cmake_config_cmd)}")

            result = subprocess.run(cmake_config_cmd, cwd=build_dir, capture_output=True, text=True)
            if result.returncode != 0:
                self._log("CMake configure failed:\n" + result.stderr)
                return
            self._log("CMake configured successfully.")

            # Step 2: Build shaders
            cmake_build_cmd = [
                "cmake",
                "--build", ".",
                "--config", build_type,
            ]
            self._log(f"Running: {' '.join(cmake_build_cmd)}")

            result = subprocess.run(cmake_build_cmd, cwd=build_dir, capture_output=True, text=True)
            if result.returncode != 0:
                self._log("CMake build failed:\n" + result.stderr)
                return

            self._log("Shader compilation target built successfully.")

        except Exception as e:
            self._log(f"Exception during build: {e}")
