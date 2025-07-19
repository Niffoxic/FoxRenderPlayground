from time import sleep
from typing import Dict, List
from installer.install_gui import InstallerApplication, FadeDirection, ContentFrame
import os
import threading

class CompileShader:
    def __init__(self):
        # TODO: Later just set it from cmake or other stuff dc
        self.shader_root_path = "C:\\Users\\niffo\Desktop\\NiffoxicRepo\\FoxRenderEngine\\shaders"
        self.shaders_name = []
        self.shaders_path: Dict[str, str] = {}  # name -> path
        self._populate_shaders()

    def _populate_shaders(self):
        for dirpath, _, filenames in os.walk(self.shader_root_path):
            for file in filenames:
                if file.endswith(".vert") or file.endswith(".frag"):
                    full_path = os.path.join(dirpath, file)
                    self.shaders_name.append(file)
                    self.shaders_path[file] = full_path

    def get_content(self) -> List[str]:
        return self.shaders_name

    def install(self, name: str) -> None:
        sleep(5)

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

        self.things_to_install: Dict[str, CompileShader] = {
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

        # Add scroll content
        for key, val in self.things_to_install.items():
            self.content_frame.add_scroll_label(key)

            for label in val.get_content():
                l = self.content_frame.add_scroll_label("• " + label)

        previous_button = self.content_frame.add_button("previous", callback=self.home_page)
        previous_button.place(relx=0.0, rely=1.0, anchor="sw", x=20, y=-20)

        next_button = self.content_frame.add_button("install", callback=self.install_page)
        next_button.place(relx=1.0, rely=1.0, anchor="se", x=-20, y=-20)

    def install_page(self):
        self.content_frame.clear()
        self.content_frame.add_label("To be installed or compiled", font=("Segoe UI", 18, "bold"))
        scroll_area = self.content_frame.add_scroll_area(height=180)

        for key, val in self.things_to_install.items():
            self.content_frame.add_scroll_label(key)

            for label_name in val.get_content():
                label_widget = self.content_frame.add_scroll_label(f"• {label_name} (pending)")
                self.install_queue.append((val, label_name, label_widget))

        self.finish_button = self.content_frame.add_button("Finish", callback=self.quit)
        self.finish_button.place(relx=1.0, rely=1.0, anchor="se", x=-20, y=-20)
        self.finish_button.configure(state="disabled")  # <== initially disabled

        # Start threaded async installation
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
