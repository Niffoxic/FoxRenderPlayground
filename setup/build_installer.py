from abc import ABC, abstractmethod
from typing import Dict, List
from installer.install_gui import InstallerApplication, FadeDirection, CTkFrame, CTkLabel, CTkComboBox
import os
import threading
import subprocess
import urllib.request
import tempfile
import shutil
import psutil
from time import sleep


def download_with_headers(url: str, dest_path: str):
    req = urllib.request.Request(
        url,
        headers={"User-Agent": "Mozilla/5.0"}  # Pretend we’re a browser
    )
    with urllib.request.urlopen(req) as response, open(dest_path, 'wb') as out_file:
        shutil.copyfileobj(response, out_file)

def read_cmake_paths(path: str = "../resource_paths.txt") -> dict:
    print("Reading cmake paths...")
    print("Current Directory:", os.getcwd())
    print("Target Path (relative):", path)
    print("Target Path (absolute):", os.path.abspath(path))

    if not os.path.exists(path):
        raise FileNotFoundError(f"CMake path file not found: {os.path.abspath(path)}")

    with open(path, "r") as f:
        return dict(line.strip().split("=", 1) for line in f if "=" in line)

class InstallerTask(ABC):
    def __init__(self):
        self.items_to_install: Dict[str, bool] = {} # item name and detected[false for not and true for hell yea]
        self.label = None
        self._populate_items()

    def prepare_for_install(self):
        pass

    def set_label(self, label: CTkLabel):
        self.label = label

    def set_option_widget(self, parent_frame: CTkFrame):
        pass

    @abstractmethod
    def _populate_items(self):
        """Discover and populate the installable items."""
        pass

    @abstractmethod
    def install(self, name: str) -> None:
        """Install/compile/copy the item with this name."""
        pass

    def get_content(self) -> List[str]:
        return list(self.items_to_install.keys())

    def is_detected(self, name: str) -> bool:
        if name not in self.items_to_install:
            return False
        return self.items_to_install[name]

    def _log(self, text: str):
        if self.label:
            self.label.configure(text=text)
        else:
            print(text)

class VulkanInstaller(InstallerTask):
    def __init__(self):
        self.installer_path = ""
        self.versions_to_select_from: List[str] = [
            "1.3.275.0",
            "1.3.296.0",
            "1.4.315.0",
            "1.4.321.1"
        ]
        self.vulkan_version = "1.3.296.0"
        super().__init__()

    def _populate_items(self):
        detected = self._is_vulkan_installed()
        self.items_to_install["Vulkan SDK"] = detected

    def _is_vulkan_installed(self) -> bool:
        return "VULKAN_SDK" in os.environ

    def set_option_widget(self, parent_frame: CTkFrame):
        from customtkinter import CTkLabel, CTkComboBox

        row = CTkFrame(parent_frame, fg_color="transparent")
        row.pack(fill="x", padx=10, pady=2)

        label = CTkLabel(row, text="Vulkan Version:")
        label.pack(side="left")

        combo = CTkComboBox(row, values=self.versions_to_select_from, width=120)
        combo.set(self.vulkan_version)
        combo.pack(side="left", padx=10)

        def on_change(choice):
            self.vulkan_version = choice
            print("Selected Vulkan version:", self.vulkan_version)

        combo.configure(command=on_change)

    def install(self, name: str) -> None:
        if name != "Vulkan SDK":
            return

        if self._is_vulkan_installed():
            self._log("Vulkan SDK is already installed.")
            return

        self._log(f"Installing Vulkan SDK {self.vulkan_version}...")

        try:
            major, minor, *_ = self.vulkan_version.split(".")
            minor = int(minor)

            if int(major) == 1 and minor >= 4:
                url = f"https://sdk.lunarg.com/sdk/download/{self.vulkan_version}/windows/vulkansdk-windows-X64-{self.vulkan_version}.exe"
            else:
                url = f"https://sdk.lunarg.com/sdk/download/{self.vulkan_version}/windows/VulkanSDK-{self.vulkan_version}-Installer.exe"

            install_dir = os.path.join("C:\\Tools")
            os.makedirs(install_dir, exist_ok=True)

            self.installer_path = os.path.join(install_dir, os.path.basename(url))
            self._log(f"Downloading Vulkan installer to {self.installer_path}...")

            if os.path.exists(self.installer_path):
                os.remove(self.installer_path)

            download_with_headers(url, self.installer_path)
            self._log("Download completed.")

            self._log("Running Vulkan installer...")
            subprocess.Popen([self.installer_path])

            self._log("Waiting for Vulkan installer to finish...")
            waited_seconds = 0
            max_wait = 600  # 10 min max

            while waited_seconds < max_wait:
                active = any(
                    "vulkan" in (p.info['name'] or "").lower()
                    for p in psutil.process_iter(['name'])
                )
                if not active and self._is_vulkan_installed():
                    break
                sleep(1)
                waited_seconds += 1

            if self._is_vulkan_installed():
                self.items_to_install["Vulkan SDK"] = True
                self._log("Vulkan SDK installed successfully.")
            else:
                self.items_to_install["Vulkan SDK"] = False
                self._log("⚠Vulkan SDK may require restart to complete installation.")

        except Exception as e:
            self._log(f"Vulkan install failed: {e}")
            self.items_to_install["Vulkan SDK"] = False

        finally:
            if os.path.exists(self.installer_path):
                try:
                    os.remove(self.installer_path)
                except:
                    pass


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
            cmake_config_cmd = ["cmake", "..", f"-DCMAKE_BUILD_TYPE={build_type}"]
            self._log(f"Running: {' '.join(cmake_config_cmd)}")

            result = subprocess.run(cmake_config_cmd, cwd=build_dir, capture_output=True, text=True)
            if result.returncode != 0:
                self._log("CMake configure failed:\n" + result.stderr)
                return
            self._log("CMake configured successfully.")

            # Step 2: Build
            cmake_build_cmd = ["cmake", "--build", ".", "--config", build_type]
            self._log(f"Running: {' '.join(cmake_build_cmd)}")

            result = subprocess.run(cmake_build_cmd, cwd=build_dir, capture_output=True, text=True)
            if result.returncode != 0:
                self._log("CMake build failed:\n" + result.stderr)
                return

            self._log("Project built successfully.")

        except Exception as e:
            self._log(f"Exception during build: {e}")


