#!/usr/bin/env python3

# Helper script to rename the testsuite in xml results.

# compiler-rt and LLVM libc always put all the test results into
# the "compiler-rt" or "libc" testsuite in the junit xml file. ATfE
# builds multiple variants of these projects, so it is useful to
# modify the xml to group the tests by variant so that the names do
# not overlap in a CI environment.

import argparse
import os
import re
from xml.etree import ElementTree


def main():
    parser = argparse.ArgumentParser(description="Reformat xml results")
    parser.add_argument(
        "--dir",
        required=True,
        help="Path to project build's test directory",
    )
    parser.add_argument(
        "--testsuite",
        required=True,
        help="New name for the testsuite",
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
                xml_file = os.path.join(args.dir, results_path)
    if xml_file is None:
        print(f"No xml results generated to modify.")
        return

    tree = ElementTree.parse(xml_file)
    root = tree.getroot()

    # The compiler-rt Builtins tests runs two testsuites: TestCases and Unit
    # TestCases are recorded in the "Builtins" suite.
    # But the Unit tests are recorded in "Builtins-arm-generic" or similar.
    # For readability, both can be combined all under compiler-rt-{variant}.
    # LLVM libc places all tests in the "libc" suite.
    for testsuite in root.iter("testsuite"):
        old_suitename = testsuite.get("name")
        new_suitename = args.testsuite
        testsuite.set("name", new_suitename)
        for testcase in testsuite.iter("testcase"):
            old_classname = testcase.get("classname")
            new_classname = old_classname.replace(old_suitename, new_suitename)
            testcase.set("classname", new_classname)

    tree.write(xml_file)
    print(f"Results written to {xml_file}")


if __name__ == "__main__":
    main()
