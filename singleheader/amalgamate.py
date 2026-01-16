#!/usr/bin/env python3
#
# Amalgamation script for robotstxt library.
# Generates single-header versions: robots.h and reporting_robots.h
#
# Usage: python3 singleheader/amalgamate.py
#

import os
import re
import sys
from datetime import datetime

# Paths
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(SCRIPT_DIR)
OUTPUT_DIR = SCRIPT_DIR

# Files to process
ROBOTS_HEADER = os.path.join(ROOT_DIR, "robots.h")
ROBOTS_SOURCE = os.path.join(ROOT_DIR, "robots.cc")
ROBOTS_C_HEADER = os.path.join(ROOT_DIR, "bindings", "c", "robots_c.h")
ROBOTS_C_SOURCE = os.path.join(ROOT_DIR, "bindings", "c", "robots_c.cc")
REPORTING_HEADER = os.path.join(ROOT_DIR, "reporting_robots.h")
REPORTING_SOURCE = os.path.join(ROOT_DIR, "reporting_robots.cc")


def get_git_info():
    """Get git commit info for version tracking."""
    try:
        import subprocess
        commit = subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"],
            cwd=ROOT_DIR, stderr=subprocess.DEVNULL
        ).decode().strip()
        date = subprocess.check_output(
            ["git", "log", "-1", "--format=%ci"],
            cwd=ROOT_DIR, stderr=subprocess.DEVNULL
        ).decode().strip()
        return commit, date
    except Exception:
        return None, datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def read_file(path):
    """Read file contents."""
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def remove_include(content, header_name):
    """Remove #include "header_name" from content."""
    pattern = rf'^\s*#include\s*"{re.escape(header_name)}"\s*\n?'
    return re.sub(pattern, "", content, flags=re.MULTILINE)


def strip_include_guards(content):
    """Remove include guards from header content."""
    lines = content.split('\n')
    result = []
    skip_first_ifndef = True
    skip_first_define = True

    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        # Skip the opening #ifndef GUARD
        if skip_first_ifndef and stripped.startswith('#ifndef ') and '_H' in stripped:
            skip_first_ifndef = False
            i += 1
            continue

        # Skip the opening #define GUARD
        if skip_first_define and stripped.startswith('#define ') and '_H' in stripped:
            skip_first_define = False
            i += 1
            continue

        result.append(line)
        i += 1

    # Remove trailing #endif for the guard
    while result and result[-1].strip() == '':
        result.pop()

    if result and result[-1].strip().startswith('#endif'):
        result.pop()

    return '\n'.join(result)


def extract_implementation(source_content, local_headers):
    """Extract implementation code, removing local includes."""
    content = source_content

    # Remove local includes
    for header in local_headers:
        content = remove_include(content, header)

    # Find the end of the license/file header comment block
    lines = content.split('\n')
    start_idx = 0

    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith('//') or stripped == '':
            continue
        else:
            start_idx = i
            break

    return '\n'.join(lines[start_idx:])


def generate_robots_h():
    """Generate single-header robots.h."""
    commit, date = get_git_info()

    header_content = read_file(ROBOTS_HEADER)
    source_content = read_file(ROBOTS_SOURCE)

    # Extract implementation
    impl = extract_implementation(source_content, ["robots.h"])

    # Find the closing #endif of the include guard
    lines = header_content.split('\n')

    # Find last #endif
    last_endif_idx = -1
    for i in range(len(lines) - 1, -1, -1):
        if lines[i].strip().startswith('#endif'):
            last_endif_idx = i
            break

    if last_endif_idx == -1:
        print("Error: Could not find #endif in robots.h")
        sys.exit(1)

    # Build implementation block
    impl_block = f'''
// ============================================================================
// IMPLEMENTATION
// ============================================================================
// Generated: {date}
// Commit: {commit or 'unknown'}
//
// Define ROBOTS_IMPLEMENTATION in exactly one source file before including
// this header to include the implementation:
//
//   #define ROBOTS_IMPLEMENTATION
//   #include "robots.h"
//
// ============================================================================

#ifdef ROBOTS_IMPLEMENTATION

{impl}

#endif  // ROBOTS_IMPLEMENTATION

'''

    # Insert implementation before last #endif
    result_lines = lines[:last_endif_idx] + impl_block.split('\n') + lines[last_endif_idx:]
    amalgamated = '\n'.join(result_lines)

    # Add notice at top
    notice = f'''//
// *** AMALGAMATED SINGLE-HEADER VERSION ***
// Generated: {date}
// Commit: {commit or 'unknown'}
//
// This file is auto-generated. Do not edit directly.
// Run: python3 singleheader/amalgamate.py
//
'''

    # Find where to insert (after license comment)
    for i, line in enumerate(result_lines):
        if not line.strip().startswith('//') and line.strip() != '':
            result_lines.insert(i, notice)
            break

    return '\n'.join(result_lines)


