--TEST--
Copy argv (FAIL)
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
	$sandbox->enter(function($arg) {}, [
		new stdClass
	]);
} catch (Error $ex) {
	var_dump($ex->getMessage());
}

try {
	$sandbox->enter(function($arg, $arg2) {}, [
		1, new stdClass
	]);
} catch (Error $ex) {
	var_dump($ex->getMessage());
}

try {
	$sandbox->enter(function($arg, $arg2, stdClass ... $arg3) {}, [
		1, new stdClass
	]);
} catch (Error $ex) {
	var_dump($ex->getMessage());
}
?>
--EXPECT--
string(59) "cannot pass an object directly to the sandbox at argument 1"
string(59) "cannot pass an object directly to the sandbox at argument 2"
string(59) "cannot pass an object directly to the sandbox at argument 3"



