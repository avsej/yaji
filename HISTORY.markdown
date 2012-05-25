=== 0.3.1 / 2012-05-25

* Protect on_object callback from GC

=== 0.3.0 / 2012-04-18

* Allow to specify filter at parser initialization
* Implement feeding of the input on the fly

=== 0.2.3 / 2012-04-10

* Add build for Windows

=== 0.2.2 / 2012-03-14

* Fix the README file

=== 0.2.1 / 2012-03-14

* Revert "Close input after work"
* Update rake dependency
* Check chunk for nil before stripping

=== 0.2.0 / 2012-01-12

* Fix root element (broken backward compatibility, see tests)
* Close input when work is done

=== 0.1.2 / 2011-12-13

* Finish yajl API hiding

=== 0.1.1 / 2011-12-13

* Do not export yajl functions

=== 0.1.0 / 2011-12-12

* Skip empty and nil chunks

=== 0.0.9 / 2011-09-01

* Optionally yield path in object iterator

=== 0.0.8 / 2011-08-30

* Use rb_str_new2 for backward comatibility with ruby 1.8.x

=== 0.0.7 / 2011-08-24

* Fix include paths

=== 0.0.6 / 2011-08-24

* bundle yajl sources because it's the easiest way to handle dependency
  for now

=== 0.0.5 / 2011-08-22

* Don't fail on empty input

=== 0.0.4 / 2011-08-22

* Pass multiple filter explicitly as an array

=== 0.0.3 / 2011-08-19

* Allow multiple filters to select sibling JSON nodes

=== 0.0.2 / 2011-08-19

* Integration with curb gem

=== 0.0.1 / 2011-08-19

* Initial public release
