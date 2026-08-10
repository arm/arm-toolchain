#  SPDX-FileCopyrightText: 2023 spdx contributors
#
# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
#
#  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Purpose: This script helps to generate SBOM for ATfE.
#
# Installation: Kindly refer https://github.com/spdx/tools-python for pre-requisite installation.
#
# Sample usage:
# python generate_ATfE-SBOM.py all
# python generate_ATfE-SBOM.py main
# python generate_ATfE-SBOM.py llvmlibc
# python generate_ATfE-SBOM.py newlib
#
# Output file generated: either one or all of these:
# ATfE-SBOM.spdx.json
# ATfE-SBOM-llvmlibc-overlay.spdx.json
# ATfE-SBOM-newlib-overlay.spdx.json
#
import argparse
import sys
import logging
import uuid
from datetime import datetime
from typing import List

from spdx_tools.common.spdx_licensing import spdx_licensing
from spdx_tools.spdx.model import (
    Actor,
    ActorType,
    CreationInfo,
    Document,
    Package,
    Relationship,
    RelationshipType,
)
from spdx_tools.spdx.validation.document_validator import validate_full_spdx_document
from spdx_tools.spdx.validation.validation_message import ValidationMessage
from spdx_tools.spdx.writer.write_anything import write_file

# Define the constants used in the script
URL_ATFE = (
    "https://github.com/arm/arm-toolchain/tree/arm-software/arm-software/embedded"
)
URL_LLVM_PROJECT = "https://github.com/llvm/llvm-project"
URL_CLANG = "https://github.com/llvm/llvm-project/tree/main/clang"
URL_LLD = "https://github.com/llvm/llvm-project/tree/main/lld"
URL_COMPILER_RT = "https://github.com/llvm/llvm-project/tree/main/compiler-rt"
URL_LIBCXX = "https://github.com/llvm/llvm-project/tree/main/libcxx"
URL_LIBCXXABI = "https://github.com/llvm/llvm-project/tree/main/libcxxabi"
URL_LIBUNWIND = "https://github.com/llvm/llvm-project/tree/main/libunwind"
URL_LIBC = "https://github.com/llvm/llvm-project/tree/main/libc"

URL_PICOLIBC = "https://github.com/picolibc/picolibc"
URL_COPYING_PICOLIBC = "https://github.com/picolibc/picolibc/blob/main/COPYING.picolibc"
URL_KEITH_PACKARD = "https://github.com/keith-packard"

URL_NEWLIB = "https://sourceware.org/newlib/"
URL_NEWLIB_COPYING_NEWLIB = (
    "https://sourceware.org/git/?p=newlib-cygwin.git;a=blob;f=COPYING.NEWLIB;hb=HEAD"
)
URL_NEWLIB_COPYING_LIBGLOSS = (
    "https://sourceware.org/git/?p=newlib-cygwin.git;a=blob;f=COPYING.LIBGLOSS;hb=HEAD"
)

EMAIL_ID_ARM_OSO = "open-source-office@arm.com"
EMAIL_ID_LLVM_PROJECT = "info@llvm.org"
EMAIL_ID_NEWLIB = "newlib@sourceware.org"

SPDX_VERSION = "SPDX-2.3"
STRING_SPDX_ID = "SPDXRef-DOCUMENT"
STRING_SPDX_PACKAGE = "SPDXRef-Package-"

# Name of SBOM file for main, llvmlibc and newlib
SBOM_JSON_FILENAME_ATFE = "ATfE-SBOM.spdx.json"
SBOM_JSON_FILENAME_LLVM_LIBC_OVERLAY = "ATfE-SBOM-llvmlibc-overlay.spdx.json"
SBOM_JSON_FILENAME_NEWLIB_OVERLAY = "ATfE-SBOM-newlib-overlay.spdx.json"

# Define the string names for SBOM title
STRING_ATFE = "Arm Toolchain for Embedded"
STRING_LLVM_LIBC_OVERLAY = "Arm Toolchain for Embedded LLVM libc overlay"
STRING_NEWLIB_OVERLAY = "Arm Toolchain for Embedded newlib overlay"

STRING_LLVM_FOUNDATION = "LLVM Foundation"
STRING_ARM_LIMITED = "Arm Limited"
APACHE20_WITH_LLVM_EXCEPTION = "Apache-2.0 WITH LLVM-exception"

# List to indicate choices for the arguments to the script
ARGUMENT_CHOICES = ["all", "main", "llvmlibc", "newlib"]

