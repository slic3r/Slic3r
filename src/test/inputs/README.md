Directory containing test-specific input files that are not source code.

---

Rules
===
* Each folder shall be named for the test that it supports. 
   * `test_config` directory supports `test_config.cpp`, etc.
* If a specific test file is reused across multiple tests, it should go in the `common` directory.
* Each extra input file should be named in such a way that it is relevant to the specific feature of it. 
* No files should have special characters in the name (unless those special characters are part of the test). 
* No spaces should be in the file paths (again, unless testing the presence of spaces is part of the test).
* Input files that are 3D models should be as small/specific as possible.
    * Do not add copyrighted models without permission from the copyright holder. 
    * Do not add NonCommercial models, these would need to be relicensed from the original holder.
    * Add any necessary licensing or attributation information as `<filename>.license`
        * Example: A CC-By-SA 3D model called `cool_statue_bro.stl` should have its attributation/license information included in a file called `cool_statue_bro.license`
        * Any submitted files without an accompanying `.license` file are assumed to be licensed under [CC-By-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/us/).
* The author of a commit adding any extra input files asserts that, via the process of committing those files, that they 
    * have abided by the terms of all licensing agreements involved with the files added or 
    * are the author of the files in question and submit them to the Slic3r project under the terms of [CC-By-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/us/).
