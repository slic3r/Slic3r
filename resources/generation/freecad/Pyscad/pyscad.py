#########################################################
# This file is distributed against the LGPL licence     #
# This file is made by supermerill (Remi Durand)        #
#########################################################

######################################
# print("You can use something like 'from ", __name__," import *' for a more convenient experience")
#
# usage: scene().show( many statement )
# if you want to make a function, don't forget to return the root node!
# Each function like cube() or union() return an object (EasyNode) 
#  which store the know-how of how to build the object tree if needed.
# The scene will only redraw a node and these children if one of the 
#  child is changed. 
#
#
# move() rotate() scale() color() return a different kind of object
#  because they are not a mod but a modifier onto a node. They can 
#  be use almost like the other one, no ned to worry.
#
# Color, multmatrix and scale can't be applied on a node so they are
#  passed to the leafs.
#
# more info on the github wiki
######################################

import Part, FreeCAD, math, Draft, InvoluteGearFeature, importSVG, Mesh
from FreeCAD import Base

######### basic functions #########
def _getColorCode(str):
	if(str == "red"):		return (1.0,0.0,0.0,0.0)
	if(str == "lime"):		return (0.0,1.0,0.0,0.0)
	if(str == "blue"):		return (0.0,0.0,1.0,0.0)
	if(str == "cyan"):		return (0.0,1.0,1.0,0.0)
	if(str == "aqua"):		return (0.0,1.0,1.0,0.0)
	if(str == "fuchsia"):	return (1.0,0.0,1.0,0.0)
	if(str == "yellow"):	return (1.0,1.0,0.0,0.0)
	if(str == "black"):		return (0.0,0.0,0.0,0.0)
	if(str == "black"):		return (0.0,0.0,0.0,0.0)
	if(str == "gray"):		return (0.5,0.5,0.5,0.0)
	if(str == "silver"):	return (.75,.75,.75,0.0)
	if(str == "white"):		return (  1,  1,  1,0.0)
	if(str == "maroon"):	return (0.5,0.0,0.0,0.0)
	if(str == "green"):		return (0.0,0.5,0.0,0.0)
	if(str == "navy"):		return (0.0,0.0,0.5,0.0)
	if(str == "teal"):		return (0.0,0.5,0.5,0.0)
	if(str == "purple"):	return (0.5,0.0,0.5,0.0)
	if(str == "olive"):		return (0.5,0.5,0.0,0.0)
	return (0.5,0.5,0.5,0.5)
	
def _getTuple2(a,b,default_first_item):
	if(isinstance(a, list)):
		if(len(a)==0):
			a=default_first_item
		elif(len(a)==1):
			a=a[0]
		elif(len(a)==2):
			b=a[1]
			a=a[0]
	return (a,b)
def _getTuple2Abs(a,b,default_first_item):
	(a,b) = _getTuple2(a,b,default_first_item)
	return (abs(a), abs(b))
def _getTuple3(a,b,c,default_first_item):
	if(isinstance(a, list)):
		if(len(a)==0):
			a=default_first_item
		elif(len(a)==1):
			a=a[0]
		elif(len(a)==2):
			b=a[1]
			a=a[0]
		elif(len(a)==3):
			c=a[2]
			b=a[1]
			a=a[0]
	return (a,b,c)
def _getTuple3Abs(a,b,c,default_first_item):
	(a,b,c) = _getTuple3(a,b,c,default_first_item)
	return (abs(a),abs(b),abs(c))
def _getTuple4(a,b,c,d,default_first_item):
	if(isinstance(a, list)):
		if(len(a)==0):
			a=default_first_item
		elif(len(a)==1):
			a=a[0]
		elif(len(a)==2):
			b=a[1]
			a=a[0]
		elif(len(a)==3):
			c=a[2]
			b=a[1]
			a=a[0]
		elif(len(a)==4):
			d=a[3]
			c=a[2]
			b=a[1]
			a=a[0]
	return (a,b,c,d)
def _getTuple4Abs(a,b,c,d,default_first_item):
	(a,b,c,d) = _getTuple4(a,b,c,d,default_first_item)
	return (abs(a),abs(b),abs(c),abs(d))
	
def _moy_vect(points=[]):
	moy = [0.0,0.0,0.0]
	nb=0
	for p in points:
		moy[0]+=p[0]
		moy[1]+=p[1]
		moy[2]+=p[2]
		nb+=1
	moy[0]/=nb
	moy[1]/=nb
	moy[2]/=nb
	return moy

######### scene class #########

if(not 'tree_storage' in globals()):
	global tree_storage
	tree_storage = {}

class Scene:
	def __init__(self):
		doc = FreeCAD.ActiveDocument
	#when draw() don't refresh correctly
	def redraw(self, *tupleOfNodes):
		for obj in FreeCAD.ActiveDocument.Objects :
			FreeCAD.ActiveDocument.removeObject(obj.Name)
			
		for node in tupleOfNodes:
			node.create()
		global tree_storage
		tree_storage[FreeCAD.ActiveDocument] = tupleOfNodes
		FreeCAD.ActiveDocument.recompute()
	def draw(self, *tupleOfNodes):
		global tree_storage
		try:
			if(not FreeCAD.ActiveDocument in tree_storage):
				tree_storage[FreeCAD.ActiveDocument] = []
			keep_nodes = []
			i=0
			doc = FreeCAD.ActiveDocument
			newArrayOfNodes = []
			# This algo can't support a shuffle (or insert) of new object in the root.
			# But it works
			for node_idx in range(len(tupleOfNodes)):
				if(len(tree_storage[FreeCAD.ActiveDocument])<=node_idx):
					newArrayOfNodes.append(tupleOfNodes[node_idx].create())
				else:
					newArrayOfNodes.append(tupleOfNodes[node_idx].check(doc, tree_storage[FreeCAD.ActiveDocument][node_idx]))
				node_idx+=1
			if(len(tree_storage[FreeCAD.ActiveDocument])>len(tupleOfNodes)):
				for node_idx in range(len(tupleOfNodes), len(tree_storage[FreeCAD.ActiveDocument])):
					tree_storage[FreeCAD.ActiveDocument][node_idx].destroyObj(doc)
		except:
			print("Unexpected error:")
		# if True:
			tree_storage[FreeCAD.ActiveDocument] = []
			for obj in doc.Objects :
				doc.removeObject(obj.Name)
			FreeCAD.ActiveDocument.recompute()
			raise
		tree_storage[FreeCAD.ActiveDocument] = tuple(newArrayOfNodes)
		FreeCAD.ActiveDocument.recompute()
		return self;
	def __call__(self, *tupleOfNodes):
		return self.draw(*tupleOfNodes)

def scene():
	return Scene()

######### Basic class a utilities ######### 

_idx_EasyNode = 0
_default_size = 1

