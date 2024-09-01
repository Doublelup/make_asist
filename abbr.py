#!/usr/bin/env python

import re
import sys
import pdb

defdir_pattern = re.compile(r'^defdir\s*(#.*)?$')
endef_pattern = re.compile(r'^endef\s*(#.*)?$')
make_rely_pattern = re.compile(r'^(.*):(.*)\s*(#.*)?$')
prefix_rule_pattern = re.compile(r'^(.*):(.*)\s*(#.*)?$')
comment_pattern = re.compile(r'^\s*#.*$')
empty_pattern = re.compile(r'^\s*$')
noextend_pattern = re.compile(r'^\s*NOEXTEND\s+(.*)\s*(#.*)?$')
make_statement_pattern = re.compile(r'^\s*MAKE\s+(.*)$')
raw_statement_pattern = re.compile(r'^\s*RAW\s+(.*)$')

def parser(input_file: str):
	in_def = False
	prefix_dict = {}
	rule_dict = {}
	noextend_list = []
	make_statement_list = []
	raw_statement_list = []
	with open(input_file, 'r') as file:
		for index, line in enumerate(file):
			line = line.strip()
			if comment_pattern.match(line):
				continue
			elif empty_pattern.match(line):
				continue
			elif noextend_pattern.match(line):
				noextend_items = noextend_pattern.match(line).group(1).split()
				noextend_list += noextend_items
				continue
			elif make_statement_pattern.match(line):
				make_statement_list.append(make_statement_pattern.match(line).group(1).strip())
				continue
			elif raw_statement_pattern.match(line):
				raw_statement_list.append(raw_statement_pattern.match(line).group(1).strip())
				continue
			else:
				if not in_def:
					if defdir_pattern.match(line):
						in_def = True
					elif make_rely_pattern.match(line):
						m = make_rely_pattern.match(line)
						targets = m.group(1).strip().split()
						if len(targets) == 0:
							raise ValueError(f"Line {index + 1}: Lack of target")
						prerequisites = m.group(2).strip().split()
						for target in targets:
							prereq_list = rule_dict.get(target)
							if prereq_list:
								prereq_list += prerequisites
							else:
								rule_dict[target] = prerequisites
					else:
						raise ValueError(f"Line {index + 1}: Syntax error: {line}")
				else:
					if endef_pattern.match(line):
						in_def = False
					elif prefix_rule_pattern.match(line):
						m = prefix_rule_pattern.match(line)
						pattern = m.group(1).strip()
						prefix = m.group(2).strip()
						prefix_num = len(prefix.split())
						if prefix_num != 1:
							raise ValueError(f"Line {index + 1}: Multiple prefix: {prefix}")
						pattern_words = pattern.split()
						for word in pattern_words:
							prefix_dict[word] = prefix
					else:
						raise ValueError(f"Line {index + 1}: Syntax error: {line}")                    
	if in_def:
		raise ValueError(f"defdir unfinish")        
	return prefix_dict, rule_dict, noextend_list, make_statement_list, raw_statement_list

def addprefix(prefix_dict: dict, rule_dict: dict, \
				noextend_list: list, make_statement_list: list, \
				raw_statement_list: list, output_file: str):
	intermidiate_prefix_list = [None] * len(prefix_dict)
	for index, (key, value) in enumerate(prefix_dict.items()):
		pattern = re.compile(key)
		intermidiate_prefix_list[index] = (pattern, value)
	
	intermidiate_noextend_list = [None] * len(noextend_list)
	for index, noextend_item in enumerate(noextend_list):
		pattern = re.compile(noextend_item)
		intermidiate_noextend_list[index] = pattern
	
	item_pattern = re.compile(r'\b[\w.]+\b')

	def full(item):
		for pattern in intermidiate_noextend_list:
			if pattern.match(item):
				return item
		for intermidiate_prefix in intermidiate_prefix_list:
			if intermidiate_prefix[0].match(item):
				full_item = intermidiate_prefix[1] + item
				return full_item
		print(f'No proper prefix for {item}')
		return item

	with open(output_file, 'w') as file:
		for target, prerequisites in rule_dict.items():
			full_rule = ''
			full_rule += full(target) + ' :'
			for prerequisite in prerequisites:
				full_rule += ' ' + full(prerequisite)
			file.write(full_rule + '\n\n')
		for statement in make_statement_list:
			items = item_pattern.finditer(statement)
			start = 0
			new_statement = ''
			for item in items:
				new_statement += statement[start: item.start()]
				new_statement += full(item.group())
				start = item.end()
			file.write(new_statement + '\n\n')
		for statement in raw_statement_list:
			file.write(statement + '\n\n')
	return

def main():
	if len(sys.argv) != 3:
		print(f'argv number error {sys.argv[1:]}')
		return
	input_file = sys.argv[1]
	output_file = sys.argv[2]
	try:
		prefix_dict, rule_dict, noextend_list, make_statement_list, raw_statement_list= parser(input_file)
		addprefix(prefix_dict, rule_dict, noextend_list, \
					make_statement_list, raw_statement_list, output_file)
		print(f'{input_file} translated into {output_file}.')
	except Exception as e:
		print(f'Error: {e}')
	return

if __name__ == "__main__":
	main()
