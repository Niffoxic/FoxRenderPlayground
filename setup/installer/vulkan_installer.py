from .base import InstallerTask
from .install_gui import CTkFrame

from typing import List
import os
import subprocess
import psutil
from time import sleep
import urllib.request
import shutil


def download_with_headers(url: str, dest_path: str):
    req = urllib.request.Request(
        url,
        headers={"User-Agent": "Mozilla/5.0"}  # Pretend we’re a browser
    )
    with urllib.request.urlopen(req) as response, open(dest_path, 'wb') as out_file:
        shutil.copyfileobj(response, out_file)


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