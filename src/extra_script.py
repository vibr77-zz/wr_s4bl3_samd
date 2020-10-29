from os import makedirs
from os.path import isdir, join
Import('env')


# Optional block, only for Teensy
env.AddPostAction(
    "$BUILD_DIR/firmware.hex",
    env.VerboseAction(" ".join([
        "sed", "-i.bak",
        "s/:10040000FFFFFFFFFFFFFFFFFFFFFFFFDEF9FFFF23/:10040000FFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFD/",
        "$BUILD_DIR/firmware.hex"
    ]), "Fixing $BUILD_DIR/firmware.hex secure flash flags"))


def _jlink_cmd_script(env, source):
    build_dir = env.subst("$BUILD_DIR")
    if not isdir(build_dir):
        makedirs(build_dir)
    script_path = join(build_dir, "upload.jlink")
    commands = ["h", "loadbin %s,0x0" % source, "r", "q"]
    with open(script_path, "w") as fp:
        fp.write("\n".join(commands))
    return script_path

env.Replace(
    __jlink_cmd_script=_jlink_cmd_script,
    UPLOADER="/usr/local/bin/JLinkExe",
    UPLOADERFLAGS=[
        "-device", "ATSAMD21G18",
        "-speed", "4000",
        "-if", "swd",
        "-autoconnect", "1"
    ],
    UPLOADCMD='"$UPLOADER" $UPLOADERFLAGS -CommanderScript ${__jlink_cmd_script(__env__, SOURCE)}'
)