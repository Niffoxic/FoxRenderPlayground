from installer.base import InstallerTask
from typing import Dict, List
import threading

from installer.cmake_builder import CMakeBuilder
from installer.install_gui import InstallerApplication, FadeDirection
from installer.vulkan_installer import VulkanInstaller


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
        }
        self.dependencies: Dict[InstallerTask, List[InstallerTask]] = {}

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
