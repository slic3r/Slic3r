Directory containing test-specific input files that are not source code.

===

Rules
===
* Each folder shall be named for the test that it supports. 
   * `test_config` directory supports `test_config.cpp`, etc.
* If a specific test file is reused across multiple tests, it should go in the `common` directory.
* Each extra input file should be named in such a way that it is relevant to the specific feature of it. 
* No files should have special characters in the name (unless those special characters are part of the test). 
* No spaces should be in the file paths (again, unless testing the presence of spaces is part of the test).
