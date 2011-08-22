require 'minitest/autorun'
require 'yaji'
require 'curb'

class TestParser < MiniTest::Unit::TestCase

  def test_it_generates_events
    events = []
    parser = YAJI::Parser.new(toys_json_str)
    parser.parse do |p, e, v|
      events << [p, e, v]
    end
    expected = [
      ["",                      :start_hash,  nil],
      ["",                      :hash_key,    "total_rows"],
      ["total_rows",            :number,      2],
      ["",                      :hash_key,    "rows"],
      ["rows",                  :start_array, nil],
      ["rows/",                 :start_hash,  nil],
      ["rows/",                 :hash_key,    "id"],
      ["rows//id",              :string,      "buzz"],
      ["rows/",                 :hash_key,    "props"],
      ["rows//props",           :start_hash,  nil],
      ["rows//props",           :hash_key,    "humanoid"],
      ["rows//props/humanoid",  :boolean,     true],
      ["rows//props",           :hash_key,    "armed"],
      ["rows//props/armed",     :boolean,     true],
      ["rows//props",           :end_hash,    nil],
      ["rows/",                 :hash_key,    "movies"],
      ["rows//movies",          :start_array, nil],
      ["rows//movies/",         :number,      1],
      ["rows//movies/",         :number,      2],
      ["rows//movies/",         :number,      3],
      ["rows//movies",          :end_array,   nil],
      ["rows/",                 :end_hash,    nil],
      ["rows/",                 :start_hash,  nil],
      ["rows/",                 :hash_key,    "id"],
      ["rows//id",              :string,      "barbie"],
      ["rows/",                 :hash_key,    "props"],
      ["rows//props",           :start_hash,  nil],
      ["rows//props",           :hash_key,    "humanoid"],
      ["rows//props/humanoid",  :boolean,     true],
      ["rows//props",           :hash_key,    "armed"],
      ["rows//props/armed",     :boolean,     false],
      ["rows//props",           :end_hash,    nil],
      ["rows/",                 :hash_key,    "movies"],
      ["rows//movies",          :start_array, nil],
      ["rows//movies/",         :number,      2],
      ["rows//movies/",         :number,      3],
      ["rows//movies",          :end_array,   nil],
      ["rows/",                 :end_hash,    nil],
      ["rows",                  :end_array,   nil],
      ["",                      :end_hash,    nil]
    ]
    assert_equal expected, events
  end

  def test_it_yields_enumerator
    parser = YAJI::Parser.new('{"hello":"world"}')
    e = parser.parse
    assert_equal ["", :start_hash, nil], e.next
    assert_equal ["", :hash_key, "hello"], e.next
    assert_equal ["hello", :string, "world"], e.next
    assert_equal ["", :end_hash, nil], e.next
    assert_raises(StopIteration) { e.next }
  end

  def test_it_symbolizes_keys
    parser = YAJI::Parser.new('{"hello":"world"}', :symbolize_keys => true)
    e = parser.parse
    expected = [
      ["", :start_hash, nil],
      ["", :hash_key, :hello],
      ["hello", :string, "world"],
      ["", :end_hash, nil]
    ]
    assert_equal expected, e.to_a
  end

  def test_it_build_ruby_objects
    parser = YAJI::Parser.new(toys_json_str)
    objects = []
    parser.each do |o|
      objects << o
    end
    expected = [{"total_rows" => 2,
                 "rows" => [
                   {
                     "id" => "buzz",
                     "props" => { "humanoid"=> true, "armed"=> true },
                     "movies" => [1,2,3]
                   },
                   {
                     "id" => "barbie",
                     "props" => { "humanoid"=> true, "armed"=> false },
                     "movies" => [2,3]
                   }
                 ]}]
    assert_equal expected, objects
  end

  def test_it_yields_whole_array
    parser = YAJI::Parser.new(toys_json_str)
    objects = []
    parser.each("rows") do |o|
      objects << o
    end
    expected = [[{
                   "id" => "buzz",
                   "props" => { "humanoid"=> true, "armed"=> true },
                   "movies" => [1,2,3]
                 },
                 {
                   "id" => "barbie",
                   "props" => { "humanoid"=> true, "armed"=> false },
                   "movies" => [2,3]
                 }]]
    assert_equal expected, objects
  end

  def test_it_yeilds_array_contents_row_by_row
    parser = YAJI::Parser.new(toys_json_str)
    objects = []
    parser.each("rows/") do |o|
      objects << o
    end
    expected = [{
                  "id" => "buzz",
                  "props" => { "humanoid"=> true, "armed"=> true },
                  "movies" => [1,2,3]
                },
                {
                  "id" => "barbie",
                  "props" => { "humanoid"=> true, "armed"=> false },
                  "movies" => [2,3]
                }]
    assert_equal expected, objects
  end

  def test_it_could_curb_async_approach
    curl = Curl::Easy.new('http://avsej.net/test.json')
    parser = YAJI::Parser.new(curl)
    object = parser.each.to_a.first
    expected = {"foo"=>"bar", "baz"=>{"nums"=>[42, 3.1415]}}
    assert_equal expected, object
  end

  def test_it_allow_several_selectors
    parser = YAJI::Parser.new(toys_json_str)
    objects = []
    parser.each(["total_rows", "rows/"]) do |o|
      objects << o
    end
    expected = [2,
                {
                  "id" => "buzz",
                  "props" => { "humanoid"=> true, "armed"=> true },
                  "movies" => [1,2,3]
                },
                {
                  "id" => "barbie",
                  "props" => { "humanoid"=> true, "armed"=> false },
                  "movies" => [2,3]
                }]
    assert_equal expected, objects
  end

  def test_it_doesnt_raise_exception_on_empty_input
    YAJI::Parser.new("").parse
    YAJI::Parser.new("  ").parse
    YAJI::Parser.new("\n").parse
    YAJI::Parser.new(" \n\n ").parse
  end

  protected

  def toys_json_str
    <<-JSON
      {
        "total_rows": 2,
        "rows": [
          {
            "id": "buzz",
            "props": {
              "humanoid": true,
              "armed": true
            },
            "movies": [1,2,3]
          },
          {
            "id": "barbie",
            "props": {
              "humanoid": true,
              "armed": false
            },
            "movies": [2,3]
          }
        ]
      }
    JSON
  end
end