class CompileShader(InstallerTask):
    def __init__(self):
        self.output_dir = None
        self.cmake_paths = None
        self.shaders_path: Dict[str, str] = {}
        super().__init__()

    def prepare_for_install(self):
        self.cmake_paths = read_cmake_paths()
        self._lazy_populate()
        self.output_dir = os.path.join(self.cmake_paths["OUTPUT_BASE"], "compiled_shaders")
        os.makedirs(self.output_dir, exist_ok=True)

    def set_option_widget(self, parent_frame: CTkFrame):
            pass

    def _populate_items(self):
        pass

    def _lazy_populate(self):
        print("populating shaders")
        self.shader_root_path = os.path.join(self.cmake_paths["PROJECT_SOURCE"], "shaders")
        for dirpath, _, filenames in os.walk(self.shader_root_path):
            for file in filenames:
                if file.endswith(".vert") or file.endswith(".frag"):
                    full_path = os.path.join(dirpath, file)
                    self.items_to_install[file] = False
                    self.shaders_path[file] = full_path

    def install(self, name: str):
        shader_path = self.shaders_path.get(name)
        if not shader_path:
            msg = f"Shader path not found for {name}"
            self._log(msg)
            return

        output_filename = name.replace(".", "-") + ".spv"
        output_path = os.path.join(self.output_dir, output_filename)

        # Get Vulkan SDK env path
        vulkan_sdk = os.environ.get("VULKAN_SDK")
        if not vulkan_sdk:
            msg = "VULKAN_SDK environment variable not set. Vulkan SDK might not be installed."
            self._log(msg)
            return

        glslc_path = os.path.join(vulkan_sdk, "Bin", "glslc.exe")
        if not os.path.exists(glslc_path):
            msg = f"glslc not found at {glslc_path}"
            self._log(msg)
            return
        self._log(f"Compiling {name}...")

        try:
            result = subprocess.run(
                [glslc_path, shader_path, "-o", output_path],
                capture_output=True,
                text=True,
                check=True
            )
            self.items_to_install[name] = True
            msg = f"Compiled {name} to {output_filename}"
            self._log(msg)
        except subprocess.CalledProcessError as e:
            msg = f"Failed to compile {name}:\n{e.stderr}"
            self._log(msg)