# Dict with details to define the contents of package
# Update the name, SBOM file name and contents of SBOM as appropriate.
dict_details_SBOM = {
    STRING_ATFE: {
        "sbom_filename": SBOM_JSON_FILENAME_ATFE,
        "sources": [
            STRING_ATFE,
            "llvm-project",
            "clang",
            "lld",
            "compiler-rt",
            "libcxx",
            "libcxxabi",
            "libunwind",
            "picolibc",
        ],
    },
    STRING_LLVM_LIBC_OVERLAY: {
        "sbom_filename": SBOM_JSON_FILENAME_LLVM_LIBC_OVERLAY,
        "sources": [STRING_ATFE, "compiler-rt", "libc"],
    },
    STRING_NEWLIB_OVERLAY: {
        "sbom_filename": SBOM_JSON_FILENAME_NEWLIB_OVERLAY,
        "sources": [
            STRING_ATFE,
            "compiler-rt",
            "libcxx",
            "libcxxabi",
            "libunwind",
            "newlib",
        ],
    },
}

# Dict to define the details of each source
# Store the details in "value" field as list.
# Syntax is name of source : list of details
dict_source_details = {
    STRING_ATFE: [
        URL_ATFE,
        ActorType.ORGANIZATION,
        STRING_ARM_LIMITED,
        EMAIL_ID_ARM_OSO,
        APACHE20_WITH_LLVM_EXCEPTION,
        "",
    ],
    "llvm-project": [
        URL_LLVM_PROJECT,
        ActorType.ORGANIZATION,
        STRING_LLVM_FOUNDATION,
        EMAIL_ID_LLVM_PROJECT,
        APACHE20_WITH_LLVM_EXCEPTION,
        "",
    ],
    "clang": [
        URL_CLANG,
        ActorType.ORGANIZATION,
        STRING_LLVM_FOUNDATION,
        EMAIL_ID_LLVM_PROJECT,
        APACHE20_WITH_LLVM_EXCEPTION,
        "",
    ],
    "lld": [
        URL_LLD,
        ActorType.ORGANIZATION,
        STRING_LLVM_FOUNDATION,
        EMAIL_ID_LLVM_PROJECT,
        APACHE20_WITH_LLVM_EXCEPTION,
        "",
    ],
    "compiler-rt": [
        URL_COMPILER_RT,
        ActorType.ORGANIZATION,
        STRING_LLVM_FOUNDATION,
        EMAIL_ID_LLVM_PROJECT,
        APACHE20_WITH_LLVM_EXCEPTION,
        "",
    ],
    "libcxx": [
        URL_LIBCXX,
        ActorType.ORGANIZATION,
        STRING_LLVM_FOUNDATION,
        EMAIL_ID_LLVM_PROJECT,
        APACHE20_WITH_LLVM_EXCEPTION,
        "",
    ],
    "libcxxabi": [
        URL_LIBCXXABI,
        ActorType.ORGANIZATION,
        STRING_LLVM_FOUNDATION,
        EMAIL_ID_LLVM_PROJECT,
        APACHE20_WITH_LLVM_EXCEPTION,
        "",
    ],
    "libunwind": [
        URL_LIBUNWIND,
        ActorType.ORGANIZATION,
        STRING_LLVM_FOUNDATION,
        EMAIL_ID_LLVM_PROJECT,
        APACHE20_WITH_LLVM_EXCEPTION,
        "",
    ],
    "libc": [
        URL_LIBC,
        ActorType.ORGANIZATION,
        STRING_LLVM_FOUNDATION,
        EMAIL_ID_LLVM_PROJECT,
        APACHE20_WITH_LLVM_EXCEPTION,
        "",
    ],
    "picolibc": [
        URL_PICOLIBC,
        ActorType.PERSON,
        "Keith Packard",
        URL_KEITH_PACKARD,
        "",
        "Detailed list of licenses is at: " + URL_COPYING_PICOLIBC,
    ],
    "newlib": [
        URL_NEWLIB,
        ActorType.ORGANIZATION,
        URL_NEWLIB,
        EMAIL_ID_NEWLIB,
        "",
        "Detailed list of licenses is at: "
        + URL_NEWLIB_COPYING_NEWLIB
        + " and "
        + URL_NEWLIB_COPYING_LIBGLOSS,
    ],
}


