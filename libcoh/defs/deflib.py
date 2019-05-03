#!/usr/bin/env python

import array

class Field:
	def __init__(self):
		self.order = 0
		self.name = None
		self.type = None
		self.isarray = False
		self.isrecord = False
		self.isflags = False
		self.enum = {}
		self.enumkey = None
		self.size = None
		self.flags = set()

class Def:
	def __init__(self, fn):
		self.name = None
		self.variants = []
		self.variantof = None
		self.namehints = []
		self.autodetect = False
		self.simple = False
		self.fields = []
		self.fieldhash = {}
		self.depends = []
		order = 0
		fp = open(fn)

		dephash = {}
		for line in fp:
			newf = Field()
			f = line.split()

			if not len(f) > 0:
				continue

			cmd = f[0].lower()
			if cmd[0] == "#":
				continue
			if cmd == "def":
				self.name = f[1]
				continue
			if cmd == "namehint":
				self.namehints.append(f[1])
				continue
			if cmd == "autodetect":
				self.autodetect = True
				continue
			if cmd == "simple":
				self.simple = True
				continue
			if cmd == "variantof":
				self.variantof = f[1]
				self.depends.append(f[1])
				continue
			if cmd == "enumval":
				self.fields[-1].enum[f[1]] = f[2]
				continue
			if cmd == "enumkey":
				self.fields[-1].enumkey = f[1]
				continue
			if cmd == "enumsameas":
				self.fields[-1].enum = self.fieldhash[f[1]].enum
				continue
			sz = len(f)
			for i in range(0, sz):
				if f[i][0] == '#':
					break
				if f[i][0] == '*':
					newf.flags.add(f[i][1:].lower())
				elif newf.name is None:
					newf.name = f[i]
				elif newf.type is None:
					tf = f[i].split(':')
					newf.type = tf.pop(0)
					if newf.type == 'array':
						newf.type = tf.pop(0)
						newf.isarray = True
					if newf.type == 'record':
						newf.type = tf.pop(0)
						newf.isrecord = True
					if newf.isrecord and not newf.type in dephash:
						dephash[newf.type] = True
						self.depends.append(newf.type)
					if len(tf) > 0:
						newf.size = tf[0]
				else:
					print("Parse error: %s" % (f[i]))
			self.order = order
			order = order + 1
			self.fields.append(newf)
			self.fieldhash[newf.name] = newf

class DefEngine:
	def __init__(self):
		self.filelist = []
		self.alldefs = {}
		self.deforder = []

	def setfiles(self, fl):
		self.filelist = fl
		self.mkorder()
	
	def mkorder(self):
		deplist = []
		nondep = {}
		self.alldefs = {}
		for fn in self.filelist:
			dfn = Def(fn)
			dfn.visited = False
			deplist.extend(dfn.depends)
			self.alldefs[dfn.name] = dfn
			nondep[dfn.name] = dfn
		for dep in deplist:
			if dep in nondep:
				del nondep[dep]

		def visit(n):
			if not n.visited:
				n.visited = True
				if n.variantof:
					self.alldefs[n.variantof].variants.append(n.name)
				for dep in n.depends:
					if dep in self.alldefs:
						visit(self.alldefs[dep])
				self.deforder.append(n)

		for _, node in nondep.items():
			visit(node)

class Def2CLike(DefEngine):
	def __init__(self, includes, namespace):
		DefEngine.__init__(self)
		self.includes = []
		for inc in includes:
			if not len(inc) > 0:
				continue
			if inc[0] != '<':
				inc = '"' + inc + '"'
			self.includes.append(inc)
		self.namespace = namespace

	def NameComponents(self, name):
		return name.split('/')
	def NameFilter(self, name):
		return name.replace('/', '::')

class Def2H(Def2CLike):
	def __init__(self, hname, includes, namespace):
		Def2CLike.__init__(self, includes, namespace)
		self.hname = hname
	
	def before(self):
		print('#ifndef __' + self.hname.upper() + '_H__')
		print('#define __' + self.hname.upper() + '_H__')
		print('')
		for incl in self.includes:
			print('#include ' + incl)
		print('')
		print('/* Do not modify this file by hand! */')
		print('')
		for ns in self.namespace:
			print('namespace ' + ns + ' {')
		if len(self.namespace) > 0:
			print('')
	
	def after(self):
		for ns in self.namespace:
			print('}')
		if len(self.namespace) > 0:
			print('')
		print('#endif')

class Def2CC(Def2CLike):
	def __init__(self, includes, namespace):
		Def2CLike.__init__(self, includes, namespace)
	
	def before(self):
		for incl in self.includes:
			print('#include ' + incl)
		print('')
		print('/* Do not modify this file by hand! */')
		print('')
		for ns in self.namespace:
			print('namespace ' + ns + ' {')
		if len(self.namespace) > 0:
			print('')
	
	def after(self):
		for ns in self.namespace:
			print('}')

class Def2PY(DefEngine):
	def before(self):
		print('#!/usr/bin/env python')
		print('')
		print('# Do not modify this file by hand!')
		print('')

	def after(self):
		pass
