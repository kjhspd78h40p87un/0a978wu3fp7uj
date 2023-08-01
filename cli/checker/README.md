# Command-line Interface: Checker

## Commands

Since violations are now found as part of the EESI service, there are no
commands related to any checker service. Instead, the only command is related
to listing the violation results that are found by EESIER.

#### List violations

This command lists the violations that are currently stored in MongoDB.

```bash
bazel run //cli:main checker ListViolations [--bitcode-uri <uri>]
```

Sample output for a database with one entry for `hello.ll`:

```bash
---Violations that appear in database---
------------------------------
Bitcode ID (last 8 characters):          File name:                                                                 
70b55c7b                                 hello.ll                                                                   
Function:                                Parent Function:                         Violation:                     Line:     
printf                                   main                                     Unused return value.           3         
Violations total: 1
```

Currently only unused violations are being found by the checker. This may be
updated to account for other various violations.
