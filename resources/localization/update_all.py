import os
import sys
import subprocess

languages = ["cs", "de", "en","es", "fr", "it", "ja", "ko", "nl", "pl", "pt_br", "ru", "tr", "uk", "zh_cn", "zh_tw"];
print(sys.getrecursionlimit())
for lang in languages:
	if 'y' != input("translating "+lang+"? (y/n): "):
		continue
	# create .po
	os.system("msgunfmt "+lang+"/PrusaSlicer.mo -o "+lang+"/PrusaSlicer.po > nul");

	file_out_stream = open(lang+"/settings.ini", mode="w", encoding="utf-8");
	file_out_stream.write("data = ./PrusaSlicer.po\n");
	file_out_stream.write("ui_dir = ../../ui_layout\n");
	file_out_stream.write("ignore_case = true\n");
	file_out_stream.write("input = ../Slic3r.pot\n");
	file_out_stream.write("todo = ./todo.po\n");
	file_out_stream.write("output = ./Slic3r.po\n");
	#flush
	file_out_stream.close();

	p = subprocess.Popen(["python","../pom_merger.py"], cwd=lang);
	p.wait();

	#create .mo
	os.system("msgfmt "+lang+"/Slic3r.po -o "+lang+"/SuperSlicer.mo");
	# msgfmt Slic3r.po -o Slic3r.mo