class EasyNode:
	def __init__(self):
		self.childs = []
		self.actions = []
		self.actions_after = []
		self.simple = True
	def addAction(self, method, args):
		self.actions.append((method,args))
	def __hash__(self):
		if(hasattr(self, '_hash')):
			if(self._hash):
				return 0
			else:
				self._hash = True
		else:
			self._hash = True
		hashval = 0
		i=0
		for action in self.actions:
			# print("action="+str(action))
			hashval = hashval + (hash(action[0].__name__))
			i+=517
			for arg in action[1]:
				if(isinstance(arg, EasyNode) or isinstance(arg, tuple) or isinstance(arg, list)):
					temp = 0
				else:
					temp = (hash(arg)*i)
				hashval = (hashval+temp) if temp<10000 else hashval ^ temp
				i+=317
		return hashval
	def printhash(self):
		hashval = 0
		i=0
		for action in self.actions:
			hashval = hashval + (hash(action[0].__name__))
			print("hash of act "+action[0].__name__+" = ",str(hash(action[0].__name__))+" => "+str(hashval))
			i+=517
			
			for arg in action[1]:
				print(isinstance(arg,EasyNode))
			for arg in action[1]:
				# print("arg : "+str(arg)+" "+str( isinstance(arg, EasyNode)))
				# try:
					# print(dir(arg))
				# except:
					# print("not a class")
				if(isinstance(arg, EasyNode) or isinstance(arg, tuple)):
					temp = 0
					print("arg passed")
				else:
					temp = (hash(arg)*i)
					hashval = (hashval+temp) if temp<10000 else hashval ^ temp
					print("hash of arg "+str(arg)+" = "+str(temp*i)+" => "+str(hashval))
				i+=317
		print("final => "+str(hashval))
		return hashval
	def hashWhithChilds(self):
		hashval = hash(self)
		i=0;
		for child in self.childs:
			hashval = hashval ^ (child.hashWhithChilds()*i)
			i+=317
		return hashval
	def create(self):
		for action in self.actions :
			action[0](*action[1])
		for action in self.actions_after :
			action[0](*action[1])
		return self
	def destroyObj(self, doc):
		if(hasattr(self,'obj')):
			print("destroy "+self.obj.Name)
			doc.removeObject(self.obj.Name)
		for child in self.childs:
			child.destroyObj(doc)
	# return the node to use
	def check(self, doc, clone):
		print("check "+self.name)
		ok = True
		if(self.simple):
			#must be done before, to update things?
			hash(self)
			hash(clone)
			if(hash(self) == hash(clone) and len(self.childs) == len(clone.childs)):
				i=0
				for item in self.childs:
					print(hash(self.childs[i])," == ",hash(clone.childs[i]))
					ok = ok and hash(self.childs[i]) == hash(clone.childs[i])
					# if(not hash(self.childs[i]) == hash(clone.childs[i])):
						# self.childs[i].printhash()
						# clone.childs[i].printhash();
						
					i+=1
				print("simple, same hash and childs are ",ok)
				# print(hash(self)," == ",hash(clone))
				# self.printhash()
				# clone.printhash();
			else:
				ok = False
				print("simple but hash: ",hash(self)," != ",hash(clone),hash(self)==hash(clone)," or ",len(self.childs)," != ",len(clone.childs),len(self.childs)==len(clone.childs))
		else:
			ok = (self.hashWhithChilds() == clone.hashWhithChilds())
			print("it's complicated ",ok)
		if(ok):					
			print("keep "+clone.name+" and ditch "+self.name)
			#ok, do not modify it but check the childs
			self.obj = clone.obj
			newNodes = []
			i=0
			ok = True
			for child in self.childs:
				good = self.childs[i].check(doc, clone.childs[i])
				print("use the new one in the old coll? "+str(good == self.childs[i])+" ( "+str(good)+" =?= "+str(self.childs[i]))
				if(not good == clone.childs[i]):
					clone.childs[i] = good
					ok = False
				i+=1
			if(not ok):
				print("reflow "+clone.name)
				clone.clear_childs().layout_childs(*tuple(clone.childs))
			return clone
		else:
			print("recreate "+self.name)
			# redo everything
			clone.destroyObj(doc)
			self.create()
			return self

	def add(self, *tupleOfNodes):
		if(len(tupleOfNodes)==1 and (isinstance(tupleOfNodes[0], list) or isinstance(tupleOfNodes[0], tuple))):
			tupleOfNodes = tuple(tupleOfNodes[0])
		for enode in tupleOfNodes :
			if(isinstance(enode, EasyNode)):
				self.childs.append(enode)
			else:
				print("error, trying to add '"+str(enode)+"' into a union of EsayNode")
		self.actions.append((self.create_childs,tupleOfNodes))
		self.actions.append((self.layout_childs,tupleOfNodes))
		return self
	def __call__(self, *tupleOfNodes):
		return self.add(*tupleOfNodes)
	def create_childs(self, *tupleOfNodes):
		for enode in tupleOfNodes :
			if(isinstance(enode, EasyNode)):
				enode.create()
	def layout_childs(self, *tupleOfNodes):
		arrayShape = self.obj.Shapes
		for enode in tupleOfNodes :
			if(isinstance(enode, EasyNode)):
				arrayShape.append(enode.obj)
				if enode.obj.ViewObject is not None:
					enode.obj.ViewObject.Visibility = False
			else:
				print("error, trying to layout '"+str(enode)+"' into a union of EsayNode")
		self.obj.Shapes = arrayShape
		return self
	def clear_childs(self, *tupleOfNodes):
		self.obj.Shapes = []
		return self
	# def printmyargs(self, *args):
		# print(args)
	def translate(self, x=0,y=0,z=0):
		return move(x,y,z)
	def move(self, x=0,y=0,z=0):
		self.actions.append((self.move_action,(x,y,z)))
		return self
	def move_action(self, x=0,y=0,z=0):
		(x,y,z) = _getTuple3(x,y,z,0)
		self.obj.Placement.move(Base.Vector(x, y, z))
		return self
	def rotate(self, x=0,y=0,z=0):
		self.actions.append((self.rotate_action,(x,y,z)))
		return self
	def rotate_action(self, x=0,y=0,z=0):
		(x,y,z) = _getTuple3(x,y,z,0)
		myMat = Base.Matrix()
		myMat.rotateX(x*math.pi/180)
		myMat.rotateY(y*math.pi/180)
		myMat.rotateZ(z*math.pi/180)
		self.obj.Placement=FreeCAD.Placement(myMat).multiply(self.obj.Placement)
		return self
	def scale(self, x=0,y=0,z=0,_instantly=False):
		self.actions_after.append((self.scale_action,(x,y,z)))
		self.simple = False
		return self
	def scale_action(self, x=0,y=0,z=0):
		for node in self.childs:
			node.scale(x,y,z,_instantly=True)
		return self
	def multmatrix(self, mat):
		self.actions_after.append((self.multmatrix_action,(mat,)))
		self.simple = False
		return self
	def multmatrix_action(self, mat):
		for node in self.childs:
			node.multmatrix(mat)
		return self
	def color(self, r=0.0,v=0.0,b=0,a=0.0,_instantly=False):
		self.actions_after.append((self.color_action,(r,v,b,a)))
		self.simple= False
		return self
	def color_action(self, r=0.0,v=0.0,b=0,a=0.0):
		for node in self.childs:
			node.color(r,v,b,a,_instantly=True)
		self.simple = False
		return self
	def x(self):
		self.actions.append((self.xyz_action,(1,0,0)))
		return self
	def y(self):
		self.actions.append((self.xyz_action,(0,1,0)))
		return self
	def z(self):
		self.actions.append((self.xyz_action,(0,0,1)))
		return self
	def xy(self):
		self.actions.append((self.xyz_action,(1,1,0)))
		return self
	def xz(self):
		self.actions.append((self.xyz_action,(1,0,1)))
		return self
	def yz(self):
		self.actions.append((self.xyz_action,(0,1,1)))
		return self
	def xyz(self):
		self.actions.append((self.xyz_action,(1,1,1)))
		return self
	def xyz_action(self,x,y,z):
		self.move_action(-self.centerx*x,-self.centery*y,-self.centerz*z)
		return self
	def center(self):
		self.actions.append((self.center_action,(1,1,1)))
		return self
	def center_action(self,x,y,z):
		self.move_action(-self.centerx if self.centerx>0 else 0.0,-self.centery if self.centery>0 else 0.0,-self.centerz if self.centerz>0 else 0.0)
		return self

	
def _easyNodeStub(obj, name):
	global _idx_EasyNode
	node = EasyNode()
	_idx_EasyNode += 1
	node.name = name+"_"+str(_idx_EasyNode)
	node.obj = obj
	return node

def useCenter(node, center):
	if(center != None and callable(center)):
		center(node)
	elif(center != None and isinstance(center, bool) and center):
		node.center()

class EasyNodeColored(EasyNode):
	def color(self, r=0.0,v=0.0,b=0.0,a=0.0,_instantly=False):
		if isinstance(r, str):
			if(a!=0.0):
				colorcode = _getColorCode(r)
				colorcode[3] = a
				if(_instantly):
					self.color_action(colorcode)
				else:
					self.actions.append((self.color_action,(colorcode,)))
			else:
				if(_instantly):
					self.color_action(_getColorCode(r))
				else:
					self.actions.append((self.color_action,(_getColorCode(r),)))
		else:
			#fixme: try to see in an array is not hidden inside the r
			(r,v,b,a) = _getTuple4Abs(r,v,b,a,0.0)
			if(_instantly):
				self.color_action((max(0.0,min(r,1.0)),max(0.0,min(v,1.0)),max(0.0,min(b,1.0)),max(0.0,min(a,1.0))))
			else:
				self.actions.append((self.color_action,((max(0.0,min(r,1.0)),max(0.0,min(v,1.0)),max(0.0,min(b,1.0)),max(0.0,min(a,1.0))),)))
		return self
	def color_action(self, colorcode):
		if self.obj.ViewObject is not None:
			self.obj.ViewObject.DiffuseColor = [colorcode]
		return self
def magic_color(node, r=0.0,v=0.0,b=0.0,a=0.0):
		node.color(r,v,b,a)
		return node

class EasyNodeLeaf(EasyNodeColored):
	# def __init__(self):
		# self.childs = []
		# self.actions = []
		# self.actions_after = []
		# self.simple = True
		# self.centerx=0.0
		# self.centery=0.0
		# self.centerz=0.0
	def scale(self, x=0.0,y=0.0,z=0,_instantly=False):
		if(_instantly):
			self.scale_action(x,y,z)
		else:
			self.actions.append((self.scale_action,(x,y,z)))
		return self
	def scale_action(self, x,y,z):
		(x,y,z) = _getTuple3(x,y,z,0.0)
		myMat = Base.Matrix()
		myMat.scale(x,y,z)
		self.obj.Shape = self.obj.Shape.transformGeometry(myMat)
		return self
	def multmatrix_action(self, mat):
		flatarray = []
		for array in mat:
			for num in array:
				flatarray.append(num)
		if matsize != len(flatarray):
			print("error, wrong matrix")
		else:
			myMat = Base.Matrix()
			myMat.A = flatarray
			self.obj.Shape = self.obj.Shape.transformGeometry(myMat)
		return self
		
# def import_)

######### transformations ######### 

class EasyNodeUnion(EasyNode):
	is2D = False
	def layout_childs(self, *tupleOfNodes):
		if(self.is2D):
			arrayobj= []
			for node in self.childs:
				arrayobj.append(node.obj)
			newObj = Draft.upgrade(arrayobj,delete=True)[0][0]
			# for()
			newObj.Placement = Base.Placement(self.obj.Placement)
			FreeCAD.ActiveDocument.removeObject(self.obj.Name)
			self.obj = newObj
		else:
			arrayShape = self.obj.Shapes
			for enode in tupleOfNodes :
				if(isinstance(enode, EasyNode)):
					arrayShape.append(enode.obj)
					if enode.obj.ViewObject is not None:
						enode.obj.ViewObject.Visibility = False
				else:
					print("error, trying to layout '"+str(enode)+"' into a union of EsayNode")
			self.obj.Shapes = arrayShape
		return self

def union(name=None):
	global _idx_EasyNode
	node = EasyNodeUnion()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "union_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createUnion(node,name):
		if(len(node.childs)>0 and isinstance(node.childs[0], EasyNode2D)):
			node.is2D = True
			#Draft.upgrade(*tuple(node.childs))
		node.obj = FreeCAD.ActiveDocument.addObject("Part::MultiFuse", node.name)
	node.addAction(createUnion, (node,name))
	return node
	
def inter(name=None):
	global _idx_EasyNode
	node = EasyNode()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "inter_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createInter(node,name):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::MultiCommon", node.name)
	node.addAction(createInter, (node,name))
	return node
def intersection(name=None):
	return inter(name)

class EasyNodeDiff(EasyNode):
	def __init__(self):
		self.my_union = None
		self.unionChilds = []
		self.childs = []
		self.actions = []
		self.actions_after = []
		self.simple = True
	def add(self, *tupleOfNodes):
		if(len(tupleOfNodes)==1 and (isinstance(tupleOfNodes[0], list) or isinstance(tupleOfNodes[0], tuple))):
			tupleOfNodes = tuple(tupleOfNodes[0])
		good_array = []
		minidx = 0
		newunion = False
		for enode in tupleOfNodes :
			if(isinstance(enode, EasyNode)):
				good_array.append(enode)
		if(len(good_array)==0):
			return self
		if(len(self.childs)==2 and len(self.childs[1].childs)>1):
			# simple case: give them to the union
			for enode in good_array :
				self.childs.append(enode)
				self.unionChilds.append(enode)
		elif( (len(self.childs)<2 or len(self.childs[1].childs)==0) and len(self.childs)+len(good_array)>2):
			# create the union
			self.my_union = union()
			newunion = True
			self.unionChilds = []
			temp = None
			if(len(self.childs)==0):
				self.childs.append(good_array[minidx])
				minidx+=1
			if(len(self.childs)==1):
				self.childs.append(self.my_union)
			else:
				self.unionChilds.append(self.childs[1])
				self.childs[1] = self.my_union
				self.childs.append(self.unionChilds[0])
			for enode in good_array[minidx:] :
				self.childs.append(enode)
				self.unionChilds.append(enode)
		else:
			# just use them
			if(len(self.childs)==0 and len(good_array)>0):
				self.childs.append(good_array[minidx])
				minidx+=1
			if(len(self.childs)==1 and len(good_array)>0):
				self.childs.append(good_array[minidx])
				minidx+=1
			
		if(newunion):
			self.actions.append((self.create_childs,(self.my_union,)+tuple(good_array)))
		else:
			self.actions.append((self.create_childs,tuple(good_array)))
		self.actions.append((self.layout_childs,tuple(good_array)))
		return self
	def layout_childs(self, *tupleOfNodes):
		if(self.obj.Base == None and len(tupleOfNodes)>0):
			self.obj.Base = tupleOfNodes[0].obj
			if tupleOfNodes[0].obj.ViewObject is not None:
				tupleOfNodes[0].obj.ViewObject.Visibility = False
			tupleOfNodes = tupleOfNodes[1:]
		if(self.my_union == None and self.obj.Tool == None and len(tupleOfNodes)>0):
			self.obj.Tool = tupleOfNodes[0].obj
			if tupleOfNodes[0].obj.ViewObject is not None:
				tupleOfNodes[0].obj.ViewObject.Visibility = False
			tupleOfNodes = tupleOfNodes[1:]
		if(self.my_union != None and len(tupleOfNodes)>0):
			self.obj.Tool = self.my_union.obj
			self.my_union.layout_childs(*tupleOfNodes)
		return self

