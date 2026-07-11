from pathlib import Path
from typing import Final

import unreal

TEMP_DIRECTORY_NAME: Final[str] = "Temp"
TEMP_BRIDGE_DIRECTORY_NAME: Final[str] = "PythonDataBridge"
INPUT_FILE_PREFIX: Final[str] = "ue_in"
OUTPUT_FILE_PREFIX: Final[str] = "ue_out"
TEMP_FILE_EXTENSION: Final[str] = "tmp"
FILE_ENCODING: Final[str] = "utf-8"


class LudusPythonBridgeIO:
    def __init__(self, script_path_or_identifier: str) -> None:
        self._script_base_name = self._resolve_identifier(script_path_or_identifier)
        self._temp_directory = self._get_temp_directory()

    def read_inputs(self) -> list[str]:
        inputs: list[str] = []
        input_index = 0
        while True:
            input_file_path = self._build_input_file_path(input_index)
            if not input_file_path.exists():
                break
            inputs.append(input_file_path.read_text(encoding=FILE_ENCODING))
            input_index += 1
        return inputs

    def write_outputs(self, outputs: list[str]) -> None:
        self._temp_directory.mkdir(parents=True, exist_ok=True)
        for output_index, output_value in enumerate(outputs):
            output_file_path = self._build_output_file_path(output_index)
            output_file_path.write_text(output_value, encoding=FILE_ENCODING)

    def _get_temp_directory(self) -> Path:
        project_saved_directory = Path(unreal.Paths.project_saved_dir())
        return project_saved_directory / TEMP_DIRECTORY_NAME / TEMP_BRIDGE_DIRECTORY_NAME

    def _resolve_identifier(self, script_path_or_identifier: str) -> str:
        path = Path(script_path_or_identifier)
        has_extension = path.suffix != ""
        has_separator = "/" in script_path_or_identifier or "\\" in script_path_or_identifier
        if has_extension or has_separator:
            return path.stem
        return script_path_or_identifier

    def _build_input_file_path(self, input_index: int) -> Path:
        file_name = f"{INPUT_FILE_PREFIX}_{self._script_base_name}_{input_index}.{TEMP_FILE_EXTENSION}"
        return self._temp_directory / file_name

    def _build_output_file_path(self, output_index: int) -> Path:
        file_name = f"{OUTPUT_FILE_PREFIX}_{self._script_base_name}_{output_index}.{TEMP_FILE_EXTENSION}"
        return self._temp_directory / file_name
