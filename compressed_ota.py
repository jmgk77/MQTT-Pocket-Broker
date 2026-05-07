import gzip
import shutil
import os

Import("env")


def compressFirmware(source, target, env):
    """ Compress ESP8266 firmware using gzip for 'compressed OTA upload' """
    SOURCE_FILE = str(target[0])
    GZ_FILE = SOURCE_FILE + ".gz"

    print(f"Compressing firmware to {GZ_FILE}...")
    with open(SOURCE_FILE, 'rb') as f_in:
        with gzip.open(GZ_FILE, 'wb') as f_out:
            shutil.copyfileobj(f_in, f_out)

    ORG_FIRMWARE_SIZE = os.stat(SOURCE_FILE).st_size
    GZ_FIRMWARE_SIZE = os.stat(GZ_FILE).st_size

    print("Compression reduced firmware size to {:.0f}% of original (was {} bytes, now {} bytes)".format(
        (GZ_FIRMWARE_SIZE / ORG_FIRMWARE_SIZE) * 100, ORG_FIRMWARE_SIZE, GZ_FIRMWARE_SIZE))


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", compressFirmware)