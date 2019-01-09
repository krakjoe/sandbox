--TEST--
Copy arginfo (OK)
--SKIPIF--
<?php
if (!extension_loaded('sandbox')) {
	echo 'skip';
}
?>
--FILE--
<?php
$sandbox = new sandbox\Runtime();

var_dump($sandbox->enter(function(int $int){
	return $int;
}, [42]));

var_dump($sandbox->enter(function(float $dbl){
	return $dbl;
}, [4.2]));

var_dump($sandbox->enter(function(string $str){
	return $str;
}, ["four"]));

var_dump($sandbox->enter(function(string $str) : string {
	return $str;
}, ["to"]));

var_dump($sandbox->enter(function(string ... $str) : string {
	return sprintf("%s %s", $str[0], $str[1]);
}, ["fourty", "two"]));
?>
--EXPECT--
int(42)
float(4.2)
string(4) "four"
string(2) "to"
string(10) "fourty two"

