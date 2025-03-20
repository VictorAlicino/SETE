"""Config module for the application."""
import yaml
from configurations.singleton_metaclass import SingletonMeta

# Configuration File
class OpenConfig(metaclass=SingletonMeta):
    """Config class for the application."""
    def __init__(self):
        with open("config.yaml", "r", encoding='utf-8') as stream:
            try:
                self.config = yaml.safe_load(stream)
            except yaml.YAMLError as exc:
                print(exc)
                self.config = None

    def __getitem__(self, key: str):
        return self.config[key]
    
CONFIG = OpenConfig()

