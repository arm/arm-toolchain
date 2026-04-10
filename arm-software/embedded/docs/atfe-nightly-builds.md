# ATfE Nightly Builds

## Overview

The ATfE Nightly builds provide the community with early access to upcoming
versions of the product ahead of official releases.
These builds are made available to support testing and early validation of new
features.

> [!WARNING]
> Please note that Nightly builds may be unstable, and could contain bugs. They
are intended for evaluation and integration purposes only, and not recommended
for production use.

## Release Cycle

ATfE Nightly builds are automatically generated as part of the ATfE Nightly
Build and Test workflow on GitHub Actions.
Once all CI and CD stages complete successfully, a set of build artifacts is
produced and uploaded.

## Accessing Nightly Build Artifacts

Nightly build artifacts are available through the GitHub Actions section of
this repository. To locate them:

1. Navigate to the [“ATfE Nightly Build and Test”](https://github.com/arm/arm-toolchain/actions/workflows/atfe_nightly_build_and_test.yml) Workflow in the Github Actions tab
3. Choose a specific workflow run e.g., the latest successful run
4. Scroll to the bottom of the run page to find the list of generated build
artifacts

## Downloading Nightly Build Artifacts

You can download the artifacts directly from the GitHub web interface.
Each artifact is provided as a zip file by GitHub. Inside this zip file, you
will find the actual ATfE package, which is distributed as either a .zip or
.tar.xz archive depending on the target platform.

Alternatively, you can use the GitHub CLI to download artifacts. The CLI
automatically extracts the contents into the specified download directory.
Run the following command from within a local clone of the **arm-toolchain**
repository:

```
gh run download
```

## Using Nightly Build executables

Executable files in the Nightly Builds are not signed, thus may trigger
a host OS protection mechanism:

* **Windows** may show a warning about unknown publisher and require user approval
  to run the binary.
* **macOS** may block the binary from running by quarantining it.
  * To prevent macOS from blocking the binaries, extract the package downloaded
    from GitHub using a command line tool, e.g. `unzip`, this will prevent macOS
    from setting the quarantine attribute.
  * To remove the quarantine attribute, change directory to `bin` and run the
    following command:

    ```
    $ find . -type f -perm +0111 | xargs xattr -d com.apple.quarantine
    ```

## Checking Nightly Build quality

Test results in XML format and detailed test execution logs can be downloaded
from the list of Artifacts:
* `test-results-*-<host OS>`
* `logs-*-<host OS>`

## Retention Policy

GitHub retains build logs and artifacts for 90 days by default. After this
period, they are automatically deleted.
