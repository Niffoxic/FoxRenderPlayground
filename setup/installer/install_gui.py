from customtkinter import *
from uuid import uuid4
from enum import Enum
from PIL import Image, ImageTk, ImageChops, ImageSequence
import os


class ApplicationState(Enum):
    STARTUP = 1


class FadeDirection(Enum):
    LEFT    = "left"
    RIGHT   = "right"
    UP      = "up"
    DOWN    = "down"


def fade(image: Image.Image, direction: FadeDirection) -> Image.Image:
    width, height = image.size
    image = image.convert("RGBA")

    # Extract current alpha channel
    existing_alpha = image.getchannel("A")

    # Create new alpha gradient mask
    if direction in (FadeDirection.LEFT, FadeDirection.RIGHT):
        gradient = Image.new("L", (width, 1), color=0xFF)
        for x in range(width):
            value = int(255 * (x / (width - 1))) if direction == FadeDirection.RIGHT else int(255 * (1 - x / (width - 1)))
            gradient.putpixel((x, 0), value)
        mask = gradient.resize((width, height))
    else:
        gradient = Image.new("L", (1, height), color=0xFF)
        for y in range(height):
            value = int(255 * (y / (height - 1))) if direction == FadeDirection.DOWN else int(255 * (1 - y / (height - 1)))
            gradient.putpixel((0, y), value)
        mask = gradient.resize((width, height))

    # Multiply existing alpha with new mask
    combined_alpha = Image.eval(ImageChops.multiply(existing_alpha, mask), lambda x: min(255, x))
    image.putalpha(combined_alpha)

    return image


class HeaderSection:
    def __init__(self, master, title_text="FoxInstaller", image_path=None, fade_direction=None):
        self.master = master
        self.height = 80 # yea m fixing it not gonna invest time on side stuff
        self.title_label = None
        self.header_frame = CTkFrame(self.master, height=self.height, corner_radius=0, fg_color="transparent")
        self.header_frame.pack(side="top", fill="x")
        self.image_path = image_path
        self.header_image = None
        self.title = title_text
        self.fade_direction = fade_direction
        self.reload()

    def reload(self):
        if self.image_path:
            self._load_image_background(self.image_path, self.fade_direction)

        if self.title_label:
            self.set_title(self.title)
        else:
            self._create_title(self.title)

    def _load_image_background(self, path, direction):
        img = Image.open(path).resize((816, self.height), Image.LANCZOS).convert("RGBA")

        if direction:
            img = fade(img, direction)

        self.header_image = CTkImage(light_image=img, size=(816, self.height))

    def _create_title(self, text):
        self.title_label = CTkLabel(
            self.header_frame,
            image=self.header_image,
            text=f"{text}",
            font=("Segoe UI", 24, "bold"),
            compound="center",
            anchor="w",
            text_color="white",
        )
        self.title_label.place(x=0, y=0, relwidth=1)

    def set_title(self, new_title: str):
        self.title_label.configure(text=f"  {new_title}")

    def get_frame(self):
        return self.header_frame


