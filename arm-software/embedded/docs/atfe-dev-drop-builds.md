# ATfE Dev Drops Builds

## Overview

ATfE Dev Drop builds provide the community with early access to upcoming
versions of the product ahead of official releases.
These builds are made available to support testing and early validation of new
features.
Please note that Dev Drop builds may be unstable, and could contain bugs. They
are intended for evaluation and integration purposes only, and not recommended
for production use.

## Release Cycle

Dev Drop builds are automatically generated nightly as part of the ATfE Nightly
Build and Test workflow on GitHub Actions.
Once all CI and CD stages complete successfully, a set of build artifacts is
produced and uploaded.

## Accessing Dev Drop Build Artifacts

Dev Drop build artifacts are available through the GitHub Actions section of
the repository **arm-toolchain**. To locate them:

1. Navigate to the Actions tab on GitHub
2. Select the workflow titled [“ATfE Nightly Build and Test”](https://github.com/arm/arm-toolchain/actions/workflows/atfe_nightly_build_and_test.yml)
3. Choose a specific workflow run e.g., the latest successful run
4. Scroll to the bottom of the run page to find the list of generated build
artifacts

## Downloading Dev Drop Build Artifacts

You can download the artifacts directly from the GitHub web interface. Please
note that each artifact is downloaded as a zip file.

Alternatively, you can use the GitHub CLI to download artifacts. The CLI
automatically extracts the contents into the specified download directory.
Run the following command from within a local clone of the **arm-toolchain**
repository:

```
gh run download
```

## Retention Policy

GitHub retains build logs and artifacts for 90 days by default. After this
period, they are automatically deleted.
