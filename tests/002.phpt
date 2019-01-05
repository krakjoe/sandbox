--TEST--
Check sandbox global scope
--SKIPIF--
<?php
if (!extension_loaded('sandbox')) {
	echo 'skip';
}
?>
--FILE--
<?php
$sandbox = new sandbox\Runtime();

$sandbox->enter(function() {
	global $thing;

	$thing = 10;
});

$sandbox->enter(function() {
	global $thing;

	var_dump($thing);
});

var_dump(@$thing);
?>
--EXPECT--
int(10)
NULL
