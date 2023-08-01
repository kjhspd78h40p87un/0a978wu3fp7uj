"""Functions for ignoring false-positive violations based on source code."""

import os
import re

import glog as log

def ignore_violation(violation, source_code_directory):
    """Checks that the violation isn't filterable from the source code."""

    violation_fname = violation.specification.function.source_name
    if "print" in violation_fname:
        return True
    if not source_code_directory:
        return False

    violation_pname = violation.parent_function.source_name
    # Not all modules have debugging information, especially as the project
    # becomes larger with multiple modules.
    if not violation.location or not violation.location.file:
        log.warning(
            f"Violation related to {violation_fname} in {violation_pname} "
            "has no location information!")
        return False

    source_code_file = os.path.join(
        source_code_directory, violation.location.file)
    should_ignore = False
    try:
        with open(source_code_file, "rb") as f:
            lines = f.readlines()
            # Not all files are encoded using UTF-8. We may need to add more in
            # the future.
            line = ""
            for enc in ["utf-8", "ISO-8859-1"]:
                try:
                    line = lines[violation.location.line-1].decode(enc)
                # Current encoding did not work, try the next.
                except UnicodeDecodeError:
                    continue
                break
            # Leaving as a separate function for now, as there may be more
            # __ignore_* functions that we write in the future.
            should_ignore = ignore_void_cast(line)
    except FileNotFoundError:
        log.warning(
            f"Source code file {source_code_file} not found for violation "
            f"of {violation.specification.function.source_name} in "
            f"{violation.parent_function.source_name}!")
        return False

    return should_ignore

def ignore_void_cast(code_snippet):
    """Searches a code snippet for a void cast and returns True if found.

    It is safe to ignore unused violations where a function return value is
    casted to void, since this is common practice in C when developers want
    to explicitly ignore function return values and suppress compiler warnings.

    Args:
        code_snippet: Code snippet string that captures the violation.
    """
    return re.search(r"(\s*void\s*)", code_snippet)
