Did you encounter an issue with using Slic3r? Fear not! This guide will help you to write a good bug report in just a few, simple steps.

There is a good chance that the issue, you have encountered, is already reported. Please check the [list of reported issues](https://github.com/alexrj/Slic3r/issues) before creating a new issue report. If you find an existing issue report, feel free to add further information to that report.

If possible, please include the following information when [reporting an issue](https://github.com/alexrj/Slic3r/issues/new):
* Slic3r version (See the about dialog for the version number. If running from git, please include the git commit ID from `git rev-parse HEAD` also.)
* Operating system type + version
* Steps to reproduce the issue, including:
    * Command line parameters used, if any
    * Slic3r configuration file (Use ``Export Config...`` from the ``File`` menu - please don't export a bundle)
    * Expected result
    * Actual result
    * Any error messages
* If the issue is related to G-code generation, please include the following:
    * STL, OBJ or AMF input file (please make sure the input file is not broken, e.g. non-manifold, before reporting a bug)
    * a screenshot of the G-code layer with the issue (e.g. using [Pronterface](https://github.com/kliment/Printrun) or preferably the internal preview tab in Slic3r).
* If the issue is a request for a new feature, be ready to explain why you think it's needed.
    * Doing more prepatory work on your end makes it more likely it'll get done. This includes the "how" it can be done in addition to the "what". 
    * Define the "What" as strictly as you can. Consider what might happen with different infills than simple rectilinear.

Please make sure only to include one issue per report. If you encounter multiple, unrelated issues, please report them as such.

Simon Tatham has written an excellent on article on [How to Report Bugs Effectively](http://www.chiark.greenend.org.uk/~sgtatham/bugs.html) which is well worth reading, although it is not specific to Slic3r.

Do you want to help fix issues in or add features to Slic3r? That's also very, very welcome :)

* A good place to start if you can is to look over the [Pull Request or Bust](https://github.com/alexrj/Slic3r/milestones/Pull%20Request%20or%20Bust) milestone. This contains all of the things (mostly new feature requests) that there isn't time or resources to address at this time. 
     * Things that are probably fixable via scripts (usually marked as such) have the lowest bar to getting something that works, as you don't need to recompile Slic3r to test.
* If you're starting on an issue, please say something in the related issues thread so that someone else doesn't start working on it too.
* If there's nothing in the [Pull Request or Bust](https://github.com/alexrj/Slic3r/milestones/Pull%20Request%20or%20Bust) milestone that interests you, the next place to look is for issues that don't have a milestone. Lots of people commit ideas to Slic3r, and it's difficult to keep up and sort through them.
* Before sending a pull request, please make sure that the changes you are submitting are contained in their own git branch, as PRs merge histories.
     * Pull requests that contain unrelated changes will be rejected.
     * A common workflow is to fork the master branch, create your new branch and do your work there. git-rebase and git-cherry-pick are also helpful for separating out unrelated changes.
* If you are pushing Slic3r code changes that touch the main application, it is very much appreciated if you write some tests that check the functionality of the code. It's much easier to vet and merge in code that includes tests and doing so will likely speed things up.
