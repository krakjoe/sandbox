--TEST--
ZEND_DECLARE_FUNCTION
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
		function test() {}
	});
} catch (Error $ex) {
	var_dump($ex->getMessage());
}
?>
--EXPECT--
string(55) "illegal instruction (function) on line 1 of entry point"