def cut(name=None):
	global _idx_EasyNode
	node = EasyNodeDiff()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "cut_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createCut(node,name):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Cut", node.name)
		node.obj.Base = None
		node.obj.Tool = None
	node.addAction(createCut, (node,name))
	return node
def difference(name=None):
	return cut(name)

# FIXME: The color doesn't apply to the chamfered/fillet section
class EasyNodeChamfer(EasyNode):
	def __init__(self):
		self.edges = [];
		self.childs = []
		self.actions = []
		self.actions_after = []
		self.simple = True
	def layout_childs(self, *tupleOfNodes):
		if(len(tupleOfNodes)>0):
			self.obj.Base = tupleOfNodes[0].obj
			if self.obj.Base.ViewObject is not None:
				self.obj.Base.ViewObject.Visibility = False
			self.obj.Edges = self.edges
		return self
	def set(self, node):
		self.add(node)
		return self
	def addEdge(self, length, *arrayOfEdges):
		if isinstance(length, list):
			if len(length) == 2:
				self.addEdge2(length[0],length[1],*arrayOfEdges)
			else:
				self.addEdge2(length[0],length[0],*arrayOfEdges)
		else:
			self.addEdge2(length,length,*arrayOfEdges)
		return self
	def addEdge2(self, startLength, endLength, *arrayOfEdges):
		for eidx in arrayOfEdges :
			self.edges.append((eidx, startLength, endLength))
		# self.obj.Edges = self.edges
		return self
	def setNb(self, length, nb, node):
		self.set(node)
		for eidx in range(1, nb+1) :
			self.edges.append((eidx, length, length))
		# self.obj.Edges = self.edges
		return self

def chamfer(name=None):
	global _idx_EasyNode
	node = EasyNodeChamfer()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "chamfer_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createChamfer(node):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Chamfer", node.name)
	node.addAction(createChamfer, (node,))
	return node

def fillet(name=None):
	global _idx_EasyNode
	node = EasyNodeChamfer()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "fillet_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createFillet(node):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Fillet", node.name)
	node.addAction(createFillet, (node,))
	return node

class EasyNodeMirror(EasyNodeColored):
	def __init__(self):
		self.edges = [];
		self.childs = []
		self.actions = []
		self.actions_after = []
		self.simple = True
	def layout_childs(self, *tupleOfNodes):
		if(len(tupleOfNodes)>0):
			self.obj.Source = tupleOfNodes[0].obj
			if self.obj.Source.ViewObject is not None:
				self.obj.Source.ViewObject.Visibility = False
		return self

def mirror(x=0.0,y=0.0,z=0.0,name=None):
	(x,y,z) = _getTuple3(x,y,z,0.0)
	global _idx_EasyNode
	node = EasyNodeMirror()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "mirror_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createMirror(node,x,y,z):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Mirroring", node.name)
		node.obj.Base = Base.Vector(0,0,0)
		node.obj.Normal = Base.Vector(x,y,z).normalize()
	node.addAction(createMirror, (node,x,y,z))
	return node

def offset(length=_default_size,fillet=True,fusion=True,name=None):
	global _idx_EasyNode
	node = EasyNodeMirror()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "offset_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createOffset(node, length, fillet, fusion):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Offset", node.name)
		node.obj.Value = length
		node.obj.Mode = 0
		node.obj.Join = 0 if(fillet) else 2
		node.obj.Intersection = fusion
		node.obj.SelfIntersection = False
	node.addAction(createOffset, (node, length, fillet, fusion))
	return node

def offset2D(length=_default_size,fillet=True,fusion=True,name=None):
	global _idx_EasyNode
	node = EasyNodeMirror()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "offset2D_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createOffset2D(node, length, fillet, fusion):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Offset2D", node.name)
		node.obj.Value = length
		node.obj.Mode = 0
		node.obj.Join = 0 if(fillet) else 2
		node.obj.Intersection = fusion
		node.obj.SelfIntersection = False
	node.addAction(createOffset2D, (node, length, fillet, fusion))
	return node

# group: can't be modified, it's just for storage for cleaning the hierarchy
# move & rotate are passed to the childs.
#note: even less tested than the rest
class EasyNodeGroup(EasyNode):
	def layout_childs(self, *tupleOfNodes):
		arrayShape = self.obj.Group
		for enode in tupleOfNodes :
			if(isinstance(enode, EasyNode)):
				arrayShape.append(enode.obj)
				if enode.obj.ViewObject is not None:
					enode.obj.ViewObject.Visibility = True
			else:
				print("error, trying to layout '"+str(enode)+"' into a union of EsayNode")
		self.obj.Group = arrayShape
	def move_action(self, x=0,y=0,z=0):
		print("moveaction group:",x, y, z)
		for enode in self.childs :
			if(isinstance(enode, EasyNode)):
				print(enode.name+" moveaction")
				enode.move_action(x, y, z)
		return self
	def rotate_action(self, x=0,y=0,z=0):
		for enode in self.childs :
			if(isinstance(enode, EasyNode)):
				enode.rotate_action(x, y, z)
		return self
def group(name=None):
	global _idx_EasyNode
	node = EasyNodeGroup()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "group_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createGroup(node,name):
		node.obj = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup", node.name)
	node.addAction(createGroup, (node,name))
	return node


######### 3D objects ######### 


def cube(size=0.0,y=0.0,z=0.0,center=None,x=0.0, name=None):
	(x,y,z,size) = _getTuple4Abs(size,y,z,x,0.0)
	if(x==0.0 and size != 0.0):
		x = size
	if(y==0.0 and z==0.0):
		return box(x,x,x,center, name)
	else:
		return box(x,y if y!=0.0 else _default_size, z if z!=0.0 else _default_size,center, name)

def box(x=_default_size,y=_default_size,z=_default_size,center=None, name=None):
	(x,y,z) = _getTuple3Abs(x,y,z,_default_size)
	global _idx_EasyNode
	node = EasyNodeLeaf()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "cube_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createBox(node, x,y,z,center):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
		node.obj.Shape = Part.makeBox(x, y, z)
		node.centerx = x/2.0
		node.centery = y/2.0
		node.centerz = z/2.0
		useCenter(node, center)
	node.addAction(createBox, (node, x,y,z,center))
	return node

def tri_rect(x=_default_size,y=_default_size,z=_default_size,center=None, name=None):
	(x,y,z) = _getTuple3Abs(x,y,z,_default_size)
	global _idx_EasyNode
	node = EasyNodeLeaf()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "tri_rect_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createTriRect(node, x,y,z,center):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
		poly = Part.makePolygon([Base.Vector(0,0,0),Base.Vector(x,0,0),Base.Vector(0,y,0),Base.Vector(0,0,0)]) 
		node.obj.Shape = Part.Face(poly).extrude(Base.Vector(0,0,z))
		node.centerx = x/2.0
		node.centery = y/2.0
		node.centerz = z/2.0
		useCenter(node, center)
	node.addAction(createTriRect, (node, x,y,z,center))
	return node

def cylinder(r=_default_size,h=_default_size,center=None,d=0.0,r1=0.0,r2=0.0,d1=0.0,d2=0.0,angle=360.0,fn=1,name=None):
	r = abs(r)
	h = abs(h)
	d = abs(d)
	if( (r == _default_size or r == 0) and d != 0.0):
		r = d/2.0
	if( d1!=0.0 and d1==d2 ):
		r = d1/2.0
		d1 = d2 = 0
	if( r1!=0.0 and r1==r2 ):
		r = r1
		r1 = r2 = 0
	if( (r == _default_size or r == 0) and (r1!=0.0 or r2!=0.0 or d1!=0.0 or d2!=0.0)):
		return cone(r1,r2,h,center,d1,d2,fn)
	if(fn>2):
		return poly_ext(r=_default_size, nb=fn, h=h,center=center,d=0.0,name=name)
	global _idx_EasyNode
	node = EasyNodeLeaf()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "cylinder_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createCylinder(node, r,h,center,angle):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
		if(int(angle) == 360):
			node.obj.Shape = Part.makeCylinder(r,h)
		else:
			node.obj.Shape = Part.makeCylinder(r,h,Base.Vector(0,0,0),Base.Vector(0,0,1),angle)
		node.centerx = -r
		node.centery = -r
		node.centerz = h/2.0
		useCenter(node, center)
	node.addAction(createCylinder, (node, r,h,center,angle))
	return node

def cone(r1=_default_size*2,r2=_default_size,h=_default_size,center=None,d1=0.0,d2=0.0,fn=1,name=None):
	r1 = abs(r1)
	r2 = abs(r2)
	h = abs(h)
	d1 = abs(d1)
	if( (r1 == _default_size*2 or r1 == 0) and d1 != 0.0):
		r1 = d1/2.0
	d2 = abs(d2)
	if( (r2 == _default_size or r2 == 0) and d2 != 0.0):
		r2 = d2/2.0
	if(fn>2):
		#compute extrusion angle FIXME: cone(r1=3,r2=1,h=4,fn=4) it amlost a pyramid!
		angle = 0.0
		if(r1>r2):
			angle = -math.atan(float(r1-r2)/float(h))
		else:
			angle = math.atan(float(r2-r1)/float(h))
		#print(str(r1)+" "+str(fn)+" "+str(angle))
		return linear_extrude(height=h,taper=angle)(poly_reg(r1,fn,center=center))
		# return poly_reg(r1,fn,center=center)
	global _idx_EasyNode
	node = EasyNodeLeaf()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "cone_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createCone(node, r1,r2,h,center):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
		node.obj.Shape = Part.makeCone(r1,r2,h)
		node.centerx = -r1 if r1>r2 else -r2
		node.centery = -r1 if r1>r2 else -r2
		node.centerz = h/2.0
		useCenter(node, center)
	node.addAction(createCone, (node, r1,r2,h,center))
	return node

