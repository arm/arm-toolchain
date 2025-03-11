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
# Usage: python generate_ATfE-SBOM.py
#
# Output file generated: ATfE-SBOM.spdx.json
#
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

# This example shows how to use the spdx-tools to create an SPDX document from scratch,
# validate it and write it to a file.


def create_Package(
    name,
    uuid,
    download_location,
    actorType,
    personName,
    personEmail,
    license,
    licenseComment,
):
    package = Package(
        name=name,
        spdx_id=uuid,
        download_location=download_location,
        supplier=Actor(actorType, personName, personEmail),
        originator=Actor(actorType, personName, personEmail),
        files_analyzed=False,
        license_concluded=spdx_licensing.parse(license),
        license_declared=spdx_licensing.parse(license),
        license_comment=licenseComment,
    )

    return package


# First up, we need general information about the creation of the document, summarised by the CreationInfo class.
creation_info = CreationInfo(
    spdx_version="SPDX-2.3",
    spdx_id="SPDXRef-DOCUMENT",
    name="Arm Toolchain for Embedded",
    data_license="CC0-1.0",
    document_namespace="https://github.com/arm/arm-toolchain/tree/arm-software/arm-software/embedded",
    creators=[
        Actor(ActorType.ORGANIZATION, "Arm Limited", "open-source-office@arm.com")
    ],
    created=datetime.now(),
)

# creation_info is the only required property of the Document class (have a look there!), the rest are optional lists.
document = Document(creation_info)

# The document currently does not describe anything. Let's create a package that we can add to it.
package_url = (
    "https://github.com/arm/arm-toolchain/tree/arm-software/arm-software/embedded"
)
package_uuid_atfe = "SPDXRef-Package-" + str(
    uuid.uuid5(uuid.NAMESPACE_URL, package_url)
)
package_atfe = create_Package(
    "Arm Toolchain for Embedded",
    package_uuid_atfe,
    package_url,
    ActorType.ORGANIZATION,
    "Arm Limited",
    "open-source-office@arm.com",
    "Apache-2.0 WITH LLVM-exception",
    "",
)

package_url = "https://github.com/llvm/llvm-project"
package_uuid_llvm = "SPDXRef-Package-" + str(
    uuid.uuid5(uuid.NAMESPACE_URL, package_url)
)
package_llvm = create_Package(
    "llvm-project",
    package_uuid_llvm,
    package_url,
    ActorType.ORGANIZATION,
    "LLVM Foundation",
    "info@llvm.org",
    "Apache-2.0 WITH LLVM-exception",
    "",
)

package_url = "https://github.com/llvm/llvm-project/tree/main/clang"
package_uuid_clang = "SPDXRef-Package-" + str(
    uuid.uuid5(uuid.NAMESPACE_URL, package_url)
)
package_clang = create_Package(
    "clang",
    package_uuid_clang,
    package_url,
    ActorType.ORGANIZATION,
    "LLVM Foundation",
    "info@llvm.org",
    "Apache-2.0 WITH LLVM-exception",
    "",
)

package_url = "https://github.com/llvm/llvm-project/tree/main/lld"
package_uuid_lld = "SPDXRef-Package-" + str(uuid.uuid5(uuid.NAMESPACE_URL, package_url))
package_lld = create_Package(
    "lld",
    package_uuid_lld,
    package_url,
    ActorType.ORGANIZATION,
    "LLVM Foundation",
    "info@llvm.org",
    "Apache-2.0 WITH LLVM-exception",
    "",
)

package_url = "https://github.com/llvm/llvm-project/tree/main/compiler-rt"
package_uuid_compiler_rt = "SPDXRef-Package-" + str(
    uuid.uuid5(uuid.NAMESPACE_URL, package_url)
)
package_compiler_rt = create_Package(
    "compiler-rt",
    package_uuid_compiler_rt,
    package_url,
    ActorType.ORGANIZATION,
    "LLVM Foundation",
    "info@llvm.org",
    "Apache-2.0 WITH LLVM-exception",
    "",
)

package_url = "https://github.com/llvm/llvm-project/tree/main/libcxx"
package_uuid_libcxx = "SPDXRef-Package-" + str(
    uuid.uuid5(uuid.NAMESPACE_URL, package_url)
)
package_libcxx = create_Package(
    "libcxx",
    package_uuid_libcxx,
    package_url,
    ActorType.ORGANIZATION,
    "LLVM Foundation",
    "info@llvm.org",
    "Apache-2.0 WITH LLVM-exception",
    "",
)