class InstallerUI:
    def __init__(self):
        self.install_queue = []
        self.finish_button = None
        self.app_window = InstallerApplication(
            "\t\t\t\tHaki Imbued Installer",
            image_path="img/sword.jpg",
            fade_direction=FadeDirection.LEFT
        )
        self.app_window.iconbitmap("img/luffy-logo.ico")
        self.content_frame = None
        self.left_content = self.app_window.add_section(side="left", weight=1, fg_color="transparent")
        self.left_content.add_graphic("img/left-gif.gif")
        self.content_frame = self.app_window.add_section(side="right", weight=3, fg_color="#2a2a2a")
        self.home_page()

        self.things_to_install: Dict[str, InstallerTask] = {
            "Vulkan SDK": VulkanInstaller(),
            "CMake": CMakeBuilder(),
            "Shaders": CompileShader()
        }
        self.dependencies: Dict[InstallerTask, List[InstallerTask]] = {
            self.things_to_install["CMake"]: [self.things_to_install["Shaders"]]
        }

    def run(self):
        self.app_window.mainloop()

    def home_page(self):
        self.content_frame.clear()
        self.content_frame.add_label("Setup Wizard", font=("Segoe UI", 18, "bold"))
        self.content_frame.add_label(
            "This will walk you through the Haki Imbued Setup...\n• Install dependencies\n• Configure runtime",
            font=("Segoe UI", 14),
            anchor="w"
        )
        btn = self.content_frame.add_button("next", callback=self.search_page)
        btn.place(relx=1.0, rely=1.0, anchor="se", x=-20, y=-20)

    def search_page(self):
        self.content_frame.clear()
        self.content_frame.add_label("To be installed or compiled", font=("Segoe UI", 18, "bold"))
        scroll_area = self.content_frame.add_scroll_area(height=180)

        for key, val in self.things_to_install.items():
            self.content_frame.add_scroll_label(key)

            for label in val.get_content():
                if val.is_detected(label):
                    self.content_frame.add_scroll_label(f"• {label} (detected ✅)")
                else:
                    self.content_frame.add_scroll_label(f"• {label} (not found ❌)")
                    val.set_option_widget(scroll_area)

        previous_button = self.content_frame.add_button("Previous", callback=self.home_page)
        previous_button.place(relx=0.0, rely=1.0, anchor="sw", x=20, y=-20)

        next_button = self.content_frame.add_button("Install", callback=self.install_page)
        next_button.place(relx=1.0, rely=1.0, anchor="se", x=-20, y=-20)

    def install_page(self):
        self.content_frame.clear()
        self.content_frame.add_label("Installing...", font=("Segoe UI", 18, "bold"))
        scroll_area = self.content_frame.add_scroll_area(height=180)

        self.install_queue = []

        for key, val in self.things_to_install.items():
            self.content_frame.add_scroll_label(key)

            for label_name in val.get_content():
                if val.is_detected(label_name):
                    # Already installed — show and skip
                    label_widget = self.content_frame.add_scroll_label(f"• {label_name} (detected ✅)")
                    val.set_label(label_widget)
                else:
                    # Needs install
                    label_widget = self.content_frame.add_scroll_label(f"• {label_name} (pending)")
                    val.set_label(label_widget)
                    self.install_queue.append((val, label_name, label_widget))

        # Finish button (disabled until install completes)
        self.finish_button = self.content_frame.add_button("Finish", callback=self.quit)
        self.finish_button.place(relx=1.0, rely=1.0, anchor="se", x=-20, y=-20)
        self.finish_button.configure(state="disabled")

        self._run_install_thread()

    def _run_install_thread(self):
        def worker():
            for val, label_name, label_widget in self.install_queue:
                self.content_frame.get_frame().after(0, lambda l=label_widget, name=label_name:
                l.configure(text=f"• {name} (installing...)"))
                val.install(label_name)
                self.content_frame.get_frame().after(0, lambda l=label_widget, name=label_name:
                l.configure(text=f"• {name} (installed ✅)"))

                if isinstance(val, CMakeBuilder):
                    self._start_dependents_after_cmake()

            self.content_frame.get_frame().after(0, self._on_install_finished)

        threading.Thread(target=worker, daemon=True).start()

    def _start_dependents_after_cmake(self):
        def dependent_worker():
            cmake = self.things_to_install["CMake"]
            for dependent in self.dependencies.get(cmake, []):
                dependent.prepare_for_install()
                for label_name in dependent.get_content():
                    if not dependent.is_detected(label_name):
                        label_widget = self.content_frame.add_scroll_label(f"• {label_name} (pending)")
                        dependent.set_label(label_widget)

                        self.content_frame.get_frame().after(0, lambda l=label_widget, name=label_name:
                        l.configure(text=f"• {name} (installing...)"))
                        dependent.install(label_name)
                        self.content_frame.get_frame().after(0, lambda l=label_widget, name=label_name:
                        l.configure(text=f"• {name} (installed ✅)"))

        threading.Thread(target=dependent_worker, daemon=True).start()

    def _on_install_finished(self):
        self.finish_button.configure(state="normal")

    def quit(self):
        self.app_window.destroy()

if __name__ == "__main__":
    installer = InstallerUI()
    installer.run()
