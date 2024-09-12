#!/usr/bin/env python3

import os;
import sys;
import tkinter;
import subprocess;
import re;

from PIL import Image, ImageTk, ImageDraw, ImageFont;
from collections import deque;

frame_pattern = re.compile(r'frame_(\d+).png');

def get_anim_list(root_path):
	objects = set();
	dirs = deque([root_path]);

	while dirs:
		current_path = dirs.popleft();
		for entry in os.scandir(current_path):
			if current_path in objects:
				continue;

			if entry.is_dir():
				dirs.append(entry.path);
			else:
				# If has frame_ in the name, it's a frame add it to the list of directories
				if frame_pattern.match(entry.name):
					objects.add(current_path);

	return objects;

def start_animation(path):

	show = "./show.py";
	process = subprocess.Popen([show, path], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, stdin=subprocess.DEVNULL, close_fds=True)

def main():
	anim_list = list(get_anim_list("./"));
	anim_list.sort();

	print("To run shpw.py, you need to have the following dependencies installed:");
	print("sudo apt install -y python3-pip python3-tk python3-pil python3-pil.imagetk");

	print("Animations found: " + str(len(anim_list)));

	for entry in anim_list:
		print(entry);

	if not anim_list:
		print("No animations found.");
		return None;

	# Now we create a window for the user to select an animation and start it
	l_window = tkinter.Tk();
	l_window.title("Select an animation");

	# Buttons with text of the animation name
	for index, entry in enumerate(anim_list):
		button = tkinter.Button(l_window, text=entry, command=lambda e=entry: start_animation(e));
		button.pack();

	l_window.mainloop();

	return None;

if __name__ == '__main__':
	main();
	sys.exit(0);