# SPDX-FileCopyrightText: Copyright 2024, 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>

import os
import subprocess
import sys
import tempfile
from os import path
from dataclasses import dataclass
from platform import uname
from typing import Optional
import shlex


@dataclass
class FVP:
    model_exe: str
    tarmac_plugin: str
    crypto_plugin: str
    cmdline_param: str
    stderr_param: Optional[str] = None


MODELS = {
    "corstone-320": {
        "x86": FVP(
            "Corstone-320_11.32.cs320_3/bin/FVP_Corstone_SSE-320",
            "Corstone-320_11.32.cs320_3/plugins/TarmacTrace.so",
            "Corstone-320_11.32.cs320_3/plugins/Crypto.so",
            "mps4_board.subsystem.cpu0.semihosting-cmd_line",
        ),
        "aarch64": FVP(
            "Corstone-320_11.32.cs320_3/bin/FVP_Corstone_SSE-320",
            "Corstone-320_11.32.cs320_3/plugins/TarmacTrace.so",
            "Corstone-320_11.32.cs320_3/plugins/Crypto.so",
            "mps4_board.subsystem.cpu0.semihosting-cmd_line",
        ),
    },
    "aem-a": {
        "x86": FVP(
            "FVP_Base_RevC_AEMvA_11.31_28/bin/FVP_Base_RevC-2xAEMvA",
            "FVP_Base_RevC_AEMvA_11.31_28/plugins/TarmacTrace.so",
            "FastModels_11.31_28/plugins/Linux64_GCC-12.3/Crypto.so",
            "cluster0.cpu0.semihosting-cmd_line",
            "cluster0.cpu0.semihosting-use_stderr",
        ),
        "aarch64": FVP(
            "FVP_Base_RevC_AEMvA_11.31_28/bin/FVP_Base_RevC-2xAEMvA",
            "FVP_Base_RevC_AEMvA_11.31_28/plugins/TarmacTrace.so",
            "FastModels_11.31_28/plugins/Linux64_armv8l_GCC-12.3/Crypto.so",
            "cluster0.cpu0.semihosting-cmd_line",
            "cluster0.cpu0.semihosting-use_stderr",
        ),
    },
    "aem-r": {
        "x86": FVP(
            "FVP_Base_AEMv8R_11.31_28/bin/FVP_BaseR_AEMv8R",
            "FVP_Base_AEMv8R_11.31_28/plugins/TarmacTrace.so",
            "FastModels_11.31_28/plugins/Linux64_GCC-12.3/Crypto.so",
            "cluster0.cpu0.semihosting-cmd_line",
            "cluster0.cpu0.semihosting-use_stderr",
        ),
        "aarch64": FVP(
            "FVP_Base_AEMv8R_11.31_28/bin/FVP_BaseR_AEMv8R",
            "FVP_Base_AEMv8R_11.31_28/plugins/TarmacTrace.so",
            "FastModels_11.31_28/plugins/Linux64_armv8l_GCC-12.3/Crypto.so",
            "cluster0.cpu0.semihosting-cmd_line",
            "cluster0.cpu0.semihosting-use_stderr",
        ),
    },
}


def run_fvp(
    fvp_install_dir,
    fvp_config_dir,
    fvp_model,
    fvp_configs,
    image,
    arguments,
    timeout,
    working_directory,
    verbose,
    tarmac_file,
):
    """Execute the program using an FVP and return the subprocess return code."""
    if fvp_model not in MODELS:
        raise Exception(f"{fvp_model} is not a recognised model name")

    # The location of files within the FVP install directory can differ
    # between the packages for different platforms.
    uname_machine = uname().machine.lower()
    if uname_machine == "x86_64":
        fvp_platform = "x86"
    elif uname_machine == "aarch64":
        fvp_platform = "aarch64"
    else:
        raise Exception(f"{uname_machine} is not a recognised uname machine")
    model = MODELS[fvp_model][fvp_platform]

    command = [path.join(fvp_install_dir, model.model_exe)]
    command.extend(["--quiet"])
    for config in fvp_configs:
        command.extend(["--config-file", path.join(fvp_config_dir, config + ".cfg")])

    if fvp_model == "corstone-320":
        command.extend(["--application", f"mps4_board.subsystem.cpu0={image}"])
    elif fvp_model == "aem-a" or fvp_model == "aem-r":
        # In case we ever need to run multiprocessor images, the instance name below
        # can be renamed to "cluster0.cpu*" (wildcard).
        command.extend(["--application", f"cluster0.cpu0={image}"])
    else:
        raise RuntimeError(
            f"FVP model {fvp_model} not covered in --application definition"
        )

    command.extend(["--parameter", f"{model.cmdline_param}={shlex.join(arguments)}"])
    if model.stderr_param is not None:
        command.extend(["--parameter", f"{model.stderr_param}=1"])
    command.extend(["--plugin", path.join(fvp_install_dir, model.crypto_plugin)])
    if tarmac_file is not None:
        command.extend(
            [
                "--plugin",
                path.join(fvp_install_dir, model.tarmac_plugin),
                "--parameter",
                "TRACE.TarmacTrace.trace-file=" + tarmac_file,
            ]
        )

    if verbose:
        print("running: {}".format(shlex.join(command)))

    # SDDKW-53824: the ":semihosting-features" pseudo-file isn't simulated
    # by these models. To work around that, we create one ourselves in the
    # test process's working directory, containing the single feature flag
    # SH_EXT_EXIT_EXTENDED, meaning that the SYS_EXIT_EXTENDED semihosting
    # request will work. This permits the test program's exit status to be
    # propagated to the exit status of the FVP, so that tests returning 77
    # for "test skipped" can be automatically detected.
    # Since multiple tests can potentially be run concurrently in the same
    # working directory, the file should be created in an atomic operation
    # to prevent one process reading an incomplete file being written from
    # another. This is done by creating a temporary file and replacing.
    shfeatures_path = path.join(working_directory, ":semihosting-features")
    try:
        fh = tempfile.NamedTemporaryFile(dir=working_directory, delete=False)
        fh.write(b"SHFB\x01")  # NamedTemporaryFile is binary already.
        fh.close()
        os.replace(fh.name, shfeatures_path)
    finally:
        try:
            os.remove(fh.name)
        except FileNotFoundError:
            pass

    result = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=sys.stderr,
        timeout=timeout,
        cwd=working_directory,
        check=False,
    )

    sys.stdout.buffer.write(result.stdout)
    return result.returncode