def sphere(r=_default_size,center=None,d=0.0,fn=1,name=None):
	r = abs(r)
	d = abs(d)
	if( (r == _default_size or r == 0.0) and d != 0.0):
		r = d/2.0
	global _idx_EasyNode
	node = EasyNodeLeaf()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "sphere_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createSphere(node, r,center):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
		node.obj.Shape = Part.makeSphere(r)
		node.centerx = -r
		node.centery = -r
		node.centerz = -r
		useCenter(node, center)
	node.addAction(createSphere, (node, r,center))
	return node

def torus(r1=_default_size, r2=0.1,center=None,d1=0.0,d2=0.0,name=None):
	r1 = abs(r1)
	r2 = abs(r2)
	d1 = abs(d1)
	if( (r1 == _default_size or r1 == 0.0) and d1 != 0.0):
		r1 = d1/2.0
	d2 = abs(d2)
	if( (r2 == _default_size or r2 == 0.0) and d2 != 0.0):
		r2 = d2/2.0
	global _idx_EasyNode
	node = EasyNodeLeaf()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "torus_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createTorus(node, r1,r2,center):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
		node.obj.Shape = Part.makeTorus(r1, r2)
		node.centerx = -r1
		node.centery = -r1
		node.centerz = -r2
		useCenter(node, center)
	node.addAction(createTorus, (node, r1,r2,center))
	return node
	

def poly_ext(r=_default_size, nb=3, h=_default_size,center=None,d=0.0,name=None):
	r = abs(r)
	h = abs(h)
	nb= max(3, abs(nb))
	d = abs(d)
	if( (r ==_default_size or r == 0.0) and d != 0.0):
		r = d/2.0
	global _idx_EasyNode
	node = EasyNodeLeaf()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "3Dpolygon_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createPolyExt(node, r,nb,h,center):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
		# create polygon
		points = [Base.Vector(r,0,0)]
		for i in range(1,nb):
			points.append(Base.Vector(r * math.cos(2*math.pi*i/nb), r * math.sin(2*math.pi*i/nb),0))
		points.append(Base.Vector(r,0,0))
		temp_poly = Part.makePolygon(points) 
		node.obj.Shape = Part.Face(temp_poly).extrude(Base.Vector(0,0,h))
		# pol = Draft.makePolygon(nb,radius=r,inscribed=True,face=True,support=None)
		# node.obj.Shape = Part.Face(pol).extrude(Base.Vector(0,0,h))
		node.centerx = -r
		node.centery = -r
		node.centerz = h/2.0
		useCenter(node, center)
	node.addAction(createPolyExt, (node, r,nb,h,center))
	return node

def poly_int(a=_default_size, nb=3, h=_default_size,center=None,d=0.0,name=None):
	a = abs(a)
	h = abs(h)
	nb= max(3, abs(nb))
	d = abs(d)
	if( (a == _default_size or a == 0.0) and d != 0.0):
		a = d/2.0
	# create polygon with apothem, not radius
	radius = a / math.cos(math.radians(180)/nb)
	return poly_ext(radius,nb,h,center,name=name)

def polyhedron(points=[], faces=[],center=None,name=None):
	if(not (isinstance(points[0], list) or isinstance(points[0], tuple)) or len(points)<2):
		return sphere(1)
	if(not (isinstance(faces[0], list) or isinstance(faces[0], tuple)) or len(faces)<2):
		return sphere(1)
	global _idx_EasyNode
	node = EasyNodeLeaf()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "polyhedron_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createPolyhedron(node, points,faces):
		try:
			vectors = []
			for point in points:
				vectors.append(Base.Vector(float(point[0]) if len(point)>0 else 0.0, float(point[1]) if len(point)>1 else 0.0, float(point[2]) if len(point)>2 else 0.0))
			polygons = []
			for face in faces:
				face_vect = []
				for idx in face:
					face_vect.append(vectors[idx])
				face_vect.append(vectors[face[0]])
				print("face : ",face_vect)
				polygons.append(Part.Face([Part.makePolygon(face_vect)]))
			node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
			node.obj.Shape = Part.Solid(Part.Shell(polygons))
			node.centerx = 0.0
			node.centery = 0.0
			node.centerz = 0.0
		except:
			node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
			node.obj.Shape = Part.makeSphere(_default_size)
			node.move(_moy_vect(points))
			node.centerx = 0.0
			node.centery = 0.0
			node.centerz = 0.0
	node.addAction(createPolyhedron, (node, points,faces))
	return node

def polyhedron_wip(points=[], faces=[],center=None,name=None):
	if(not (isinstance(points[0], list) or isinstance(points[0], tuple)) or len(points)<2):
		return sphere(1)
	global _idx_EasyNode
	if(name == None or not isinstance(name, str)):
		name = "polyhedron_wip_"+str(_idx_EasyNode)
	node = group(name)
	shapes = []
	if(len(faces)==0):
		idx=0
		for p in points:
			shapes.append(point(p, name=node.name+"_p_"+str(idx)))
			idx+=1
	elif(len(faces)>0):
		idx=0
		for p in points:
			shapes.append(point(p, name=node.name+"_p_"+str(idx)))
			idx+=1
		idx=0
		for face in faces:
			face_vect = []
			for idx in face:
				if(idx>=len(points)):
					face_vect = []
					break;
				face_vect.append(points[idx])
			if(len(face_vect)>2):
				shapes.append(polygon(face_vect,closed=True, name=node.name+"_face_"+str(idx)))
			else:
				shapes.append(sphere(_default_size, name=node.name+"_face_"+str(idx)))
			idx+=1
	node.add(shapes)
	return node
	

def _center_point(points=[]):
	center = [0.0,0.0,0.0]
	for flpoint in points:
		center[0] += flpoint[0]
		center[1] += flpoint[1]
		center[2] += flpoint[2]
	return Base.Vector(center[0]/len(points),center[1]/len(points),center[2]/len(points))

def _calc_angle_3D(v1,v2,norm_approx):
	# dot = x1*x2 + y1*y2      # dot product
	# det = x1*y2 - y1*x2      # determinant
	det = 0
	vdet = v1.cross(v2)
	if(norm_approx.dot(vdet)<0):
		det = -vdet.Length
	else:
		det = vdet.Length
	#det = v1.x*v2.y - v1.y*v2.x
	# print("atan2",det, v1.dot(v2), v1.cross(v2))
	angle = math.atan2(det, v1.dot(v2))  # atan2(y, x) or atan2(sin, cos)
	if(angle<0):
		angle = angle + math.pi*2
	return angle
def _calc_angle_3D_spec(id1, id2, pref, p1, p2, pcenter, vn):
	angle2=0
	if(id2<0):
		angle2 = _calc_angle_3D(pref-pcenter, p2-pcenter, vn) - math.pi*2
	elif(id2>0):
		angle2 = _calc_angle_3D(pref-pcenter, p2-pcenter, vn)
	if(angle2!=0):
		return angle2
	else:
		angle1=0
		if(id1<0):
			angle1 = _calc_angle_3D(pref-pcenter, p1-pcenter, vn) - math.pi*2
		elif(id1>0):
			angle1 = _calc_angle_3D(pref-pcenter, p1-pcenter, vn)
		
		return angle1