class ContentFrame:
    def __init__(self, frame: CTkFrame):
        self.frame = frame
        self.buttons = {}
        self.labels = list()
        self.checkboxes = []
        self.graphic_label = None
        self.gif_frames = []
        self.gif_index = 0
        self.gif_running = False
        self.scroll_area = None

    def add_scroll_area(self, height=180) -> CTkScrollableFrame:
        if self.scroll_area:
            self.scroll_area.destroy()

        self.scroll_area = CTkScrollableFrame(self.frame, height=height, fg_color="transparent")
        self.scroll_area.pack(fill="both", expand=False, padx=20, pady=10)
        return self.scroll_area

    def add_scroll_label(self, text: str, font=("Segoe UI", 14), anchor="w"):
        if not self.scroll_area:
            self.add_scroll_area()

        label = CTkLabel(self.scroll_area, text=text, font=font, anchor=anchor, justify="left")
        label.pack(anchor=anchor, pady=5)

    def add_scroll_checkbox(self, text: str):
        if not self.scroll_area:
            self.add_scroll_area()

        checkbox = CTkCheckBox(self.scroll_area, text=text)
        checkbox.pack(anchor="w", pady=5)
        self.checkboxes.append(checkbox)

    def add_checkbox(self, text: str, variable=None, command=None) -> CTkCheckBox:
        checkbox_id = str(uuid4())
        checkbox = CTkCheckBox(
            self.frame,
            text=text,
            variable=variable,
            command=command,
            text_color="white"
        )
        checkbox.pack(anchor="w", padx=20, pady=5)
        self.checkboxes.append(checkbox)
        return checkbox

    def add_grid_frame(self, rows=1, cols=2, fg_color="transparent") -> CTkFrame:
        grid_frame = CTkFrame(self.frame, fg_color=fg_color)
        grid_frame.pack(fill="both", expand=True, padx=10, pady=10)

        for r in range(rows):
            grid_frame.grid_rowconfigure(r, weight=1)
        for c in range(cols):
            grid_frame.grid_columnconfigure(c, weight=1)

        return grid_frame

    def add_button(self, text, callback) -> CTkButton:
        button_id = str(uuid4())
        _button = CTkButton(self.frame, text=text, command=callback)
        _button.pack(padx=10, pady=10, anchor="se")
        self.buttons[button_id] = _button
        return _button

    def add_label(self, text: str, font=("Segoe UI", 14), text_color="white",
                 pady=(0, 10), padx=20, anchor="w", justify="left"):
        label = CTkLabel(
            self.frame,
            text=text,
            font=font,
            text_color=text_color,
            anchor=anchor,
            justify=justify
        )
        label.pack(padx=padx, pady=pady, anchor=anchor, fill="x")
        self.labels.append(label)

    def get_button(self, button_id) -> CTkButton:
        return self.buttons.get(button_id)

    def add_graphic(self, path: str):
        ext = os.path.splitext(path)[1].lower()
        if ext == ".gif":
            self._load_gif(path)
        else:
            self._load_static_image(path)

    def _load_static_image(self, path):
        image = Image.open(path).convert("RGBA")
        image = image.resize((self.frame.winfo_width(), self.frame.winfo_height()), Image.Resampling.LANCZOS)
        img = ImageTk.PhotoImage(image)

        if not self.graphic_label:
            self.graphic_label = CTkLabel(self.frame, text="")
            self.graphic_label.place(x=0, y=0, relwidth=1, relheight=1)

        self.graphic_label.configure(image=img)
        self.graphic_label.image = img

    def _load_gif(self, path):
        gif = Image.open(path)

        def _prepare_frames():
            w, h = self.frame.winfo_width(), self.frame.winfo_height()
            if w <= 1 or h <= 1:
                self.frame.after(50, _prepare_frames)
                return

            self.gif_frames = [
                ImageTk.PhotoImage(frame.copy().convert("RGBA").resize((w, h), Image.Resampling.LANCZOS))
                for frame in ImageSequence.Iterator(gif)
            ]
            self.gif_index = 0

            if not self.graphic_label:
                self.graphic_label = CTkLabel(self.frame, text="")
                self.graphic_label.place(x=0, y=0, relwidth=1, relheight=1)

            self.gif_running = True
            self._animate_gif()

        _prepare_frames()

    def _animate_gif(self):
        if not self.gif_running or not self.gif_frames:
            return

        frame = self.gif_frames[self.gif_index]
        self.graphic_label.configure(image=frame)
        self.graphic_label.image = frame

        self.gif_index = (self.gif_index + 1) % len(self.gif_frames)
        self.frame.after(80, self._animate_gif)

    def clear(self):
        for widget in self.frame.winfo_children():
            widget.destroy()
        self.buttons.clear()
        self.labels.clear()
        self.checkboxes.clear()
        self.graphic_label = None
        self.gif_frames = []
        self.gif_index = 0
        self.gif_running = False


