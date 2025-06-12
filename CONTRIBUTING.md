# Contributing to the Arm Toolchain

The Arm Toolchain repository is a fork of the *llvm-project*, with an
additional arm-software directory containing build scripts to produce
Arm Toolchain for Embedded and Arm Toolchain for Linux, as well as
documentation and samples.

Additionally, Arm Toolchain for Embedded can integrate external
projects such as the *picolibc* C-library or the *newlib* C-library
via an overlay package.

Contributions are welcome that improve the project in areas including
but not limited to:
* Integration of upstream components.
* Packaging.
* Testing of the above.
* Documentation.

The Arm Toolchain repository is synchronized with the upstream
*llvm-project* including the main and release branches. Arm Toolchain
for Embedded aims to stay close to the head of the *picolibc* main
branch. The *newlib* overlay package aims to track the most recent
*newlib* release.

All changes that can be made in the upstream project, must be made in
the upstream project. Please see the [Downstream Patch Policy](#downstream-patch-policy)
below for what Arm considers a downstream patch along with the
additional requirements for such a patch.

For guidance on how to contribute to the upstream projects see:
* *llvm-project* [Contributing to LLVM](https://llvm.org/docs/Contributing.html).
* *picolibc* GitHub.
[Pull requests documentation](https://docs.github.com/en/pull-requests)

## Ways to contribute

### Report an issue

Arm Toolchain is heavily dependent on the *llvm* and *picolibc* projects.  If you have an issue that is generic to one of these projects, you may want to submit it directly:
* [LLVM issue tracker](https://github.com/llvm/llvm-project/issues)
* [Picolibc issue tracker](https://github.com/picolibc/picolibc/issues)

This will allow you to interact directly with this community, including Arm's development team.

If the issue is in Arm Toolchain itself, or if you are not sure, please create a GitHub issue in the *arm-toolchain* project:
* [Arm Toolchain issue tracker](https://github.com/arm/arm-toolchain/issues)
* Please add the `ATfE` label for Arm Toolchain for Embedded issues, or `ATfL` for Arm Toolchain for Linux.

Please attach any support files to the issue itself,
see [Attaching files](https://docs.github.com/en/get-started/writing-on-github/working-with-advanced-formatting/attaching-files).

### Submit a fix

For a small change, please create a Pull Request as described in
_How to submit a change_ section below.

### Suggest a feature or bigger change

For a bigger change, please create an issue in the
*arm-toolchain* project
[Issues](https://github.com/arm/arm-toolchain/issues)
list and put [RFC] in the subject line (Request for Comments) to initiate the discussion
first, before submitting a PR for the change.

There is no formal template for an `RFC`, however it would be good to
explain the purpose of the change and the key design options, proposed
decisions.

## How to submit a change

Contributions are accepted under the
[Apache License 2.0 with LLVM Exceptions](https://github.com/arm/arm-toolchain/blob/arm-software/LICENSE.TXT).
Only submit contributions where you have authored all of the code.

### Pull request

This project follows the conventional
[GitHub pull request](https://docs.github.com/en/pull-requests) flow.

### Testing

Please ensure your change doesn't break tests. (The project doesn't yet have
GitHub actions to do this). Except for documentation changes, please check that
this passes:

```
ninja check-llvm-toolchain
```

### Coding style

Use the following commands to check the scripts before submitting a pull
request:

```
$ ./setup.sh
$ ./run-precommit-checks.sh
```

## Downstream Patch Policy

### What is considered a downstream patch?

A patch is considered a downstream patch if:

* It adds, modifies or removes, any file in the *arm-toolchain*
  repository outside of the *arm-software* directory.

* It is a patch file to be applied to an external project used by a
  toolchain built from the fork. For example *picolibc* or *newlib*.

All contributions that can be made in an upstream project, must be
made in the upstream project.

### Suitable categories for downstream patches

#### Toolchain integration

Patches that are specific to integrating components of the Arm
Toolchain or the projects that it depends on, may be implemented
downstream if there's insufficient benefit to the upstream
project. For example upstream may not be willing to accept a patch
that only affects Arm Toolchain, but they may be willing to accept a
patch that improves integration with another open-source project in
the general case.

#### Guarded Optimizations for key benchmarks

Some optimizations that are important for benchmarks that are
important to Arm are not acceptable upstream. Patches that implement
these optimizations are permitted when the following conditions hold:

* All attempts at upstreaming the optimization have failed.

* The optimization is required for a benchmark that Arm promotes for
  its CPUs.

* The changes are guarded behind an opt-in command-line
  option. Code-generation must not be affected when the command-line
  option is not used.

#### Changes critical to Arm that are in upstream review

When a feature or bug fix that is both important and urgent for Arm,
is likely to face a long period of upstream review then it may be
carried as a downstream patch. Downstream patches in upstream review
must have experimental status and will be replaced with the upstream
implementation once that has landed.

#### Backports critical to Arm for a numbered release

When a feature or bug fix in the main branch of upstream
*llvm-project* has missed the cut off for the upstream numbered
release, Arm may choose to backport the change to the Arm Toolchain
numbered release. An attempt must have been made to use the upstream
backport process first.

## Requirements for a downstream patch

The following requirements for downstream patches are to assist in
auditing and tracking. All downstream patches must satisfy all the
requirements below:

* The pull request description or comment must contain a justification
  of why the patch isn't being made upstream. This should include
  links to any previous attempts to upstream.

* The commit message for a new downstream patch must include
  `Downstream issue:#<issue number>` where `#<issue number>` is an issue
  that contains the reason for the downstream patch,
  for example `Downstream issue:#123`. A space is allowed before the `#<issue number>`.

* The source change should be annotated with a comment including the
  `#<issue number>`, if there are multiple lines changed a single
  comment for the block can be used.

* The commit message that removes an existing downstream patch, such
  as when the upstream equivalent lands, must include `Removes
  downstream issue:#<issue number>`.

* New files in the *arm-toolchain* repository must include the LLVM
  license, with the standard header comment and SPDX identifier.

* New files added to an external project like *picolibc* or *newlib*
  must follow the licensing and copyright of the external project.

* Any patch that causes the Arm Toolchain to deviate from upstream
  behavior, such as a new command-line option, must have user
  documentation. See [Downstream Feature
  Documentation](arm-software/docs/arm-toolchain-features.md) for the location
  and required contents.

### Downstream Feature Documentation

There are three files that contain the user facing documentation for downstream features.

* `arm-software/docs/arm-toolchain-features.md` for features common to
  both Arm Toolchain for Embedded and Arm Toolchain for Linux.

* `arm-software/docs/arm-toolchain-for-embedded-features.md` for
  features specific to Arm Toolchain for Embedded.

* `arm-software/docs/arm-toolchain-for-linux-features.md` for
  features specific to Arm Toolchain for Linux.

The documentation for a feature must state if it is
experimental. Experimental features may change or be removed at any
point in the future.
