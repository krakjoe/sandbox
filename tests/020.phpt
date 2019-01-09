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
}, ["hell"]));
?>
--EXPECT--
int(42)
float(4.2)
string(4) "hell"

