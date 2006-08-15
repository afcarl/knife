#!/usr/bin/env ruby
# unit test for subtri

require 'test/unit'
require 'subtri'
require 'subnode'

class TestSubtri < Test::Unit::TestCase
 
 def test_initialize
  subtri = Subtri.new("n0", "n1", "n2")

  assert subtri.is_a?(Subtri), "not a subtri"

  assert_equal "n0", subtri.n0
  assert_equal "n1", subtri.n1
  assert_equal "n2", subtri.n2
 end

 def test_barycentric
  subtri = Subtri.new(Subnode.new(1.0,0.0,0.0),
                      Subnode.new(0.0,1.0,0.0),
                      Subnode.new(0.0,0.0,1.0))

  tol = 1.0e-14

  u=0.2;v=0.3;w=0.5
  subnode = Subnode.new(u,v,w)
  bary = subtri.barycentric(subnode)
  assert_in_delta u, bary[0], tol
  assert_in_delta v, bary[1], tol
  assert_in_delta w, bary[2], tol
  u=0.5;v=0.1;w=0.4
  subnode = Subnode.new(u,v,w)
  bary = subtri.barycentric(subnode)
  assert_in_delta u, bary[0], tol
  assert_in_delta v, bary[1], tol
  assert_in_delta w, bary[2], tol
 end

end