package_url = "https://github.com/llvm/llvm-project/tree/main/libcxxabi"
package_uuid_libcxxabi = "SPDXRef-Package-" + str(
    uuid.uuid5(uuid.NAMESPACE_URL, package_url)
)
package_libcxxabi = create_Package(
    "libcxxabi",
    package_uuid_libcxxabi,
    package_url,
    ActorType.ORGANIZATION,
    "LLVM Foundation",
    "info@llvm.org",
    "Apache-2.0 WITH LLVM-exception",
    "",
)

package_url = "https://github.com/llvm/llvm-project/tree/main/libunwind"
package_uuid_libunwind = "SPDXRef-Package-" + str(
    uuid.uuid5(uuid.NAMESPACE_URL, package_url)
)
package_libunwind = create_Package(
    "libunwind",
    package_uuid_libunwind,
    package_url,
    ActorType.ORGANIZATION,
    "LLVM Foundation",
    "info@llvm.org",
    "Apache-2.0 WITH LLVM-exception",
    "",
)

package_url = "https://github.com/picolibc/picolibc"
package_uuid_picolibc = "SPDXRef-Package-" + str(
    uuid.uuid5(uuid.NAMESPACE_URL, package_url)
)
package_picolibc = create_Package(
    "picolibc",
    package_uuid_picolibc,
    package_url,
    ActorType.PERSON,
    "Keith Packard",
    "https://github.com/keith-packard",
    "",
    "Detailed list of licenses is at: https://github.com/picolibc/picolibc/blob/main/COPYING.picolibc",
)

# Now that we have a package defined, we can add it to the document's package property.
document.packages = [
    package_atfe,
    package_llvm,
    package_clang,
    package_lld,
    package_compiler_rt,
    package_libcxx,
    package_libcxxabi,
    package_libunwind,
    package_picolibc,
]

# A DESCRIBES relationship asserts that the document indeed describes the package.
describes_relationship_atfe = Relationship(
    "SPDXRef-DOCUMENT", RelationshipType.DESCRIBES, package_uuid_atfe
)

describes_relationship_llvm = Relationship(
    "SPDXRef-DOCUMENT", RelationshipType.DESCRIBES, package_uuid_llvm
)

describes_relationship_clang = Relationship(
    "SPDXRef-DOCUMENT", RelationshipType.DESCRIBES, package_uuid_clang
)

describes_relationship_lld = Relationship(
    "SPDXRef-DOCUMENT", RelationshipType.DESCRIBES, package_uuid_lld
)

describes_relationship_compiler_rt = Relationship(
    "SPDXRef-DOCUMENT", RelationshipType.DESCRIBES, package_uuid_compiler_rt
)

describes_relationship_libcxx = Relationship(
    "SPDXRef-DOCUMENT", RelationshipType.DESCRIBES, package_uuid_libcxx
)

describes_relationship_libcxxabi = Relationship(
    "SPDXRef-DOCUMENT", RelationshipType.DESCRIBES, package_uuid_libcxxabi
)

describes_relationship_libunwind = Relationship(
    "SPDXRef-DOCUMENT", RelationshipType.DESCRIBES, package_uuid_libunwind
)

describes_relationship_picolibc = Relationship(
    "SPDXRef-DOCUMENT", RelationshipType.DESCRIBES, package_uuid_picolibc
)

document.relationships = [
    describes_relationship_atfe,
    describes_relationship_llvm,
    describes_relationship_clang,
    describes_relationship_lld,
    describes_relationship_compiler_rt,
    describes_relationship_libcxx,
    describes_relationship_libcxxabi,
    describes_relationship_libunwind,
    describes_relationship_picolibc,
]

# This library provides comprehensive validation against the SPDX specification.
validation_messages: List[ValidationMessage] = validate_full_spdx_document(document)

for message in validation_messages:
    logging.warning(message.validation_message)
    logging.warning(message.context)

# If the document is valid, validation_messages will be empty.
assert validation_messages == []

# Using the write_file() method from the write_anything module.
write_file(document, "ATfE-SBOM.spdx.json")
