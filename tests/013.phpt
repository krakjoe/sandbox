--TEST--
ZEND_DECLARE_LAMBDA_FUNCTION
--SKIPIF--
<?php
if (!extension_loaded('sandbox')) {
	echo 'skip';
}
?>
--FILE--
<?php
$sandbox = new sandbox\Runtime();

try {
	$sandbox->enter(function() {
		function() {};
	});
} catch (Error $ex) {
	var_dump($ex->getMessage());
}
?>
--EXPECT--
string(58) "cannot declare anonymous functions directly in the sandbox"