def _create_geometry_slices(points = [], centers=[]):
	print(len(points), len(centers))
	#first layer: connect them with center
	rPoints = []
	centersP=[]
	rPoints.append([])
	for flpoint in points[0]:
		rPoints[0].append(Base.Vector(flpoint[0],flpoint[1],flpoint[2]))
		# Draft.makePoint(flpoint[0],flpoint[1],flpoint[2])
	if(len(centers)==0):
		centersP.append(_center_point(points[0]))
		print("new center 0 : ",centersP[0])
	else:
		centersP.append(Base.Vector(centers[0]))
		print("get center 0 : ",centersP[0])
	# Draft.makePoint(centersP[0])
	faces=[]
	if(len(points[0])>3):
		face = Part.Face(Part.makePolygon([centersP[0],rPoints[0][len(rPoints[0])-1], rPoints[0][0] ],True))
		faces.append(face)
		# obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "obj_"+str(_idx_EasyNode))
		# obj.Shape = face
		print(faces)
		for idp in range(1,len(rPoints[0])):
			face = Part.Face(Part.makePolygon([centersP[0],rPoints[0][idp-1], rPoints[0][idp] ],True))
			faces.append(face)
			# obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "obj_"+str(_idx_EasyNode))
			# obj.Shape = face
	elif(len(points[0])==3):
		face = Part.Face(Part.makePolygon([rPoints[0][0],rPoints[0][1], rPoints[0][2] ],True))
		faces.append(face)
		
	nbLayers = len(points)
	for numlayer in range(1,nbLayers):
		print(numlayer)
		rPoints.append([])
		if(len(centers)<=numlayer):
			centersP.append(_center_point(points[numlayer]))
			# print("new center "+str(numlayer)+" : ",centersP[numlayer])
		else:
			centersP.append(Base.Vector(centers[numlayer]))
			# print("get center "+str(numlayer)+" : ",centersP[numlayer], centers[numlayer])
		# Draft.makePoint(centersP[numlayer])
		for flpoint in points[numlayer]:
			rPoints[numlayer].append(Base.Vector(flpoint[0],flpoint[1],flpoint[2]))
		id_bot = len(rPoints[numlayer-1])-1
		point_bot = rPoints[numlayer-1][id_bot]
		id_top = len(rPoints[numlayer])-1
		point_top = rPoints[numlayer][id_top]
		id_bot = -1 if(id_bot>0) else 0
		id_top = -1 if(id_top>0) else 0
		while(id_bot<len(rPoints[numlayer-1])-1 and id_top<len(rPoints[numlayer])-1):
			# print(point_bot, rPoints[numlayer-1][id_bot+1], )
			# print(_calc_angle_3D_spec(id_bot, id_bot+1, rPoints[numlayer-1][0], point_bot, rPoints[numlayer-1][id_bot+1], centersP[numlayer-1], centersP[numlayer]-centersP[numlayer-1])
				# , _calc_angle_3D_spec(id_top, id_top+1, rPoints[numlayer][0], point_top, rPoints[numlayer][id_top+1], centersP[numlayer], centersP[numlayer]-centersP[numlayer-1]))
			anglebot = _calc_angle_3D_spec(id_bot, id_bot+1, rPoints[numlayer-1][0], point_bot, rPoints[numlayer-1][id_bot+1], centersP[numlayer-1], centersP[numlayer]-centersP[numlayer-1])
			angletop = _calc_angle_3D_spec(id_top, id_top+1, rPoints[numlayer][0], point_top, rPoints[numlayer][id_top+1], centersP[numlayer], centersP[numlayer]-centersP[numlayer-1])
			if( anglebot > angletop or (anglebot == angletop and len(rPoints[numlayer])<len(rPoints[numlayer-1])) ):
				# print("face top ",id_bot, id_top, id_bot, id_top+1, len(rPoints[numlayer-1]), len(rPoints[numlayer]), point_bot, point_top, rPoints[numlayer][id_top+1])
				face = (Part.Face(Part.makePolygon([point_bot, point_top, rPoints[numlayer][id_top+1] ],True)))
				faces.append(face)
				# obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "obj_"+str(_idx_EasyNode))
				# obj.Shape = face
				id_top += 1
				point_top = rPoints[numlayer][id_top]
			else:
				# print("face bot ",id_bot, id_top, id_bot+1, id_top, len(rPoints[numlayer-1]), len(rPoints[numlayer]), point_bot, point_top, rPoints[numlayer-1][id_bot+1])
				face = (Part.Face(Part.makePolygon([point_bot, point_top, rPoints[numlayer-1][id_bot+1] ],True)))
				faces.append(face)
				# obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "obj_"+str(_idx_EasyNode))
				# obj.Shape = face
				id_bot += 1
				point_bot = rPoints[numlayer-1][id_bot]
		while(id_bot<len(rPoints[numlayer-1])-1):
			# print("last face bot ",id_bot, id_top, id_bot+1, id_top, len(rPoints[numlayer-1]), len(rPoints[numlayer]), point_bot, point_top, rPoints[numlayer-1][id_bot+1])
			face = (Part.Face(Part.makePolygon([point_bot, point_top, rPoints[numlayer-1][id_bot+1] ],True)))
			faces.append(face)
			# obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "obj_"+str(_idx_EasyNode))
			# obj.Shape = face
			id_bot += 1
			point_bot = rPoints[numlayer-1][id_bot]
		while(id_top<len(rPoints[numlayer])-1):
			# print("last face top ",id_bot, id_top, id_bot, id_top+1, len(rPoints[numlayer-1]), len(rPoints[numlayer]), point_bot, point_top, rPoints[numlayer][id_top+1])
			face = (Part.Face(Part.makePolygon([point_bot, point_top, rPoints[numlayer][id_top+1] ],True)))
			faces.append(face)
			# obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "obj_"+str(_idx_EasyNode))
			# obj.Shape = face
			id_top += 1
			point_top = rPoints[numlayer][id_top]
	#close top
	if(len(points[nbLayers-1])>3):
		face = Part.Face(Part.makePolygon([centersP[nbLayers-1],rPoints[nbLayers-1][len(rPoints[nbLayers-1])-1], rPoints[nbLayers-1][0] ],True))
		faces.append(face)
		# obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "obj_"+str(_idx_EasyNode))
		# obj.Shape = face
		for idp in range(1,len(rPoints[nbLayers-1])):
			face = Part.Face(Part.makePolygon([centersP[nbLayers-1],rPoints[nbLayers-1][idp-1], rPoints[nbLayers-1][idp] ],True))
			faces.append(face)
			# obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "obj_"+str(_idx_EasyNode))
			# obj.Shape = face
	elif(len(points[0])==3):
		face = Part.Face(Part.makePolygon([rPoints[nbLayers-1][0],rPoints[nbLayers-1][1], rPoints[nbLayers-1][2] ],True))
		faces.append(face)
		
	# obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "obj_"+str(_idx_EasyNode))
	# obj.Shape = Part.Solid(Part.Shell(faces))
	# obj.Shape = (Part.Shell(faces))
	return Part.Solid(Part.Shell(faces))
_default_size=1
def solid_slices(points=[],centers=[],center=None,name=None):
	if(len(points)<2):
		return sphere(_default_size)
	global _idx_EasyNode
	node = EasyNodeLeaf()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "solid_slices_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createSolidSlices(node, points, centers, center):
		print("centersA=",len(centers))
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
		try:
			node.obj.Shape = _create_geometry_slices(points, centers)
			pcenter = _center_point(points[0])
			node.centerx = pcenter.x
			node.centery = pcenter.y
			node.centerz = (_center_point(points[len(points)-1]).z-pcenter.z)/2.0
			useCenter(node, center)
		except:
			node.obj.Shape = Part.makeSphere(_default_size)
			node.centerx = 0.0
			node.centery = 0.0
			node.centerz = 0.0
	node.addAction(createSolidSlices, (node, points, list(centers), center))
	return node

thread_pattern_triangle = [[0,0],[1,1],[0,1]]
thread_pattern_iso_internal = [[0,0],[0,0.125],[1,0.4375],[1,0.6875],[0,1]]
# 375 -> -625 -> 312.5
thread_pattern_iso_external = [[0,0],[0,0.250],[1,0.5625],[1,0.6875],[0,1]]
def thread(r=_default_size,p=_default_size,nb=_default_size,r2=_default_size*1.5, pattern=[[0,0],[1,0.5],[0,1]],fn=8,center=None,name=None):
	#check pattern
	if(len(pattern)<1):
		return sphere(1)
	#check if begin in (0,0)
	if(pattern[0][0] !=0 or pattern[0][1] != 0):
		pattern.insert(0,[0,0])
	#check if return to x=0 line
	if(pattern[len(pattern)-1][0] !=0):
		pattern.append([0,1])
	if(nb<1):
		nb = 1
		
	#create node
	global _idx_EasyNode
	node = EasyNodeLeaf()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "thread_"+str(_idx_EasyNode)
	else:
		node.name = name
	
	#def construct
	def createThread(node, r, p, nb, r2, pattern,fn, center):
		maxXpat = 0.0
		for ppat in pattern:
			maxXpat = max(ppat[0], maxXpat)
		scalex = (r2-r)/float(maxXpat)
		scaley = p/pattern[len(pattern)-1][1]
		#make
		faces = []
		pattVect = []
		for pattp in pattern:
			pattVect.append(Base.Vector(r+pattp[0]*scalex,0,pattp[1]*scaley))
		previousLine = []
		
		#init
		zoffsetIncr = p/float(fn)
		zoffset = -p
		#iterations
		rotater = Base.Rotation()
		rotater.Axis = Base.Vector(0,0,1)
		rotater.Angle = (math.pi*2.0/fn)*(-1)
		for pattp in pattVect:
			tempvec = rotater.multVec(pattp)
			tempvec.z += zoffset
			previousLine.append(tempvec)
			
		#closing faces
		rotater.Angle = (math.pi*2.0/fn)*(-2)
		closevec = rotater.multVec(pattp)
		closevec.z += zoffset - zoffsetIncr
		for id in range(1,len(pattVect)):
			face = Part.Face(Part.makePolygon([previousLine[id-1], closevec, previousLine[id] ],True))
			faces.append(face)
			# obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "obj_"+str(_idx_EasyNode))
			# obj.Shape = face
		middlevec = Base.Vector(0,0,-p)
		for step in range(0,fn):
			rotater.Angle = (math.pi*2.0/fn)*(step-1)
			nextclosevec = rotater.multVec(pattp)
			nextclosevec.z += zoffset -p + zoffsetIncr * step 
			face = Part.Face(Part.makePolygon([closevec, middlevec, nextclosevec ],True))
			faces.append(face)
			# obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "obj_"+str(_idx_EasyNode))
			# obj.Shape = face
			closevec = nextclosevec
			
		#iterate
		for turn in range(0,nb):
			for step in range(0,fn):
				zoffset += zoffsetIncr
				rotater.Angle = (math.pi*2.0/fn)*step
				nextLine = []
				for pattp in pattVect:
					tempvec = rotater.multVec(pattp)
					tempvec.z += zoffset
					nextLine.append(tempvec)
				for id in range(1,len(pattVect)):
					# face = Part.Face(Part.makePolygon([previousLine[id-1],nextLine[id-1], nextLine[id], previousLine[id] ],True))
					face = Part.Face(Part.makePolygon([previousLine[id-1],nextLine[id-1], previousLine[id] ],True))
					faces.append(face)
					# obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "obj_"+str(_idx_EasyNode))
					# obj.Shape = face
					face = Part.Face(Part.makePolygon([nextLine[id-1], nextLine[id], previousLine[id] ],True))
					faces.append(face)
					# obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "obj_"+str(_idx_EasyNode))
					# obj.Shape = face
				previousLine = nextLine
		
		#closing faces
		rotater.Angle = (math.pi*2.0/fn)*(0)
		closevec = rotater.multVec(pattp)
		closevec.z += zoffset -p + zoffsetIncr
		for id in range(1,len(pattVect)):
			face = Part.Face(Part.makePolygon([previousLine[id-1], closevec, previousLine[id] ],True))
			faces.append(face)
			# obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "obj_"+str(_idx_EasyNode))
			# obj.Shape = face
		middlevec = Base.Vector(0,0,(nb)*p)
		rotater.Angle = (math.pi*2.0/fn)*(-1)
		nextvec = rotater.multVec(pattp)
		nextvec.z += zoffset + zoffsetIncr
		face = Part.Face(Part.makePolygon([closevec, middlevec, previousLine[id] ],True))
		faces.append(face)
		# obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "obj_"+str(_idx_EasyNode))
		# obj.Shape = face
		for step in range(0,fn-1):
			rotater.Angle = (math.pi*2.0/fn)*(step+1)
			nextclosevec = rotater.multVec(pattp)
			nextclosevec.z += zoffset - p + zoffsetIncr * (step+2)
			face = Part.Face(Part.makePolygon([closevec, middlevec, nextclosevec ],True))
			faces.append(face)
			# obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "obj_"+str(_idx_EasyNode))
			# obj.Shape = face
			closevec = nextclosevec
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
		node.obj.Shape = Part.Solid(Part.Shell(faces))
		print(node.obj)
	
		node.centerx = -r
		node.centery = -r
		node.centerz = p*(nb-1)/2.0
		useCenter(node, center)
		
	node.addAction(createThread, (node, r, p, nb, r2, pattern,fn, center))
	return node

def iso_thread(d=1,p=0.25, h=1,internal = False, offset=0.0,name=None, fn=15):
	my_pattern = [[0,0],[0,0.250],[1,0.5625],[1,0.6875],[0,1]]
	if(internal):
		my_pattern = [[0,0],[0,0.125],[1,0.4375],[1,0.6875],[0,1]]
	# if(offset!=0):
	my_pattern[1][1] = my_pattern[1][1]+offset*2
	my_pattern[2][1] = my_pattern[2][1]+offset*2
	print("internal?", internal, my_pattern)
	rOffset = 0.866*p*5/8.0
	rMax = d/2.0
	return cut(name=name)(
		thread(r=rMax-rOffset, r2=rMax, p=p, nb=int(h/p +2),fn=fn, pattern = my_pattern).move(z= -offset/2.0),
		cube(3*d,3*d,3*p).xy().move(z= -3*p),
		cube(3*d,3*d,4*p).xy().move(z=h),
	)
	
