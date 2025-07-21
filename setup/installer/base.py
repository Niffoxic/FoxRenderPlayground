from abc import ABC, abstractmethod
from typing import Dict, List
from .install_gui import CTkFrame, CTkLabel

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