def generate_reporting_robots_h():
    """Generate single-header reporting_robots.h with embedded robots.h."""
    commit, date = get_git_info()

    robots_h = read_file(ROBOTS_HEADER)
    robots_cc = read_file(ROBOTS_SOURCE)
    reporting_h = read_file(REPORTING_HEADER)
    reporting_cc = read_file(REPORTING_SOURCE)

    # Remove #include "robots.h" from reporting_robots.h
    reporting_h = remove_include(reporting_h, "robots.h")

    # Strip include guards from robots.h
    robots_h_inner = strip_include_guards(robots_h)

    # Find where to insert robots.h content (after #define GUARD)
    lines = reporting_h.split('\n')
    insert_idx = 0
    for i, line in enumerate(lines):
        if line.strip().startswith('#define ') and '_H_' in line:
            insert_idx = i + 1
            break

    # Insert robots.h content
    robots_section = [
        "",
        "// === Begin embedded robots.h ===",
        robots_h_inner,
        "// === End embedded robots.h ===",
        ""
    ]

    lines = lines[:insert_idx] + robots_section + lines[insert_idx:]
    reporting_h = '\n'.join(lines)

    # Extract implementations
    robots_impl = extract_implementation(robots_cc, ["robots.h"])
    reporting_impl = extract_implementation(reporting_cc, ["reporting_robots.h", "robots.h"])

    # Find last #endif
    lines = reporting_h.split('\n')
    last_endif_idx = -1
    for i in range(len(lines) - 1, -1, -1):
        if lines[i].strip().startswith('#endif'):
            last_endif_idx = i
            break

    if last_endif_idx == -1:
        print("Error: Could not find #endif in reporting_robots.h")
        sys.exit(1)

    impl_block = f'''
// ============================================================================
// IMPLEMENTATION
// ============================================================================
// Generated: {date}
// Commit: {commit or 'unknown'}
//
// Define ROBOTS_IMPLEMENTATION in exactly one source file before including
// this header to include the implementation:
//
//   #define ROBOTS_IMPLEMENTATION
//   #include "reporting_robots.h"
//
// ============================================================================

#ifdef ROBOTS_IMPLEMENTATION

// === Begin robots.cc implementation ===
{robots_impl}
// === End robots.cc implementation ===

// === Begin reporting_robots.cc implementation ===
{reporting_impl}
// === End reporting_robots.cc implementation ===

#endif  // ROBOTS_IMPLEMENTATION

'''

    lines = lines[:last_endif_idx] + impl_block.split('\n') + lines[last_endif_idx:]
    amalgamated = '\n'.join(lines)

    # Add notice
    notice = f'''//
// *** AMALGAMATED SINGLE-HEADER VERSION ***
// Generated: {date}
// Commit: {commit or 'unknown'}
//
// This file is auto-generated. Do not edit directly.
// Run: python3 singleheader/amalgamate.py
//
'''

    lines = amalgamated.split('\n')
    for i, line in enumerate(lines):
        if not line.strip().startswith('//') and line.strip() != '':
            lines.insert(i, notice)
            break

    return '\n'.join(lines)


