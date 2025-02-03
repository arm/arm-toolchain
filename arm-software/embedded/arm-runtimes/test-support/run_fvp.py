#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright 2024 Arm Limited and/or its affiliates <open-source-office@arm.com>

import re
import subprocess
import sys
from os import path
from dataclasses import dataclass
import shlex

@dataclass
class FVP:
    model_exe: str
    tarmac_plugin: str
    crypto_plugin: str
    cmdline_param: str

MODELS = {
    "corstone-310": FVP(
        "Corstone-310/models/Linux64_GCC-9.3/FVP_Corstone_SSE-310",
        "Corstone-310/plugins/Linux64_GCC-9.3/TarmacTrace.so",
        "FastModelsPortfolio_11.27/plugins/Linux64_GCC-9.3/Crypto.so",
        "cpu0.semihosting-cmd_line",
    ),
    "aem-a": FVP(
        "Base_RevC_AEMvA_pkg/models/Linux64_GCC-9.3/FVP_Base_RevC-2xAEMvA",
        "Base_RevC_AEMvA_pkg/plugins/Linux64_GCC-9.3/TarmacTrace.so",
        "FastModelsPortfolio_11.27/plugins/Linux64_GCC-9.3/Crypto.so",
        "cluster0.cpu0.semihosting-cmd_line",
    ),
    "aem-r": FVP(
        "AEMv8R_base_pkg/models/Linux64_GCC-9.3/FVP_BaseR_AEMv8R",
        "AEMv8R_base_pkg/plugins/Linux64_GCC-9.3/TarmacTrace.so",
        "FastModelsPortfolio_11.27/plugins/Linux64_GCC-9.3/Crypto.so",
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
    with open(
        path.join(working_directory, ":semihosting-features"), "wb"
    ) as fh:
        fh.write(b"SHFB\x01")

    result = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=sys.stderr,
        timeout=timeout,
        cwd=working_directory,
        check=False,
    )

    # Corstone-310 prints out boilerplate text on stdout alongside the actual
    # output of the image. Some tests, for instance in libcxx, check the
    # contents of stdout, and may treat the unexpected text as a condition for
    # failure. To work around this, we cut out the model's boilerplate output.
    if fvp_model == "corstone-310":
        decoded_stdout = result.stdout.decode()
        expected_stdout_format = r"""
    Ethos-U rev [0-9a-z]+ --- \w{3} \d{2} \d{4} \d{2}:\d{2}:\d{2}
    \(C\) COPYRIGHT (?:\d{4}|\d{4}-\d{4})(?:,\s?(?:\d{4}|\d{4}-\d{4}))* Arm Limited
    ALL RIGHTS RESERVED

(.*)
Info: /OSCI/SystemC: Simulation stopped by user.
\[warning \]\[main@0\]\[\d+ ns\] Simulation stopped by user
"""

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
