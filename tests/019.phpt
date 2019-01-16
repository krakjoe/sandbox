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
	$sandbox->enter(function($arg, $arg2, ... $arg3) {}, [
		1, 2, new stdClass
	]);
} catch (Error $ex) {
	var_dump($ex->getMessage());
}

try {
	$sandbox->enter(function($array) {}, [
		[new stdClass]
	]);
} catch (Error $ex) {
	var_dump($ex->getMessage());
}

try {
	$sandbox->enter(function($array) {}, [
		[STDIN]
	]);
} catch (Error $ex) {
	var_dump($ex->getMessage());
}
?>
--EXPECT--
string(57) "illegal variable (object) passed to sandbox at argument 1"
string(57) "illegal variable (object) passed to sandbox at argument 2"
string(57) "illegal variable (object) passed to sandbox at argument 3"
string(57) "illegal variable (object) passed to sandbox at argument 1"
string(59) "illegal variable (resource) passed to sandbox at argument 1"