######### 2D & 1D objects ######### 

class EasyNode2D(EasyNodeLeaf):
	def __init__(self):
		# super().__init__()
		centerz = 0.0
		self.childs = []
		self.actions = []
		self.actions_after = []
		self.simple = True
	def rotate2D(self, x=0.0,y=0.0):
		(x,y) = _getTuple2(x,y,0.0)
		myMat = Base.Matrix()
		myMat.rotateX(y*math.pi/180)
		myMat.rotateY(x*math.pi/180)
		myMat.rotateZ(0.0)
		self.obj.Placement=FreeCAD.Placement(myMat).multiply(self.obj.Placement)
		return self

def circle(r=_default_size,fn=1,closed=True,center=None,d=0.0,name=None):
	r=abs(r)
	d = abs(d)
	if(fn>2):
		return poly_reg(r,fn,center)
	if( r == 0.0 and d != 0.0):
		r = d/2.0
	global _idx_EasyNode
	node = EasyNode2D()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "circle_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createCircle(node, r,center):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
		temp_poly = Part.makeCircle(r)
		if(closed):
			node.obj.Shape = Part.Face(Part.Wire(temp_poly))
		else:
			node.obj.Shape = Part.Wire(temp_poly)
		node.centerx = -r
		node.centery = -r
		useCenter(node, center)
	node.addAction(createCircle, (node, r,center))
	return node

def ellipse(r1=0.0,r2=0.0,center=None,d1=0.0,d2=0.0,name=None):
	r1 = abs(r1)
	r2 = abs(r2)
	d1 = abs(d1)
	d2 = abs(d2)
	if( r1 == 0.0 and d1 != 0.0):
		r1 = d1/2.0
	if( r2 == 0.0 and d2 != 0.0):
		r2 = d2/2.0
	global _idx_EasyNode
	node = EasyNode2D()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "ellipse_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createEllipse(node, r1,r2,center):
		# node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "ellipse_"+str(_idx_EasyNode))
		node.obj = Draft.makeEllipse(r2,r1)
		node.obj.Label = node.name
		# node.obj.Shape = temp_poly
		node.centerx = -r2
		node.centery = -r1
		useCenter(node, center)
	node.addAction(createEllipse, (node, r1,r2,center))
	return node

def poly_reg(r=_default_size,nb=3,center=None,inscr=True,d=0.0,name=None):
	r=abs(r)
	d = abs(d)
	nb=max(3,abs(nb))
	if( r == 0.0 and d != 0.0):
		r = d/2.0
	global _idx_EasyNode
	node = EasyNode2D()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "reg_ploygon_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createPolygonReg(node, r,nb,center,inscr):
		node.obj = Draft.makePolygon(nb,radius=r,inscribed=inscr,face=True,support=None)
		node.obj.Label = node.name
		# temp_poly = Part.makeCircle(r)
		# node.obj.Shape = temp_poly
		node.centerx = -r
		node.centery = -r
		useCenter(node, center)
	node.addAction(createPolygonReg, (node, r,nb,center,inscr))
	return node

def square(size=_default_size,y=0.0,x=0.0,center=None,name=None):
	(size,y,x) = _getTuple3Abs(size,y,x,_default_size)
	if(y>0.0):
		if(x>0.0):
			return rectangle(x,y,center=center)
		else:
			return rectangle(size,y,center=center)
	global _idx_EasyNode
	node = EasyNode2D()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "square_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createSquare(node, size,center):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
		node.obj.Shape = Part.makePlane(size, size)
		node.centerx = size/2.0
		node.centery = size/2.0
		useCenter(node, center)
	node.addAction(createSquare, (node, size,center))
	return node

def rectangle(x=_default_size,y=_default_size,center=None,name=None):
	(x,y) = _getTuple2Abs(x,y,_default_size)
	global _idx_EasyNode
	node = EasyNode2D()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "rectangle_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createRectangle(node, x,y,center):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
		node.obj.Shape = Part.makePlane(x, y)
		node.centerx = x/2.0
		node.centery = y/2.0
		useCenter(node, center)
	node.addAction(createRectangle, (node, x,y,center))
	return node

def polygon(points=[], closed=True,name=None):
	if(not (isinstance(points[0], list) or isinstance(points[0], tuple)) or len(points)<2):
		return circle(_default_size)
	if(len(points)==1 and (isinstance(points[0], list) or isinstance(points[0], tuple))):
		points = points[0]
	if len(points) < 3:
		return circle(_default_size)
	global _idx_EasyNode
	node = EasyNode2D()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "polygon_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createPolygon(node, points,center):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
		vectors = []
		for point in points:
			vectors.append(Base.Vector(float(point[0]) if len(point)>0 else 0.0, float(point[1]) if len(point)>1 else 0.0, float(point[2]) if len(point)>2 else 0.0))
		if(closed):
			vectors.append(Base.Vector(float(points[0][0]) if len(points[0])>0 else 0.0, float(points[0][1]) if len(points[0])>1 else 0.0, float(points[0][2]) if len(points[0])>2 else 0.0))
		try:
			temp_poly = Part.makePolygon(vectors) 
			if(closed):
				node.obj.Shape = Part.Face(temp_poly)
			else:
				node.obj.Shape = temp_poly
		except:
			node.obj.Shape = Part.makeSphere(_default_size)
			node.move(_moy_vect(points))
		node.centerx = 0.0
		node.centery = 0.0
	node.addAction(createPolygon, (node, points,center))
	return node

def bspline(points=[], closed=False,name=None):
	if(not (isinstance(points[0], list) or isinstance(points[0], tuple)) or len(points)<2):
		return circle(_default_size)
	if(len(points)==1 and (isinstance(points[0], list) or isinstance(points[0], tuple))):
		points = points[0]
	vectors = []
	for point in points:
		if isinstance(point, list) or isinstance(point, tuple) :
			vectors.append(FreeCAD.Vector(point[0] if len(point)>0 else 0.0, point[1] if len(point)>1 else 0.0, point[2] if len(point)>2 else 0.0))
	global _idx_EasyNode
	node = EasyNode2D()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "bspline_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createBspline(node, vectors,center,closed):
		node.obj = Draft.makeBSpline(vectors,closed=closed,face=closed,support=None)
		node.obj.Label = node.name
		node.centerx = 0.0
		node.centery = 0.0
	node.addAction(createBspline, (node, vectors,center,closed))
	return node

def bezier(points=[], closed=False,name=None):
	if(not (isinstance(points[0], list) or isinstance(points[0], tuple)) or len(points)<3):
		return circle(_default_size)
	if(len(points)==1 and (isinstance(points[0], list) or isinstance(points[0], tuple))):
		points = points[0]
	vectors = []
	for point in points:
		if isinstance(point, list) or isinstance(point, tuple) :
			vectors.append(FreeCAD.Vector(point[0] if len(point)>0 else 0.0, point[1] if len(point)>1 else 0.0, point[2] if len(point)>2 else 0.0))
	global _idx_EasyNode
	node = EasyNode2D()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "bezier_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createBezier(node, vectors,center,closed):
		node.obj = Draft.makeBezCurve(vectors,closed,support=None)
		node.obj.Label = node.name
		node.centerx = 0.0
		node.centery = 0.0
	node.addAction(createBezier, (node, vectors,center,closed))
	return node

def helix(r=_default_size,p=_default_size,h=_default_size,center=None,d=0.0,name=None):
	r = abs(r)
	p = abs(p)
	h = abs(h)
	d = abs(d)
	if( r == 0.0 and d != 0.0):
		r = d/2.0
	global _idx_EasyNode
	node = EasyNode2D()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "helix_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createHelix(node, r,p,h,center):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
		node.obj.Shape = Part.makeHelix (p, h, r)
		node.centerx = -float(r)
		node.centery = -float(r)
		useCenter(node, center)
	node.addAction(createHelix, (node, r,p,h,center))
	return node
	
def point(p=[0.0,0.0,0.0],center=None,name=None):
	if(not isinstance(p,list) and not isinstance(p,tuple)):
		p = [0.0,0.0,0.0]
	p = ( float(p[0]) if len(p)>0 else 0.0, float(p[1]) if len(p)>1 else 0.0, float(p[2]) if len(p)>2 else 0.0 )
	global _idx_EasyNode
	node = EasyNode()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "line_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createPoint(node, p,center):
		node.obj = Draft.makePoint(Base.Vector(p))
		node.centerx = 0
		node.centery = 0
		node.centerz = 0
		useCenter(node, center)
	node.addAction(createPoint, (node, p,center))
	return node
	
def line(p1=[0.0,0.0,0.0],p2=[_default_size,_default_size,_default_size],center=None,name=None):
	if(not isinstance(p1,list) and not isinstance(p1,tuple)):
		p1 = [0.0,0.0,0.0]
	if(not isinstance(p2,list) and not isinstance(p2,tuple)):
		p2 = [_default_size,_default_size,_default_size]
	p1 = ( float(p1[0]) if len(p1)>0 else 0.0, float(p1[1]) if len(p1)>1 else 0.0, float(p1[2]) if len(p1)>2 else 0.0 )
	p2 = ( float(p2[0]) if len(p2)>0 else 0.0, float(p2[1]) if len(p2)>1 else 0.0, float(p2[2]) if len(p2)>2 else 0.0 )
	global _idx_EasyNode
	node = EasyNode()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "line_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createLine(node, p1,p2,center):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
		node.obj.Shape = Part.makeLine(p1,p2)
		node.centerx = (p1[0]+p2[0])/2.0
		node.centery = (p1[1]+p2[1])/2.0
		node.centerz = (p1[2]+p2[2])/2.0
		useCenter(node, center)
	node.addAction(createLine, (node, p1,p2,center))
	return node
	
