import os
import shutil

Import("env")

def copy_firmware_to_root(source, target, env):
    """Copy firmware bin file to project root."""
    project_dir = env.get("PROJECT_DIR")
    firmware_name = os.path.basename(target[0].get_path())
    firmware_path = os.path.join(project_dir, firmware_name)

    # Check for compressed version
    gz_src = target[0].path + ".gz"
    if os.path.exists(gz_src):
        gz_dest = firmware_path + ".gz"
        print(f"Copying {gz_src} to {gz_dest}")
        shutil.copy(gz_src, gz_dest)
    
    print("Firmware assets copied to project root.")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_firmware_to_root)