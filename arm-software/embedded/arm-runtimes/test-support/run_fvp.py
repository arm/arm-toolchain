#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright 2024 Arm Limited and/or its affiliates <open-source-office@arm.com>

import os
import re
import subprocess
import sys
import tempfile
from os import environ
from os import path
from dataclasses import dataclass
from platform import uname
import shlex


@dataclass
class FVP:
    model_exe: str
    tarmac_plugin: str
    crypto_plugin: str
    cmdline_param: str


# The location of files within the FVP install directory can differ
# between the packages for different platforms.
uname_machine = uname().machine.lower()
if uname_machine == "x86_64":
    platform_dir = "Linux64_GCC-9.3"
elif uname_machine == "aarch64":
    platform_dir = "Linux64_armv8l_GCC-9.3"
else:
    raise Exception(f"{uname_machine} is not a recognised uname machine")

MODELS = {
    "corstone-310": FVP(
        f"Corstone-310/models/{platform_dir}/FVP_Corstone_SSE-310",
        f"Corstone-310/plugins/{platform_dir}/TarmacTrace.so",
        f"FastModelsPortfolio_11.27/plugins/{platform_dir}/Crypto.so",
        "cpu0.semihosting-cmd_line",
    ),
    "aem-a": FVP(
        f"Base_RevC_AEMvA_pkg/models/{platform_dir}/FVP_Base_RevC-2xAEMvA",
        f"Base_RevC_AEMvA_pkg/plugins/{platform_dir}/TarmacTrace.so",
        f"FastModelsPortfolio_11.27/plugins/{platform_dir}/Crypto.so",
        "cluster0.cpu0.semihosting-cmd_line",
    ),
    "aem-r": FVP(
        f"AEMv8R_base_pkg/models/{platform_dir}/FVP_BaseR_AEMv8R",
        f"AEMv8R_base_pkg/plugins/{platform_dir}/TarmacTrace.so",
        f"FastModelsPortfolio_11.27/plugins/{platform_dir}/Crypto.so",
        "cluster0.cpu0.semihosting-cmd_line",
    ),
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
    model = MODELS[fvp_model]

    env = environ.copy()
    if fvp_model == "corstone-310":
        # Corstone-310 v11.27 requires PYTHONHOME to be set to the version of
        # python included in the install, and fmtplib and python/lib to be found
        # in LD_LIBRARY_PATH. If the installed version include these, then add
        # them to the environment.
        fmtplib_dir = path.join(fvp_install_dir, "Corstone-310", "fmtplib")
        python_dir = path.join(fvp_install_dir, "Corstone-310", "python")
        ld_library_paths = []
        if path.exists(fmtplib_dir):
            ld_library_paths.append(fmtplib_dir)
        if path.exists(python_dir):
            ld_library_paths.append(path.join(python_dir, "lib"))
            env["PYTHONHOME"] = python_dir
        if len(ld_library_paths) > 0:
            if "LD_LIBRARY_PATH" in env:
                ld_library_paths.append(env["LD_LIBRARY_PATH"])
            env["LD_LIBRARY_PATH"] = ":".join(ld_library_paths)

    command = [path.join(fvp_install_dir, model.model_exe)]
    command.extend(["--quiet"])
    for config in fvp_configs:
        command.extend(["--config-file", path.join(fvp_config_dir, config + ".cfg")])

    if fvp_model == "corstone-310":
        command.extend(["--application", f"cpu0={image}"])
    elif fvp_model == "aem-a" or fvp_model == "aem-r":
        # In case we ever need to run multiprocessor images, the instance name below
        # can be renamed to "cluster0.cpu*" (wildcard).
        command.extend(["--application", f"cluster0.cpu0={image}"])
    else:
        raise RuntimeError(
            f"FVP model {fvp_model} not covered in --application definition"
        )

    command.extend(["--parameter", f"{model.cmdline_param}={shlex.join(arguments)}"])
    command.extend(["--plugin", path.join(fvp_install_dir, model.crypto_plugin)])
    if tarmac_file is not None:
        command.extend([
            "--plugin",
            path.join(fvp_install_dir, model.tarmac_plugin),
            "--parameter",
            "TRACE.TarmacTrace.trace-file=" + tarmac_file,
        ])

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
        env=env,
    )

    # Corstone-310 prints out boilerplate text on stdout alongside the actual
    # output of the image. Some tests, for instance in libcxx, check the
    # contents of stdout, and may treat the unexpected text as a condition for
    # failure. To work around this, we cut out the model's boilerplate output.
    if fvp_model == "corstone-310":
        decoded_stdout = result.stdout.decode()
        boilerplate_pattern = r"""
    Ethos-U rev [0-9a-z]+ --- \w{3} {1,2}\d{1,2} \d{4} \d{2}:\d{2}:\d{2}
    \(C\) COPYRIGHT (?:\d{4}|\d{4}-\d{4})(?:,\s?(?:\d{4}|\d{4}-\d{4}))* Arm Limited
    ALL RIGHTS RESERVED

"""
        # Not all Corstone-310 versions print the "Info" message.
        stop_info_pattern = "\nInfo: /OSCI/SystemC: Simulation stopped by user.\n"
        stop_warning_pattern = r"""\[warning \]\[main@0\]\[\d+ ns\] Simulation stopped by user
"""
        expected_stdout_format = (
            f"{boilerplate_pattern}(.*?)(?:{stop_info_pattern})?{stop_warning_pattern}"
        )
        regex_result = re.fullmatch(
            expected_stdout_format, decoded_stdout, flags=re.DOTALL
        )
        if not regex_result:
            error_msg = (
                f"Corstone's output format is different than expected\n"
                f"Expected (regex): {expected_stdout_format}\n"
                f"Got: {decoded_stdout}"
            )
            raise RuntimeError(error_msg)

        relevant_stdout = regex_result[1]
        result_stdout = relevant_stdout.encode()
    else:
        result_stdout = result.stdout

    sys.stdout.buffer.write(result_stdout)
    return result.returncode
