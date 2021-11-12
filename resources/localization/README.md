
# How to create / update your own language

Note that Slic3r only supports left-to-right languages (if you want to change that, you're welcome to contribute).

Your language should have [a code](https://www.loc.gov/standards/iso639-2/php/code_list.php) (use the ISO 639-1, second column). Creating a directory and putting a .mo file inside it will add automatically the translation in Slic3r.

You will have to complete a '.po' text file, with a text editor or a specific ide like poedit. When your translation work is finished, you'll have to compile it into a '.mo' file, this one is not readable with a text editor but is readable by the software.

If your language is already here, you remove all current files to start from scratch or rename them to use them as the start point.

If you found an odd translation and want to change it, go to section B)

## A) Make your own translation

Useful tools:

* windows:
  * [python](https://www.python.org/)
  * [gettext](http://gnuwin32.sourceforge.net/downlinks/gettext.php)
* linux:
  * python: make sure you can execute python3, install it if it isn't here.
  * gettext: if you can't execute msgfmt, install the package 'gettext'
* macos: maybe like linux?

### 1) initialisation
Open the settings.ini
* For each file that can contain useful translation, create/edit a "data" line to point to the said file.
* The 'input' property must be the Slic3r.pot path
* The 'output' must be the Slic3r.po
* The 'todo' contains the path of the po file to complete.

note that the first data line has the priority over the other ones (the first translation encountered is the one used)

In this example, we are going to update the Spanish translation.  
We are going to use your old translation file and the current PrusaSlicer one.  
To decompile the .mo of Prusaslicer, use the command `msgunfmt PrusaSlicer.mo -o PrusaSlicer.po`.  
So the settings.ini contains these lines :

```
data = es/Slic3r.po
data = MyKnowledgeBase.po
data = es/PrusaSlicer_es.po

database_out = MyKnowledgeBase.po

ui_dir = ../ui_layout
allow_msgctxt = false
ignore_case = false
remove_comment = true
percent_error_similar = 0.4
max_similar = 3
language = french
language_code = fr

input = Slic3r.pot
todo = es/todo.po
output = es/Slic3r.po
```

Notes:
* the 'todo' and 'output' files will be erased, so be sure you didn't use this file name (or copy it elsewhere). Note that often, Slic3r.po is used as input & output, as it's the "updated" file.
* the file at 'database_out' will receive all the translations created from the data files. That way, it will keep your new & old unused translations just in case the wording revert back to it, or to be used as reference for the helper.
* ui_dir should be the path to the slic3r/resources/ui_layout directory. If you're in slic3r/resources/localization, this value is good.
* allow_msgctxt is a bool to allow to keep the msgctxt tags. You need a recent version of gettext to use that.
* ignore_case is a bool that will let the tool to ignore the case when comparing msgid if no translation is found.
* remove_comment is a bool that will remove all comments in the output file. It's to avoid unecessary changes in the git commit. Don't submit pull request for '.po' file with comments. (Note that the translations are ordered by alphabetic order) 
* percent_error_similar is a number between 0 and 1. This will activate the helper that will write help comment in the TODO file. These will present similar string that are already translated, to let you pick chunk that are already translated to avoid redoing all the work. It's the percentage of difference allowed (0 = identical, 1 = everything, 0.5 = not more than half of the string is different), using (levenshtein distance / msgid length). You have to install the levenshtein lib in python to use it.
* max_similar: max number of help translation per item
* language and language_code: text to include in the header.


### 2) launch the utility.
* You need to have python (v3). You can download it if you don't.
* Open a console
* cd into the localization directory,
* execute 'python pom_merger.py'
  * use python3 if python is the python v2 exe
  * you can use the full path to python.exe if you just installed it and it isn't in your path. It's installed by default in your appdata on windows.
It will tell you if you made some mistakes about the paths, the number of translations reused and the number to do.

### 3) complete the translation file
Then, you have to open the es/todo.po file and complete all the translations. 
* msgid lines are the English string
* msgstr is the translated one, should be an empty string ("").

Important: 
* you must write it in one line, use \n to input a line change. 
* the %1, %2, ... MUST be put also in the translation, as it's a placeholder for input numbers, if one is missed the software will crash, so be careful. '%%' is the way to write '%' without making the program crash. The utility should warn you about every translation that has a different number of '%'.


### 4) relaunch the utility

You can copy/save the todo.po in another file in case of.
After filling the todo file, change the settings.ini:

```
data = es/todo.po
data = MyKnowledgeBase.po

database_out = MyKnowledgeBase.po

ui_dir = ../ui_layout
allow_msgctxt = false
ignore_case = false
remove_comment = true
percent_error_similar = 0.4
max_similar = 3
language = french
language_code = fr

input = Slic3r.pot
todo = es/todo.po
output = es/Slic3r.po
```

This will tell you to use your todo and your newly created/edited Slic3r.po before using the other older file to complete unfound strings. A translation from the third file won't erase the one from the second unless it's empty (less than 3 characters) and the new one isn't. If a translation is replaced, the tool will inform you.

And re-launch the utility.

Repeat (if needed) until you have almost nothing left in your todo.po file (one-letter translation like "." are not copied by the utility)

When you're finished, compile your .po with the command `msgfmt Slic3r.po -o Slic3r.mo`.
Then you can launch Slic3r to test it.
Note that you have to rename it to SuperSlicer.mo if you're using Superslicer and same for PrusaSlicer.

## B) Update an existing .po file

* Open the problematic .po language file in a text editor.
* Search the offending sentence.
* *Modify the translated sentence (at the right of 'msgstr').
  * Note that you must keep a '"' at each side of every line, but you can concatenate the multiple lines into one if you prefer.
  * You can use '\n' to add a 'newline' into your sentence.
* When finished, compile your .po into a .mo and replace the current .mo
* Open Slic3r and check that your sentence works. If Slic3r crashes, it's probably wrong.
* Submit your change by opening an issue (search before for 'is:issue translation' in the issue search bar, to piggyback an existing one if it exists).
