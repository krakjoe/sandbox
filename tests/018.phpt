--TEST--
Copy arginfo (FAIL)
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
	$sandbox->enter(function(stdClass $arg) {});
} catch (Error $ex) {
	var_dump($ex->getMessage());
}

try {
	$sandbox->enter(function($arg, stdClass $arg2) {});
} catch (Error $ex) {
	var_dump($ex->getMessage());
}

try {
	$sandbox->enter(function($arg, $arg2, stdClass ... $arg3) {});
} catch (Error $ex) {
	var_dump($ex->getMessage());
}

try {
	$sandbox->enter(function() : stdClass {});
} catch (Error $ex) {
	var_dump($ex->getMessage());
}

try {
	$sandbox->enter(function(&$arg) {});
} catch (Error $ex) {
	var_dump($ex->getMessage());
}

try {
	$sandbox->enter(function($arg, &$arg2) {});
} catch (Error $ex) {
	var_dump($ex->getMessage());
}

try {
	$sandbox->enter(function($arg, $arg2, & ... $arg3) {});
} catch (Error $ex) {
	var_dump($ex->getMessage());
}

try {
	$sandbox->enter(function & () : int {});
} catch (Error $ex) {
	var_dump($ex->getMessage());
}
?>
--EXPECT--
string(55) "illegal type (object) accepted by sandbox at argument 1"
string(55) "illegal type (object) accepted by sandbox at argument 2"
string(55) "illegal type (object) accepted by sandbox at argument 3"
string(41) "illegal type (object) returned by sandbox"
string(62) "illegal variable (object) accepted by to sandbox at argument 1"
string(62) "illegal variable (object) accepted by to sandbox at argument 2"
string(62) "illegal variable (object) accepted by to sandbox at argument 3"
string(48) "illegal variable (reference) returned by sandbox"



