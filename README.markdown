Yet another JSON iterator
=========================

YAJI is a ruby wrapper to YAJL providing iterator interface to streaming JSON parser.

INSTALL
-------

This gem depend on [yajl][1]. So you need development headers installed
on your system to build this gem. For Debian GNU/Linux family it will be something like:

    sudo apt-get install libyajl-dev

Now you ready to install YAJI gem:

    gem install yaji

USAGE
-----

YAJI::Parser initializer accepts `IO` instance or `String`.

    require 'yaji'

    YAJI::Parser.new('{"foo":"bar"}')
    YAJI::Parser.new(File.open('data.json'))

There is integration with [curb][2], so you can pass `Curl::Easy` instance to
as input for parser.

    require 'curl'
    curl = Curl::Easy.new('http://avsej.net/test.json')
    parser = YAJI::Parser.new(curl)
    parser.each.to_a.first  #=> {"foo"=>"bar", "baz"=>{"nums"=>[42, 3.1415]}}

There no strict requirement though, it could be any instance responding
to `#on_body` and `#perform`.

Parser instance provides two iterators to get JSON data: event-oriented
and object-oriented. `YAJI::Parser#parse` yields tuple `[path, event,
value] describing some parser event. For example, this code

    parser = YAJI::Parser.new('{"foo":[1, {"bar":"baz"}]}')
    parser.parse do |path, event, value|
      puts [path, event, value].inspect
    end

prints all parser events

    ["", :start_hash, nil]
    ["", :hash_key, "foo"]
    ["foo", :start_array, nil]
    ["foo/", :number, 1]
    ["foo/", :start_hash, nil]
    ["foo/", :hash_key, "bar"]
    ["foo//bar", :string, "baz"]
    ["foo/", :end_hash, nil]
    ["foo", :end_array, nil]
    ["", :end_hash, nil]

You can call `#parse` method without block and it will return
`Enumerator` object.

The another approach is to use `YAJI::Parser#each` method to iterate
over JSON objects. It accepts optional filter parameter if you'd like to
iterated sub-objects. Here is the example

    parser = YAJI::Parser.new('{"size":2,"items":[{"id":1}, {"id":2}]}')
    parser.each do |obj|
      puts obj.inspect
    end

will print only one line:

    {"size"=>2, "items"=>[{"id"=>1}, {"id"=>2}]}

But it might be more useful to yield items from inner array:

    parser = YAJI::Parser.new('{"size":2,"items":[{"id":1}, {"id":2}]}')
    parser.each("items/") do |obj|
      puts obj.inspect
    end

code above will print two lines:

    {"id"=>1}
    {"id"=>2}

You can use this iterator when the data is huge and you'd like to allow
GC to collect yielded object before parser finish its job.


LICENSE
-------

Copyright 2011 Couchbase, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

[1]: http://lloyd.github.com/yajl/
[2]: https://rubygems.org/gems/curb/
