class BaseTardisError(Exception):
    """Base error class inherited by Tardis errors."""
    def __init__(self, message: str = "An unknown error occured.") -> None:
        super().__init__(message)
        self.message = message
        
class FileNotFoundError(BaseTardisError):
    def __init__(self, filepath: str):
        super().__init__(f"File not found: {filepath}")
        self.filepath = filepath

class IncompleteConfiguration(BaseTardisError):
    def __init__(self, parameter: str):
        super().__init__(f"Required parameter not found in configuration file: {parameter}")
        self.parameter = parameter