from customtkinter import *

from enum import Enum
from PIL import Image, ImageTk, ImageChops, ImageSequence


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

class InstallerApplication(CTk):
    def __init__(self):
        super().__init__()

        self.application_state_layout = {}
        self.application_state = ApplicationState.STARTUP

        self.title("Fox Render Setup")
        self.geometry("816x480")
        self.resizable(False, False)

        self._configure_base_layout()
        self._build_layout()

    def _configure_base_layout(self):
        self._set_background()

        set_appearance_mode("dark")
        set_default_color_theme("blue")

        self.header_frame = CTkFrame(self.root_container, height=80, corner_radius=0, fg_color="transparent")
        self.header_frame.pack(side="top", fill="x")

        # Load original image
        header_bg = Image.open("../img/sword.jpg").convert("RGBA")
        header_bg = header_bg.resize((816, 80), Image.LANCZOS)

        faded_img = fade(header_bg, FadeDirection.LEFT)
        self.header_image = CTkImage(light_image=faded_img, size=(816, 80))

        # Display image and text together
        self.header_image_label = CTkLabel(
            self.header_frame,
            image=self.header_image,
            text="\t\t\t\tHaki Imbued Installer",
            font=("Segoe UI", 24, "bold"),
            compound="center",
            anchor="w",
            text_color="white",
        )
        self.header_image_label.place(x=0, y=0, relwidth=1, relheight=1)

        # --- NEW SECTION ---
        self.content_frame = CTkFrame(self.root_container, fg_color="transparent")
        self.content_frame.pack(fill="both", expand=True)

        self.content_frame.grid_columnconfigure(0, weight=7)
        self.content_frame.grid_columnconfigure(1, weight=13)
        self.content_frame.grid_rowconfigure(0, weight=1)

        self.left_section = CTkFrame(self.content_frame, fg_color="#1f1f1f")
        self.left_section.grid(row=0, column=0, sticky="nsew")

        self.right_section = CTkFrame(self.content_frame, fg_color="#2a2a2a")
        self.right_section.grid(row=0, column=1, sticky="nsew", padx=(5, 3), pady=(5, 3))

        self._load_gif_in_left_section()
        self._load_layout_in_right_section()

    def _set_background(self, image_path="../img/bg.jpg"):
        # Create the root container frame
        self.root_container = CTkFrame(self, fg_color="transparent")
        self.root_container.place(x=0, y=0, relwidth=1, relheight=1)

        # Load and set background image inside the root container
        bg_img = Image.open(image_path).resize((816, 480), Image.LANCZOS)
        self.bg_image = CTkImage(light_image=bg_img, size=(816, 480))

        self.bg_label = CTkLabel(self.root_container, image=self.bg_image, text="")
        self.bg_label.place(x=0, y=0, relwidth=1, relheight=1)

    def _load_gif_in_left_section(self, gif_path="../img/left-gif.gif"):
        self.gif_path = gif_path
        self.after(100, self._prepare_gif_animation)

    def _prepare_gif_animation(self):
        gif = Image.open(self.gif_path)

        # Get actual dimensions of the left section
        width = self.left_section.winfo_width()
        height = self.left_section.winfo_height()

        if width == 1 and height == 1:
            # Not ready yet, try again shortly
            self.after(50, self._prepare_gif_animation)
            return

        # Resize frames to match left section size
        self.gif_frames = [
            ImageTk.PhotoImage(frame.copy().convert("RGBA").resize((width, height), Image.LANCZOS))
            for frame in ImageSequence.Iterator(gif)
        ]

        self.gif_index = 0

        # Make label fill entire section
        self.gif_label = CTkLabel(self.left_section, text="")
        self.gif_label.place(x=0, y=0, relwidth=1, relheight=1)

        self._animate_gif()

    def _animate_gif(self):
        frame = self.gif_frames[self.gif_index]
        self.gif_label.configure(image=frame)
        self.gif_label.image = frame

        self.gif_index = (self.gif_index + 1) % len(self.gif_frames)
        self.after(100, self._animate_gif)  # 80ms delay (~12.5 FPS)

    def _load_layout_in_right_section(self):
        for widget in self.right_section.winfo_children():
            widget.destroy()
        title = CTkLabel(
            self.right_section,
            text="Welcome to the Haki Installer",
            font=("Segoe UI", 20, "bold"),
            text_color="white",
            anchor="w"
        )
        title.pack(padx=20, pady=(30, 10), anchor="w")

        instructions = CTkLabel(
            self.right_section,
            text=(
                "This installer will guide you through the setup process.\n\n"
                "• It will install the required dependencies.\n"
                "• Set up environment paths.\n"
                "• Configure runtime options.\n\n"
                "Click 'Next' to continue."
            ),
            font=("Segoe UI", 14),
            text_color="white",
            justify="left",
            anchor="w"
        )
        instructions.pack(padx=20, pady=(0, 30), fill="x", anchor="w")
        next_button = CTkButton(
            self.right_section,
            text="Next >",
            command=self._on_next_click,
            width=100
        )
        next_button.place(relx=1.0, rely=1.0, x=-20, y=-20, anchor="se")  # bottom right padding

    def _on_next_click(self):
        print("Going to next step...")

    def _build_layout(self):
        pass

    def add_layout(self, state, layout):
        self.application_state_layout[state] = layout


if __name__ == "__main__":
    app = InstallerApplication()
    app.iconbitmap("../img/luffy-logo.ico")
    app.mainloop()