def arc(p1=[_default_size,0.0,0.0],p2=[_default_size*0.7071,_default_size*0.7071,0.0],p3=[0.0,_default_size,0.0],center=None,name=None):
	if(not isinstance(p1,list) and not isinstance(p1,tuple)):
		p1 = [_default_size,0.0,0.0]
	if(not isinstance(p2,list) and not isinstance(p2,tuple)):
		p2 = [_default_size*0.7071,_default_size*0.7071,0.0]
	if(not isinstance(p3,list) and not isinstance(p3,tuple)):
		p3 = [0.0,_default_size,0.0]
	p1 = ( float(p1[0]) if len(p1)>0 else 0.0, float(p1[1]) if len(p1)>1 else 0.0, float(p1[2]) if len(p1)>2 else 0.0 )
	p2 = ( float(p2[0]) if len(p2)>0 else 0.0, float(p2[1]) if len(p2)>1 else 0.0, float(p2[2]) if len(p2)>2 else 0.0 )
	p3 = ( float(p3[0]) if len(p3)>0 else 0.0, float(p3[1]) if len(p3)>1 else 0.0, float(p3[2]) if len(p3)>2 else 0.0 )
	global _idx_EasyNode
	node = EasyNode()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "arc_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createArc(node, p1,p2,p3,center):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
		node.obj.Shape = Part.Shape([Part.Arc(Base.Vector(p1),Base.Vector(p2),Base.Vector(p3))])
		node.centerx = (p1[0]+p3[0])/2.0
		node.centery = (p1[1]+p3[1])/2.0
		node.centerz = (p1[2]+p3[2])/2.0
		useCenter(node, center)
	node.addAction(createArc, (node, p1,p2,p3,center))
	return node
	
def text(text="Hello", size=_default_size, font="arial.ttf",center=None,name=None):
	global _idx_EasyNode
	node = EasyNode2D()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "text_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createText(node, text,size,font,center):
		node.obj = Draft.makeShapeString(String=text,FontFile=font,Size=size,Tracking=0)
		node.obj.Label = node.name
		# temp_poly = Draft.makeShapeString(String=text,FontFile=font,Size=size,Tracking=0)
		# node.obj.Shape = temp_poly
		node.centerx = 0.0
		node.centery = 0.0
		useCenter(node, center)
	node.addAction(createText, (node, text,size,font,center))
	return node

def gear(nb=6, mod=2.5, angle=20.0, external=True, high_precision=False,name=None):
	global _idx_EasyNode
	node = EasyNode2D()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "gear_"+str(_idx_EasyNode)
	else:
		node.name = name
	nb = int(nb)
	def createGear(node, nb,mod,angle,center,high_precision,external):
		gear = InvoluteGearFeature.makeInvoluteGear(node.name)
		gear.NumberOfTeeth = nb
		gear.Modules = str(mod)+' mm'
		gear.HighPrecision = high_precision
		gear.ExternalGear = external
		gear.PressureAngle = angle
		node.obj = gear
		node.centerx = 0.0 #TODO
		node.centery = 0.0
	node.addAction(createGear, (node, nb,mod,angle,center,high_precision,external))
	return node;

######### Extrusion (2D to 3D) #########

class EasyNodeLinear(EasyNodeColored):
	def __init__(self):
		self.edges = [];
		self.childs = []
		self.actions = []
		self.actions_after = []
		self.simple = True
	def layout_childs(self, *tupleOfNodes):
		if(len(tupleOfNodes)>=1):
			self.obj.Base = tupleOfNodes[0].obj
			if self.obj.Base.ViewObject is not None:
				self.obj.Base.ViewObject.Visibility = False
		return self

def linear_extrude(height=_default_size,center=None,name=None, twist=0, taper=0.0, slices=0,convexity=0):
	global _idx_EasyNode
	node = EasyNodeLinear()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "lin_extrude_"+str(_idx_EasyNode)
	else:
		node.name = name
	if(twist!=0):
		#if twist, we have to create a sweep against an helix but we don't know the parameters of the helix yet
		#so we add an action to the sweep node to create the helix and layout the thing
		pitch = height * 360.0/abs(twist)
		print("pitch="+str(pitch)+", h="+str(height))
		extruder = path_extrude(frenet=True,name=node.name)
		def create_helix(node, height, pitch, twist, nameNode):
			if(len(node.childs)>0):
				pos = node.childs[0].obj.Placement.Base
				zpos = pos.z
				pos.z = 0.0
				radius = pos.Length
				anglerad = math.atan2(pos.x,pos.y)
				helix_obj = helix(r=radius,p=pitch,h=height, name=nameNode+"_twist").rotate(0,0,anglerad*180.0/math.pi)
				if(twist>0):
					helix_obj = mirror(0,1,0, name=nameNode+"_twist_m")(helix_obj)
				node.childs.insert(0, helix_obj)
				helix_obj.create()
				node.layout_childs(*tuple(node.childs))
		extruder.actions_after.append((create_helix, (extruder, height, pitch, twist, node.name)))
		return extruder
	def createZExtrude(node, height, taper, center):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Extrusion", node.name)
		node.obj.DirMode = "Normal"
		node.obj.TaperAngle = taper*180.0/math.pi
		node.obj.LengthFwd = height
		node.obj.Solid = True
		node.centerx=0.0
		node.centery=0.0
		node.centerz=height/2.0
		useCenter(node, center)
	node.addAction(createZExtrude, (node, height, taper, center))
	return node

def extrude(x=0.0,y=0.0,z=0.0, taper=0.0,name=None,convexity=0):
	(x,y,z) = _getTuple3Abs(x,y,z,0.0)
	normal = Base.Vector(x,y,z);
	length = normal.Length
	global _idx_EasyNode
	node = EasyNodeLinear()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "extrude_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createExtrude(node, x,y,z,taper):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Extrusion", node.name)
		node.obj.DirMode = "Custom"
		node.obj.LengthFwd = length
		node.obj.TaperAngle = taper
		node.obj.Dir = normal
		node.obj.Solid = True
	node.addAction(createExtrude, (node, x,y,z,taper))
	return node
	
class EasyNodeRotateExtrude(EasyNodeColored):
	def layout_childs(self, *tupleOfNodes):
		if(len(tupleOfNodes)>=1):
			self.obj.Source = tupleOfNodes[0].obj
			if self.obj.Source.ViewObject is not None:
				self.obj.Source.ViewObject.Visibility = False
		return self

def rotate_extrude(angle=360.0,name=None,convexity=2):
	angle = min(abs(angle),360.0)
	global _idx_EasyNode
	node = EasyNodeRotateExtrude()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "revolution_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createRExtrude(node, angle):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Revolution", node.name)
		node.obj.Axis = Base.Vector(0.0,_default_size,0.0)
		node.obj.Base = Base.Vector(0.0,0.0,0.0)
		node.obj.Angle = float(angle)
		node.obj.Solid = True
		node.obj.AxisLink = None
		node.obj.Symmetric = False
		node.rotate(90,0,0)
	node.addAction(createRExtrude, (node, angle))
	return node

class EasyNodeSweep(EasyNode):
	base = None
	def layout_childs(self, *tupleOfNodes):
		if len(tupleOfNodes) == 1 and self.base == None:
			self.base = tupleOfNodes[0]
			return self
		base = None
		tool = None
		if len(tupleOfNodes) == 2 :
			base = tupleOfNodes[0]
			tool = tupleOfNodes[1]
		elif len(tupleOfNodes) > 2 :
			base = tupleOfNodes[0]
			tool = tupleOfNodes[1]
		elif len(tupleOfNodes) == 1:
			base = self.base
			tool = tupleOfNodes[0]
		else:
			# print("Error, wrong number of sweep elements : "+str(len(self.childs)) +" and we need 2")
			return self
		self.obj.Sections = [tool.obj]
		arrayEdge = []
		i=1
		for edge in base.obj.Shape.Edges:
			arrayEdge.append("Edge"+str(i))
			i += 1
		self.obj.Spine = (base.obj,arrayEdge)
		if base.obj.ViewObject is not None:
			base.obj.ViewObject.Visibility = False
		if tool.obj.ViewObject is not None:
			tool.obj.ViewObject.Visibility = False
		return self

#transition in ["Right corner", "Round corner","Transformed" ]
def path_extrude(frenet=True, transition = "Right corner",name=None):
	global _idx_EasyNode
	node = EasyNodeSweep()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "sweep_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createPExtrude(node, frenet,transition):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Sweep", node.name)
		node.obj.Solid = True
		node.obj.Frenet = frenet
		node.obj.Transition = transition
	node.addAction(createPExtrude, (node, frenet,transition))
	return node

class EasyNodeAssembleWire(EasyNode):
	def layout_childs(self, *tupleOfNodes):
		edges = []
		arrayObjChilds = []
		for node in tupleOfNodes:
			arrayObjChilds.append(node.obj)
			if node.obj.ViewObject is not None:
				node.obj.ViewObject.Visibility = False
			for edge in node.obj.Shape.Edges:
				edges.append(Part.Edge(edge))
		#put childs into a group
		obj_group = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup", self.name+"_components")
		obj_group.Group = arrayObjChilds
		if obj_group.ViewObject is not None:
			obj_group.ViewObject.Visibility = False
		#create wire
		if(self.closed):
			self.obj.Shape = Part.Face(Part.Wire(edges))
		else:
			self.obj.Shape = Part.Wire(edges)
		return self

#transition in ["Right corner", "Round corner","Transformed" ]
def create_wire(closed=False, name=None):
	global _idx_EasyNode
	node = EasyNodeAssembleWire()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "assemble_"+str(_idx_EasyNode)
	else:
		node.name = name
	def createWire(node):
		node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
	node.addAction(createWire, (node,))
	node.closed = closed
	return node

######### convenience objects for chaining ########
	
center = EasyNode.center
center_x = EasyNode.x
center_y = EasyNode.y
center_z = EasyNode.z
center_xy = EasyNode.xy
center_xz = EasyNode.xz
center_yz = EasyNode.yz
center_xyz = EasyNode.xyz

