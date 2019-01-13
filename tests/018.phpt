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
string(59) "cannot pass an object directly to the sandbox at argument 1"
string(59) "cannot pass an object directly to the sandbox at argument 2"
string(59) "cannot pass an object directly to the sandbox at argument 3"
string(49) "cannot return an object directly from the sandbox"
string(62) "cannot pass by reference directly to the sandbox at argument 1"
string(62) "cannot pass by reference directly to the sandbox at argument 2"
string(62) "cannot pass by reference directly to the sandbox at argument 3"
string(52) "cannot return by reference directly from the sandbox"

