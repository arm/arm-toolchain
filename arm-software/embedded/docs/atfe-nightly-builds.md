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

## Retention Policy

GitHub retains build logs and artifacts for 90 days by default. After this
period, they are automatically deleted.