class ApplyNodeFunc():
	def __init__(self, func, args):
		self.args = args
		self.func = func
		self.before = []
	def __call__(self, *nodes):
		print(nodes[0])
		goodnodes = []
		if len(self.before)>0:
			goodnodes.append(self.before.pop()(*nodes))
			while len(self.before)>0:
				goodnodes[0] = self.before.pop()(goodnodes[0])
		else:
			for node in nodes:
				if(isinstance(node, EasyNode)):
					goodnodes.append(node)
		if(len(goodnodes)==0):
			print("Error, use a function on no node ")
			return self
		elif(len(goodnodes)==1):
			print("do(1) "+self.func.__name__)
			if(len(self.args)==2):
				self.func(goodnodes[0],self.args[0],self.args[1])
			elif(len(self.args)==3):
				self.func(goodnodes[0],self.args[0],self.args[1],self.args[2])
			elif(len(self.args)==4):
				self.func(goodnodes[0],self.args[0],self.args[1],self.args[2],self.args[3])
			return goodnodes[0]
		else:
			print("do(union) "+self.func.__name__)
			union_obj = union(goodnodes)
			if(len(self.args)==2):
				self.func(union_obj,self.args[0],self.args[1])
			elif(len(self.args)==3):
				self.func(union_obj,self.args[0],self.args[1],self.args[2])
			elif(len(self.args)==4):
				self.func(union_obj,self.args[0],self.args[1],self.args[2],self.args[3])
			return union_obj
	#also redefine every function to do the same,
	# it avoid the user to type '(' and ')' and replace it with '.'
	def translate(self, x=0.0,y=0.0,z=0.0):
		self.before.append(ApplyNodeFunc(EasyNode.move, (x,y,z)))
		return self
	def move(self, x=0.0,y=0.0,z=0.0):
		self.before.append(ApplyNodeFunc(EasyNode.move, (x,y,z)))
		return self
	def rotate(self, x=0.0,y=0.0,z=0.0):
		self.before.append(ApplyNodeFunc(EasyNode.rotate, (x,y,z)))
		return self
	def rotate2D(self, x=0.0,y=0.0):
		self.before.append(ApplyNodeFunc(EasyNode2D.rotate2D, (x,y)))
		return self
	def scale(self, x=0.0,y=0.0,z=0.0):
		self.before.append(ApplyNodeFunc(EasyNode.scale, (x,y,z)))
		return self
	def color(self, r=0.0,v=0.0,b=0,a=0.0):
		self.before.append(ApplyNodeFunc(magic_color, (r,v,b,a)))
		return self
	def union(self,name=None):
		return self(union(name))
	def inter(self,name=None):
		return self(inter(name))
	def intersection(self,name=None):
		return self(inter(name))
	def cut(self,name=None):
		return self(cut(name))
	def difference(self,name=None):
		return self(cut(name))
	def chamfer(self,name=None):
		return self(chamfer(name))
	def fillet(self,name=None):
		return self(fillet(name))
	def mirror(self,x=0.0,y=0.0,z=0.0,name=None):
		return self(mirror(x,y,z,name))
	def offset(self,length=0.0,fillet=True,fusion=True,name=None):
		return self(offset(length=length,fillet=fillet,fusion=fusion,name=name))
	def offset2D(self,length=0.0,fillet=True,fusion=True,name=None):
		return self(offset2D(length=length,fillet=fillet,fusion=fusion,name=name))
	def group(self,name=None):
		return self(group(name))
	def cube(self,size=0.0,y=0.0,z=0.0,center=None,x=0.0, name=None):
		return self(cube(size,y,z,center,x, name))
	def box(self,x=_default_size,y=_default_size,z=_default_size,center=None, name=None):
		return self(box(x,y,z,center, name))
	def tri_rect(self,x=_default_size,y=_default_size,z=_default_size,center=None, name=None):
		return self(tri_rect(x,y,z,center, name))
	def cylinder(self,r=0.0,h=_default_size,center=None,d=0.0,r1=0.0,r2=0.0,d1=0.0,d2=0.0,angle=360.0,fn=1,name=None):
		return self(cylinder(r,h,center,d,r1,r2,d1,d2,angle,fn,name))
	def cone(self,r1=_default_size,r2=_default_size,h=_default_size,center=None,d1=0.0,d2=0.0,fn=1,name=None):
		return self(cone(r1,r2,h,center,d1,d2,fn,name))
	def sphere(self,r=_default_size,center=None,d=0.0,fn=1,name=None):
		return self(sphere(r,center,d,fn,name))
	def torus(self,r1=_default_size, r2=0.1,center=None,d1=0.0,d2=0.0,name=None):
		return self(torus(r1, r2,center,d1,d2,name))
	def poly_ext(self,r=_default_size, nb=3, h=_default_size,center=None,d=0.0,name=None):
		return self(poly_ext(r, nb, h,center,d,name))
	def polyhedron(self,points=[],faces=[],center=None,name=None):
		return self(polyhedron(points, faces,center,name))
	def polyhedron_wip(self,points=[],faces=[],center=None,name=None):
		return self(polyhedron_wip(points, faces,center,name))
	def solid_slices(self,points=[],centers=[],center=None,name=None):
		return self(solid_slices(points, centers,center,name))
	def createPolyExt(self,node, r,nb,h,center):
		return self(createPolyExt(node, r,nb,h,center))
	def poly_int(self,a=_default_size, nb=3, h=_default_size,center=None,d=0.0,name=None):
		return self(poly_int(a, nb, h,center,d,name))
	def circle(self,r=_default_size,center=None,d=0.0,fn=1,name=None):
		return self(circle(r,center,d,fn,name))
	def ellipse(self,r1=0.0,r2=0.0,center=None,d1=0.0,d2=0.0,name=None):
		return self(ellipse(r1,r2,center,d1,d2,name))
	def poly_reg(self,r=_default_size,nb=3,center=None,inscr=True,d=0.0,name=None):
		return self(poly_reg(r,nb,center,inscr,d,name))
	def square(self,size=_default_size,y=0.0,x=0.0,center=None,name=None):
		return self(square(size,y,x,center,name))
	def rectangle(self,x=_default_size,y=_default_size,center=None,name=None):
		return self(rectangle(x,y,center,name))
	def polygon(self,points=[], closed=True,name=None):
		return self(polygon(points, closed,name))
	def bspline(self,points=[], closed=False,name=None):
		return self(bspline(points, closed,name))
	def bezier(self,points=[], closed=False,name=None):
		return self(bezier(points, closed,name))
	def helix(self,r=_default_size,p=_default_size,h=_default_size,center=None,d=0.0,name=None):
		return self(helix(r,p,h,center,d,name))
	def gear(self,nb=6, mod=2.5, angle=20.0, external=True, high_precision=False,name=None):
		return self(gear(nb, mod, angle, external, high_precision,name))
	def line(self,p1=[0.0,0.0,0.0],p2=[_default_size,_default_size,_default_size],center=None,name=None):
		return self(line(p1,p2,center,name))
	def text(self,text="Hello", size=_default_size, font="arial.ttf",center=None,name=None):
		return self(text(text, size, font,center,name))
	def linear_extrude(self,length=_default_size, angle=0.0):
		return self(linear_extrude(length, angle))
	def extrude(self,x=0.0,y=0.0,z=0.0, angle=0.0,name=None):
		return self(linear_extrude(x,y,z, angle,name))
	def rotate_extrude(self,angle=360.0,name=None):
		return self(rotate_extrude(angle,name))
	def path_extrude(self,frenet=True, transition = "Right corner",name=None):
		return self(path_extrude(frenet,transition,name))
	def create_wire(self,name=None):
		return self(create_wire(name))
	def importSvg(self,filename="./None.svg", ids=[],name=None):
		return self(importSvg(filename,ids,name))
	def importStl(self,filename="./None.stl", ids=[],name=None):
		return self(importStl(filename,ids,name))
	
def translate(x=0.0,y=0.0,z=0.0):
	return ApplyNodeFunc(EasyNode.move, (x,y,z))

def move(x=0.0,y=0.0,z=0.0):
	return ApplyNodeFunc(EasyNode.move, (x,y,z))

def rotate(x=0.0,y=0.0,z=0.0):
	return ApplyNodeFunc(EasyNode.rotate, (x,y,z))

def scale(x=0.0,y=0.0,z=0.0):
	return ApplyNodeFunc(EasyNode.scale, (x,y,z))
	
def color(r=0.0,v=0.0,b=0,a=0.0):
	return ApplyNodeFunc(magic_color, (r,v,b,a))

######### import & export #########

def importSvg(filename="./None.svg",ids=[],name=None):
	global _idx_EasyNode
	node = EasyNode()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "svg_"+str(_idx_EasyNode)
	else:
		node.name = name
	node.simple= False
	def importSvg_action(filename,ids=[]):
		objects_before = FreeCAD.ActiveDocument.Objects
		importSVG.insert(filename,FreeCAD.ActiveDocument.Name)
		objects_inserted = FreeCAD.ActiveDocument.Objects
		for obj in objects_before:
			objects_inserted.remove(obj)
		if(len(ids)==0):
			if(len(objects_inserted)==1):
				node.obj = objects_inserted[0]
			else:
				node.obj = FreeCAD.ActiveDocument.addObject("Part::MultiFuse", "union_"+str(_idx_EasyNode))
				for obj in objects_inserted:
					node(_easyNodeStub(obj,"svg"))
		else:
			to_unionise = []
			for idx in ids:
				if(idx<len(objects_inserted)):
					to_unionise.append(objects_inserted[idx])
			if(len(to_unionise)==1):
				node.obj = to_unionise[0]
			else:
				node.obj = FreeCAD.ActiveDocument.addObject("Part::MultiFuse", "union_"+str(_idx_EasyNode))
				for obj in to_unionise:
					node(_easyNodeStub(obj,"svg"))
				
	node.addAction(importSvg_action, (filename,ids))
	return node	

def importStl(filename=";/None.stl",ids=[],name=None):
	global _idx_EasyNode
	node = EasyNode()
	_idx_EasyNode += 1
	if(name == None or not isinstance(name, str)):
		node.name = "stl_"+str(_idx_EasyNode)
	else:
		node.name = name
	node.simple= False
	def importStl_action(filename,ids=[]):
		objects_before = FreeCAD.ActiveDocument.Objects
		Mesh.insert(filename,FreeCAD.ActiveDocument.Name)
		objects_inserted = FreeCAD.ActiveDocument.Objects
		for obj in objects_before:
			objects_inserted.remove(obj)
		if(len(ids)==0):
			if(len(objects_inserted)==1):
				shape = Part.Shape()
				shape.makeShapeFromMesh(objects_inserted[0].Mesh.Topology,0.05)
				node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
				node.obj.Shape = Part.makeSolid(shape)
			else:
				node.obj = FreeCAD.ActiveDocument.addObject("Part::MultiFuse", "union_"+str(_idx_EasyNode))
				for mesh in objects_inserted:
					shape = Part.Shape()
					shape.makeShapeFromMesh(mesh.Mesh.Topology,0.05)
					obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
					obj.Shape = Part.makeSolid(shape)
					node(_easyNodeStub(obj),"stl")
		else:
			to_unionise = []
			for idx in ids:
				if(idx<len(objects_inserted)):
					to_unionise.append(objects_inserted[idx])
			if(len(to_unionise)==1):
				shape = Part.Shape()
				shape.makeShapeFromMesh(to_unionise[0].Mesh.Topology,0.05)
				node.obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
				node.obj.Shape = Part.makeSolid(shape)
			else:
				node.obj = FreeCAD.ActiveDocument.addObject("Part::MultiFuse", "union_"+str(_idx_EasyNode))
				for mesh in to_unionise:
					shape = Part.Shape()
					shape.makeShapeFromMesh(mesh.Mesh.Topology,0.05)
					obj = FreeCAD.ActiveDocument.addObject("Part::Feature", node.name)
					obj.Shape = Part.makeSolid(shape)
					node(_easyNodeStub(obj,"stl"))
	node.addAction(importStl_action, (filename,ids))
	return node	
