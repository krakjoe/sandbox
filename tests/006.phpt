--TEST--
Check sandbox args
--SKIPIF--
<?php
if (!extension_loaded('sandbox')) {
	echo 'skip';
}
?>
--FILE--
<?php
try {
	$sandbox = new sandbox\Runtime(4,2);
} catch (Throwable $t) {
	var_dump($t->getMessage());
}
?>
--EXPECT--
string(37) "optional configuration array expected"

