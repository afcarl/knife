
require 'node'
require 'segment'
require 'triangle'
require 'polyhedron'
require 'cut'

refine_path = File.expand_path("~/GRIDEX/refine/src")

$:.push refine_path

# for Grid...
require 'Adj/Adj'
require 'Line/Line'
require 'Sort/Sort'
require 'Grid/Grid'
require 'GridMath/GridMath'
require 'Near/Near'

class CutSurface

 attr_reader :triangles
 attr_reader :grid
 attr_reader :near_tree

 def CutSurface.from_FAST(filename,cut_bcs)

  # read in the grid
  surface_grid = Grid.from_FAST File.expand_path(filename)
 
  # create a global to local index for surface nodes
  triangles = 0
  surface_g2l = Array.new(surface_grid.nnode)
  surface_grid.nface.times do |face_index|
   face = surface_grid.face(face_index)
   if cut_bcs.include? face[3]
    surface_g2l[face[0]] = true
    surface_g2l[face[1]] = true
    surface_g2l[face[2]] = true
    triangles += 1
   end
  end

  puts "cut surface has #{triangles} triangles"

  node = 0
  surface_grid.nnode.times do |node_index|
   if surface_g2l[node_index] 
    surface_g2l[node_index] = node
    node += 1
   end
  end

  puts "cut surface has #{node} nodes"

  # create surface node objects
  surface_node = Array.new(node)
  node = 0
  surface_grid.nnode.times do |node_index|
   if surface_g2l[node_index]
    xyz = surface_grid.nodeXYZ(node_index)
    surface_node[node] = Node.new(xyz[0],xyz[1],xyz[2])
    node += 1
   end
  end
 
  # count unique segments
  segment = Array.new(node)
  node.times { |i| segment[i] = Array.new(node) }

  segments = 0
  surface_grid.nface.times do |face_index|
   face = surface_grid.face(face_index)
   if cut_bcs.include? face[3]
    face[0] = surface_g2l[face[0]]
    face[1] = surface_g2l[face[1]]
    face[2] = surface_g2l[face[2]]

    node0 = [face[0],face[1]].min
    node1 = [face[0],face[1]].max
    unless segment[node0][node1]
     segment[node0][node1] = Segment.new(surface_node[node0],
                                         surface_node[node1])
     segments += 1
    end

    node0 = [face[1],face[2]].min
    node1 = [face[1],face[2]].max
    unless segment[node0][node1]
     segment[node0][node1] = Segment.new(surface_node[node0],
                                         surface_node[node1])
     segments += 1
    end
   
    node0 = [face[2],face[0]].min
    node1 = [face[2],face[0]].max
    unless segment[node0][node1]
     segment[node0][node1] = Segment.new(surface_node[node0],
                                         surface_node[node1])
     segments += 1
    end

   end
  end

  puts "cut surface has #{segments} segments"

  cut_triangles = Array.new(triangles)

  triangles = 0
  surface_grid.nface.times do |face_index|
   face = surface_grid.face(face_index)
   if cut_bcs.include? face[3]
    face[0] = surface_g2l[face[0]]
    face[1] = surface_g2l[face[1]]
    face[2] = surface_g2l[face[2]]

    node0 = [face[0],face[1]].min
    node1 = [face[0],face[1]].max
    segment2 = segment[node0][node1]
    
    node0 = [face[1],face[2]].min
    node1 = [face[1],face[2]].max
    segment0 = segment[node0][node1]
    
    node0 = [face[2],face[0]].min
    node1 = [face[2],face[0]].max
    segment1 = segment[node0][node1]

    cut_triangles[triangles] = Triangle.new(segment0,segment1,segment2)
    triangles += 1
   end

   CutSurface.new(triangles,grid)
  end

  #build a near tree to speed up searches
  def build_tree
   near_list = Array.new(@triangles.size)
   @triangles.each_with_index do |triangle, index|
    center = triangle.center
    diameter = triangle.diameter
    near_list[index] = Near.new(index,center[0],center[1],center[2],diameter)
   end
   near_list.each_index do |index|
    cut_tree.insert(near_list[index]) if index > 0
   end
   near_list.first
  end

  def initialize(triangles,grid=nil)
   @triangles = triangles
   @grid = grid

   @near_tree = build_tree
  end
 end