# Creates the package from the given details, such as name and URL string
# Returns the package and UUID
def create_Package(
    name,
    URL_string,
    actorType,
    personName,
    personEmail,
    license,
    licenseComment,
):
    package_uuid = get_UUID(URL_string)

    package = Package(
        name=name,
        spdx_id=package_uuid,
        download_location=URL_string,
        supplier=Actor(actorType, personName, personEmail),
        originator=Actor(actorType, personName, personEmail),
        files_analyzed=False,
        license_concluded=spdx_licensing.parse(license),
        license_declared=spdx_licensing.parse(license),
        license_comment=licenseComment,
    )

    return package, package_uuid


# Creates the UUID from the given URL
def get_UUID(URL_string):
    return STRING_SPDX_PACKAGE + str(uuid.uuid5(uuid.NAMESPACE_URL, URL_string))


def getCreationInfo(name):
    # First up, we need general information about the creation of the document, summarised by the CreationInfo class.
    creation_info = CreationInfo(
        spdx_version=SPDX_VERSION,
        spdx_id=STRING_SPDX_ID,
        name=name,
        data_license="CC0-1.0",
        document_namespace=URL_ATFE,
        creators=[Actor(ActorType.ORGANIZATION, STRING_ARM_LIMITED, EMAIL_ID_ARM_OSO)],
        created=datetime.now(),
    )
    return creation_info


def describe_Relationship(package_UUID):
    return Relationship(STRING_SPDX_ID, RelationshipType.DESCRIBES, package_UUID)


def validate_document(document):
    # This library provides comprehensive validation against the SPDX specification.
    validation_messages: List[ValidationMessage] = validate_full_spdx_document(document)

    for message in validation_messages:
        logging.warning(message.validation_message)
        logging.warning(message.context)

    # If the document is valid, validation_messages will be empty.
    assert validation_messages == []


def generateSBOMFromDetails(string_name):
    if string_name not in dict_details_SBOM.keys():
        print(
            "Error: Exiting script, since name does not exist in dict_details_SBOM: "
            + string_name
        )
        sys.exit(1)

    # Extract the content of dict
    sbom_filename = dict_details_SBOM[string_name]["sbom_filename"]
    list_sources = dict_details_SBOM[string_name]["sources"]

    creation_info_name = getCreationInfo(string_name)

    # creation_info is the only required property of the Document class.
    document_name = Document(creation_info_name)

    # Initialize the list of packages for the document
    document_name.packages = []

    # Initialize the list of relationships for the document
    document_name.relationships = []

    for source_name in list_sources:
        if source_name not in dict_source_details.keys():
            print(
                "Error: Exiting script, since name does not exist in dict_source_details: "
                + source_name
            )
            sys.exit(1)

        # Unpack the list contents
        (
            URL_string,
            actorType,
            personName,
            personEmail,
            license,
            licenseComment,
        ) = dict_source_details[source_name]

        # Let's create a package that we can add to it.
        package_name, uuid_name = create_Package(
            source_name,
            URL_string,
            actorType,
            personName,
            personEmail,
            license,
            licenseComment,
        )
        describes_relationship_field = describe_Relationship(uuid_name)

        document_name.packages.append(package_name)
        document_name.relationships.append(describes_relationship_field)

    validate_document(document_name)

    # Write to the JSON files
    write_file(document_name, sbom_filename)

    print("Completed generating SBOM file.")


def main():
    parser = argparse.ArgumentParser(
        description="Process user input for different sources, for which SBOM needs to be generated."
    )
    parser.add_argument(
        "source",
        choices=ARGUMENT_CHOICES,
        help="Specify the source: " + str(ARGUMENT_CHOICES),
    )

    args = parser.parse_args()

    if args.source not in ARGUMENT_CHOICES:
        print("Unknown option. Exiting script.")
        sys.exit(1)

    # ATfE
    if args.source == "main" or args.source == "all":
        print("Generating SBOM for main branch of ATfE.")
        generateSBOMFromDetails(STRING_ATFE)

    # llvm libc overlay
    if args.source == "llvmlibc" or args.source == "all":
        print("Generating SBOM for llvmlibc.")

        generateSBOMFromDetails(STRING_LLVM_LIBC_OVERLAY)

    # newlib overlay
    if args.source == "newlib" or args.source == "all":
        print("Generating SBOM for newlib.")

        generateSBOMFromDetails(STRING_NEWLIB_OVERLAY)


if __name__ == "__main__":
    main()
