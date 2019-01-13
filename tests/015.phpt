--TEST--
ZEND_YIELD
--SKIPIF--
<?php
if (!extension_loaded('sandbox')) {
	echo 'skip';
}
?>
--FILE--
<?php
$sandbox = new sandbox\Runtime();
$var     = null;

try {
	$sandbox->enter(function() {
		yield;
	});
} catch (Error $ex) {
	var_dump($ex->getMessage());
}
?>
--EXPECT--
string(52) "illegal instruction (yield) on line 1 of entry point"


