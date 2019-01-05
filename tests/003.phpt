--TEST--
Check sandbox return values
--SKIPIF--
<?php
if (!extension_loaded('sandbox')) {
	echo 'skip';
}
?>
--FILE--
<?php
$sandbox = new sandbox\Runtime();

var_dump($sandbox->enter(function() {
	return 10;
}));
?>
--EXPECT--
int(10)