def generate_robots_c_h():
    """Generate single-header robots_c.h with C API."""
    commit, date = get_git_info()

    robots_h = read_file(ROBOTS_HEADER)
    robots_cc = read_file(ROBOTS_SOURCE)
    robots_c_h = read_file(ROBOTS_C_HEADER)
    robots_c_cc = read_file(ROBOTS_C_SOURCE)

    # Strip include guards from robots.h
    robots_h_inner = strip_include_guards(robots_h)

    # Remove #include "robots_c.h" and "robots.h" from robots_c.cc
    robots_c_cc = remove_include(robots_c_cc, "robots_c.h")
    robots_c_cc = remove_include(robots_c_cc, "robots.h")

    # Find where to insert robots.h content (after #define ROBOTS_C_H)
    lines = robots_c_h.split('\n')
    insert_idx = 0
    for i, line in enumerate(lines):
        if line.strip().startswith('#define ') and 'ROBOTS_C_H' in line:
            insert_idx = i + 1
            break

    # We need to wrap C++ parts for C compatibility
    cpp_section = [
        "",
        "#ifdef __cplusplus",
        "// === Begin embedded robots.h (C++ only) ===",
        robots_h_inner,
        "// === End embedded robots.h ===",
        "#endif  // __cplusplus",
        ""
    ]

    lines = lines[:insert_idx] + cpp_section + lines[insert_idx:]
    robots_c_h = '\n'.join(lines)

    # Extract implementations
    robots_impl = extract_implementation(robots_cc, ["robots.h"])
    robots_c_impl = extract_implementation(robots_c_cc, ["robots_c.h", "robots.h"])

    # Find last #endif
    lines = robots_c_h.split('\n')
    last_endif_idx = -1
    for i in range(len(lines) - 1, -1, -1):
        if lines[i].strip().startswith('#endif') and 'ROBOTS_C_H' in lines[i]:
            last_endif_idx = i
            break
    if last_endif_idx == -1:
        for i in range(len(lines) - 1, -1, -1):
            if lines[i].strip().startswith('#endif'):
                last_endif_idx = i
                break

    impl_block = f'''
// ============================================================================
// IMPLEMENTATION (C++ required for implementation)
// ============================================================================
// Generated: {date}
// Commit: {commit or 'unknown'}
//
// Define ROBOTS_IMPLEMENTATION in exactly one C++ source file before including
// this header to include the implementation:
//
//   #define ROBOTS_IMPLEMENTATION
//   #include "robots_c.h"
//
// Note: The implementation requires C++, but the API can be called from C.
// ============================================================================

#if defined(ROBOTS_IMPLEMENTATION) && defined(__cplusplus)

// === Begin robots.cc implementation ===
{robots_impl}
// === End robots.cc implementation ===

// === Begin robots_c.cc implementation ===
{robots_c_impl}
// === End robots_c.cc implementation ===

#endif  // ROBOTS_IMPLEMENTATION && __cplusplus

'''

    lines = lines[:last_endif_idx] + impl_block.split('\n') + lines[last_endif_idx:]
    amalgamated = '\n'.join(lines)

    # Add notice
    notice = f'''//
// *** AMALGAMATED SINGLE-HEADER VERSION ***
// Generated: {date}
// Commit: {commit or 'unknown'}
//
// This file is auto-generated. Do not edit directly.
// Run: python3 singleheader/amalgamate.py
//
'''

    lines = amalgamated.split('\n')
    for i, line in enumerate(lines):
        if not line.strip().startswith('//') and line.strip() != '':
            lines.insert(i, notice)
            break

    return '\n'.join(lines)


def main():
    print("Generating robots.h...")
    robots = generate_robots_h()
    output_robots = os.path.join(OUTPUT_DIR, "robots.h")
    with open(output_robots, "w", encoding="utf-8") as f:
        f.write(robots)
    print(f"  -> {output_robots}")

    print("Generating robots_c.h...")
    robots_c = generate_robots_c_h()
    output_robots_c = os.path.join(OUTPUT_DIR, "robots_c.h")
    with open(output_robots_c, "w", encoding="utf-8") as f:
        f.write(robots_c)
    print(f"  -> {output_robots_c}")

    print("Generating reporting_robots.h...")
    reporting = generate_reporting_robots_h()
    output_reporting = os.path.join(OUTPUT_DIR, "reporting_robots.h")
    with open(output_reporting, "w", encoding="utf-8") as f:
        f.write(reporting)
    print(f"  -> {output_reporting}")

    print("Done!")


if __name__ == "__main__":
    main()
