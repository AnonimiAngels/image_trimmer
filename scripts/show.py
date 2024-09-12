#!/usr/bin/env python3

import os;
import sys;
import tkinter;

from PIL import Image, ImageTk, ImageDraw, ImageFont;
from collections import deque;


target_fps = 30;
is_loop = True;
bg_color = '#000000';

frame_list = list();
m_window = tkinter.Tk();

def get_files(root_path):
	objects = set();
	dirs = deque([root_path]);

	while dirs:
		current_path = dirs.popleft();
		for entry in os.scandir(current_path):
			if entry.is_dir():
				continue;
			else:
				objects.add(entry.path);

	return objects;

def gather_frames(path):
	objects = get_files(path);

	return [obj for obj in objects if obj.endswith('.png')];

def generate_frames():
	global frame_list;
	root_path = "./";

	if len(sys.argv) > 1:
		root_path = sys.argv[1];

	frames = gather_frames(root_path);
	frames.sort();

	for index, frame in enumerate(frames):
		image = Image.open(frame);
		image = image.convert('RGBA');
		image.apply_transparency();

		draw = ImageDraw.Draw(image);
		font = ImageFont.load_default();
		draw.text((0, 0), str(index), font=font, fill=(255, 255, 255, 255));

		frame_list.append(ImageTk.PhotoImage(image));


	return None;

def create_window():
	global m_window, frame_list;
	parent_dir = os.path.basename(os.path.normpath(os.path.dirname(os.path.abspath(__file__))));

	if len(sys.argv) > 1:
		parent_dir = sys.argv[1];

	m_window.title(parent_dir);
	m_window.configure(bg=bg_color);

	size = frame_list[0].width(), frame_list[0].height();
	m_window.geometry('{}x{}'.format(size[0], size[1]));

	return None;

def show_frames():
	global frame_list, m_window, target_fps, is_loop, bg_color;


	label = tkinter.Label(m_window, bg=bg_color, background=bg_color);
	label.pack();

	def update_frame(index):
		frame = frame_list[index];
		label.config(image=frame, bg=bg_color, background=bg_color);
		index += 1;

		if index >= len(frame_list):
			if is_loop:
				index = 0;
			else:
				return;

		m_window.after(int(1000 / target_fps), update_frame, index);

	update_frame(0);
	m_window.mainloop();

	return None;

def main():
	global m_window;

	generate_frames();
	create_window();
	show_frames();

	m_window.mainloop();
	return None;

if __name__ == '__main__':
	main();
	sys.exit(0);