class InstallerApplication(CTk):
    def __init__(self, title_text: str, image_path: str, fade_direction: FadeDirection):
        super().__init__()

        self.application_state_layout = {}
        self.application_state = ApplicationState.STARTUP

        self.image_path = image_path
        self.fade_direction = fade_direction
        self.title_text = title_text

        self.title("Fox Render Setup")
        self.geometry("816x480")
        self.resizable(False, False)
        self._configure_base_layout()

    def _configure_base_layout(self):
        # Always use self as master, not self.master
        self.root_container = CTkFrame(self, fg_color="transparent")
        self.root_container.place(x=0, y=0, relwidth=1, relheight=1)

        self.header = HeaderSection(
            master=self.root_container,
            image_path=self.image_path,
            title_text=self.title_text,
            fade_direction=self.fade_direction
        )
        set_appearance_mode("dark")
        set_default_color_theme("blue")
        self._set_body()

    def _set_body(self):
        # Correct master to be root_container instead of self.master
        self.frame = CTkFrame(self.root_container, fg_color="transparent")
        self.frame.pack(fill="both", expand=True)

        # Setup for sections
        self.sections = {}       # section_id -> CTkFrame
        self.section_grid_row = 0

    def add_section(self, side="left", weight=1, fg_color="transparent") -> ContentFrame:
        section_id = str(uuid4())
        col = 0 if side == "left" else 1

        self.frame.grid_columnconfigure(col, weight=weight)
        self.frame.grid_rowconfigure(self.section_grid_row, weight=1)

        section_frame = CTkFrame(self.frame, fg_color=fg_color)
        section_frame.grid(row=self.section_grid_row, column=col, sticky="nsew", padx=5, pady=5)

        # Wrap in ContentFrame
        content_frame = ContentFrame(section_frame)

        self.sections[section_id] = content_frame
        return content_frame

    def get_section(self, section_id: str) -> CTkFrame:
        return self.sections.get(section_id)


if __name__ == "__main__":
    app = InstallerApplication("Imbued Haki Installer", "../img/sword.jpg", FadeDirection.RIGHT)
    app.iconbitmap("../img/luffy-logo.ico")

    gif_content = app.add_section(side="left", weight=1, fg_color="transparent")
    gif_content.add_graphic("../img/left-gif.gif")

    content = app.add_section(side="right", weight=3, fg_color="#2a2a2a")

    content.add_label("Setup Wizard", font=("Segoe UI", 18, "bold"))
    content.add_label(
        "This will walk you through the Haki Imbued Setup...\n• Install dependencies\n• Configure runtime",
        font=("Segoe UI", 14),
        anchor="w"
    )

    def test_next():
        content.clear()  # Clear the right-side section
        content.add_label("Third Phase", font=("Segoe UI", 18, "bold"))
        content.add_label("Now installing dependencies...", font=("Segoe UI", 14), anchor="w")

    def on_next():
        content.clear()
        content.add_label("Second Phase", font=("Segoe UI", 18, "bold"))
        scroll_area = content.add_scroll_area(height=180)
        # Add scroll content
        content.add_scroll_label("Select installation components:")
        content.add_scroll_checkbox("DirectX Runtime")
        content.add_scroll_checkbox("Visual C++ Redistributable")
        content.add_scroll_checkbox("FoxRender Shader Pack")
        content.add_scroll_label("Optional Add-ons:")
        content.add_scroll_checkbox("Install Demo Content")
        content.add_scroll_checkbox("Enable Debug Logs")

        # Next button
        btn_2nd = content.add_button("Next", callback=test_next)
        btn_2nd.place(relx=1.0, rely=1.0, anchor="se", x=-20, y=-20)


    btn = content.add_button("Next", callback=on_next)
    btn.place(relx=1.0, rely=1.0, anchor="se", x=-20, y=-20)
    app.mainloop()
