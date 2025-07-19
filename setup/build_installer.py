from abc import ABC, abstractmethod
from typing import Dict, List

from installer.install_gui import InstallerApplication, FadeDirection, ContentFrame, CTkLabel
import os
import threading
import subprocess
import urllib.request
import tempfile
import shutil
from time import sleep

class InstallerTask(ABC):
    def __init__(self):
        self.items_to_install: Dict[str, bool] = {} # item name and detected[false for not and true for hell yea]
        self.label = None
        self._populate_items()

    def set_label(self, label: CTkLabel):
        self.label = label

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


class CMakeBuilder(InstallerTask):
    def __init__(self):
        super().__init__()

    def _log(self, text: str):
        if self.label:
            self.label.configure(text=text)
        else:
            print(text)

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

        try:
            self._log("🛠️ Building project...")
            os.makedirs(build_dir, exist_ok=True)

            result = subprocess.run(["cmake", ".."], cwd=build_dir, capture_output=True, text=True)
            if result.returncode != 0:
                self._log("CMake configure failed:\n" + result.stderr)
                return

            self._log("CMake configured successfully.")

            result = subprocess.run(["cmake", "--build", "."], cwd=build_dir, capture_output=True, text=True)
            if result.returncode != 0:
                self._log("❌ CMake build failed:\n" + result.stderr)
                return
            self._log("Project built successfully.")

        except Exception as e:
            self._log(f"Error during build: {e}")

class CompileShader(InstallerTask):
    def __init__(self):
        self.shader_root_path = "C:\\Users\\niffo\Desktop\\NiffoxicRepo\\FoxRenderEngine\\shaders"
        self.shaders_path: Dict[str, str] = {}  # name -> path
        super().__init__()

    def _populate_items(self):
        for dirpath, _, filenames in os.walk(self.shader_root_path):
            for file in filenames:
                if file.endswith(".vert") or file.endswith(".frag"):
                    full_path = os.path.join(dirpath, file)
                    self.items_to_install[file] = False
                    self.shaders_path[file] = full_path

    def install(self, name: str):
        print(f"Installing {name} from {self.shaders_path[name]}")
        sleep(2)
        self.items_to_install[name] = True

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
            "CMake": CMakeBuilder(),
            "Shaders": CompileShader()
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
            self.content_frame.get_frame().after(0, self._on_install_finished)

        threading.Thread(target=worker, daemon=True).start()

    def _on_install_finished(self):
        self.finish_button.configure(state="normal")

    def quit(self):
        self.app_window.destroy()

if __name__ == "__main__":
    installer = InstallerUI()
    installer.run()
