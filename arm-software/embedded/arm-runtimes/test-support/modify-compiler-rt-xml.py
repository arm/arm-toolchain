#!/usr/bin/env python3

# Helper script to modify the xml results from compiler-rt.

# compiler-rt always puts all the test results into the "compiler-rt"
# testsuite in the junit xml file. We have multiple variants of
# compiler-rt, so the xml is modified to group the tests by variant.

import argparse
import os
import re
from xml.etree import ElementTree


def main():
    parser = argparse.ArgumentParser(description="Reformat compiler-rt xml results")
    parser.add_argument(
        "--dir",
        required=True,
        help="Path to compiler-rt build directory",
    )
    parser.add_argument(
        "--variant",
        required=True,
        help="Name of the variant under test",
    )
    args = parser.parse_args()

    xml_file = None
    # The xml file path can be set by lit's --xunit-xml-output option.
    # Since ATfE does not set this directly, it will likely be found
    # in the LIT_OPTS environment variable, which lit will read
    # options from.
    if "LIT_OPTS" in os.environ:
        lit_opts = os.environ["LIT_OPTS"]
        m = re.search("--xunit-xml-output=([^ ]+)", lit_opts)
        if m is not None:
            results_path = m.group(1)
            # Path may be absolute or relative.
            if os.path.isabs(results_path):
                xml_file = results_path
            else:
                # If not absolute, the path will be relative to compiler-rt/test
                # in the build directory, not this script.
                xml_file = os.path.join(args.dir, "compiler-rt", "test", results_path)
    if xml_file is None:
        print(f"No xml results generated to modify.")
        return

    tree = ElementTree.parse(xml_file)
    root = tree.getroot()

    # The compiler-rt Builtins tests runs two testsuites: TestCases and Unit
    # TestCases are recorded in the "Builtins" suite.
    # But the Unit tests are recorded in "Builtins-arm-generic" or similar.
    # For readability, combine them all under compiler-rt-{variant}-Builtins
    for testsuite in root.iter("testsuite"):
        old_suitename = testsuite.get("name")
        new_suitename = f"compiler-rt-{args.variant}-Builtins"
        testsuite.set("name", new_suitename)
        for testcase in testsuite.iter("testcase"):
            old_classname = testcase.get("classname")
            new_classname = old_classname.replace(old_suitename, new_suitename)
            testcase.set("classname", new_classname)

    tree.write(xml_file)
    print(f"Results written to {xml_file}")


if __name__ == "__main__":
    main()
