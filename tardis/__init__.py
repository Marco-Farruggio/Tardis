import subprocess
import sysinfo
import zipfile
import errors
import os

__version__ = "0.0.0"

COMPRESSION_FORMAT = "Zip"
TARDIS_MARKER = b"TARDIS_MARKER"
COMPILER = "gcc"

current_dir = os.path.dirname(os.path.abspath(__file__))

def parse_config(config_filepath: str) -> dict:
    if not os.path.isfile(config_filepath):
        raise RuntimeError(f"Config file not found at path: {config_filepath}")

    result = {}

    with open(config_filepath, "r") as config_file:
        for line in config_file:
            line = line.strip()

            if " = " in line:
                key, value = line.split(" = ", 1)
                result[key.strip()] = value.strip()
    
    return result

def fetch_output_name(config_filepath) -> str:
    try:
        return parse_config(config_filepath)["name"].strip("\"")
    
    except KeyError:
        raise errors.IncompleteConfiguration("name")

def zip_directory(input_dir_path, output_dir_path, compression: bool=True):
        compression_mode = zipfile.ZIP_DEFLATED if compression else None
        print(f"[Tardis] [Build] [Info] Attempting zipping the payload with ZIP_DEFLATED")
        
        try:
            with zipfile.ZipFile(output_dir_path, "w", compression_mode) as zip_file:
                # Walk the directory tree
                for root, _, files in os.walk(input_dir_path):
                    for file in files:
                        file_path = os.path.join(root, file)
                        # Archive name should be relative path inside the zip
                        relative_path = os.path.relpath(file_path, input_dir_path)
                        zip_file.write(file_path, relative_path)
        
            print(f"[Tardis] [Build] Successfully built the payload using zip")

        except Exception as e:
            print(f"[Tardis] [Build] [Error] An unknown error occured during zipping: {e}")

def append_zip_to_exe(zip_path: str, exe_path: str, marker: bytes = TARDIS_MARKER):
        # Read EXE 
        try:
            with open(exe_path, "rb") as exe_file:
                exe_data = exe_file.read()
                print("[Tardis] [Build] Successfully read generated .exe expander")
        
        except FileNotFoundError:
            print(f"[Tardis] [Build] [Error] Unable to locate the EXE at {exe_path}")

        # Read ZIP content
        try:
            with open(zip_path, "rb") as zip_file:
                zip_data = zip_file.read()
                print("[Tardis] [Build] Successfully read the payload zip file")
        
        except FileNotFoundError:
            print(f"[Tardis] [Build] [Error] Unable to locate the payload zip file at {zip_path}")

        # Append marker + zip data
        with open(exe_path, "wb") as exe_file:
            exe_file.write(exe_data)
            exe_file.write(marker)
            exe_file.write(zip_data)
            print("[Tardis] [Build] Successfully appended the zipped directory to the .exe expander")

def run_test_exe(payload_path: str) -> None:
    result = subprocess.run(f"{fetch_output_name(f"{payload_path}\\tardis.config")}.exe")

    if result.returncode == 0:
        print(f"[Tardis] [Testing] Succesfully ran the .exe")
    
    else:
        print(f"[Tardis] [Testing] [Error] Failed to run the .exe: {result.stderr}")

def compile_c_file(source_file: str, output_file: str, payload_path: str):
    result = subprocess.run([COMPILER, os.path.join(current_dir, "expander.c"), "-o", output_file],
                            capture_output=True, text=True)

    if result.returncode == 0:
        print(f"[Tardis] [Build] Successfully compiled expander.c to {fetch_output_name(f"{payload_path}\\tardis.config")}.exe")
    
    else:
        print(f"[Tardis] [Build] [Error] Failed to compile expander.c to {fetch_output_name(f"{payload_path}\\tardis.config")}.exe: {result.stderr}")

def main(payload_path: str, compression: bool=True) -> None:
    print(f"[Tardis] [Info] Operating System: {sysinfo.os_name}")
    print(f"[Tardis] [Info] Operating System Version: {sysinfo.os_version}")
    print(f"[Tardis] [Info] Operating System Release: {sysinfo.os_release}")
    print(f"[Tardis] [Info] Operating System CPU Architecture: {sysinfo.os_cpu_arch}")
    print(f"[Tardis] [Info] Python Version: {sysinfo.py_version}")
    print(f"[Tardis] [Info] Python Bit Architecture: {sysinfo.py_arch[0]}")
    print(f"[Tardis] [Info] Python Executable Architecture: {sysinfo.py_arch[1]}")
    print(f"[Tardis] [Info] Python Compiler: {sysinfo.py_compiler}")
    print(f"[Tardis] [Info] Python Build Version: {sysinfo.py_build[0]}")
    print(f"[Tardis] [Info] Python Build Date: {sysinfo.py_build[1]}")
    print(f"[Tardis] [Info] Tardis Version: {__version__}")
    print(f"[Tardis] [Info] Compression Format: {COMPRESSION_FORMAT}")

    zip_directory(payload_path, "payload.zip", compression)

    compile_c_file("expander.c", f"{fetch_output_name(f"{payload_path}\\tardis.config")}.exe", payload_path)
    
    append_zip_to_exe("payload.zip", f"{fetch_output_name(f"{payload_path}\\tardis.config")}.exe")

    run_test_exe(payload_path)
    
    fetch_output_name(f"{payload_path}\\tardis.config")

if __name__ == "__main__":
    main(payload_path=r"C:\Users\farru\desktop_files\Desktop\Coding\Tardis\test_dir")