--TEST--
ZEND_YIELD_FROM
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
		yield from [];
	});
} catch (Error $ex) {
	var_dump($ex->getMessage());
}
?>
--EXPECTF--
string(52) "illegal instruction (yield) on line 1 of entry point"

