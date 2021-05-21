import os
import sys
import subprocess

languages = ["cs", "de", "en","es", "fr", "it", "ja", "ko", "nl", "pl", "pt_br", "ru", "tr", "uk", "zh_cn", "zh_tw"];
print(sys.getrecursionlimit())
for lang in languages:
	if 'y' != input("translating "+lang+"? (y/n): "):
		continue
	# create .po
	#os.system("msgunfmt "+lang+"/Slic3r.mo -o "+lang+"/SuperSlicer.po > nul");
	#os.system("copy ..\\loc\\"+lang+"\\*.po "+lang+"\\PrusaSlicer.po");
	
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

	for file in os.listdir(lang):
		if not file.endswith("Slic3r.po"):
			os.remove(lang + "/" + file);
	# #create .mo
	# os.system("msgfmt "+lang+"/SuperSlicer.po -o "+lang+"/SuperSlicer.mo");
	# #msgfmt Slic3r.po -o Slic3r.mo
