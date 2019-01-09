--TEST--
ZEND_BIND_STATIC (FAIL)
--SKIPIF--
<?php
if (!extension_loaded('sandbox')) {
	echo 'skip';
}
?>
--FILE--
<?php
$sandbox = new sandbox\Runtime();
$var     = null; /* avoid undefined */

try {
	$sandbox->enter(function() use($var) {});
} catch (Error $ex) {
	var_dump($ex->getMessage());
}
?>
--EXPECT--
string(38) "cannot bind static vars in the sandbox"

