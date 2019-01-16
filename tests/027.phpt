--TEST--
sandbox may disable functions and classes
--SKIPIF--
<?php
if (!extension_loaded('sandbox')) {
	echo 'skip';
}
?>
--FILE--
<?php
$sandbox = new \sandbox\Runtime([
	"disable_functions" => [
		"getenv",
		"strlen"
	],
	"disable_classes" => "stdClass,DateTime"
]);

$sandbox->enter(function(){
	new stdClass;
	getenv();
	strlen();
	new DateTime;
});
?>
--EXPECTF--
Warning: stdClass() has been disabled for security reasons in %s on line 11

Warning: getenv() has been disabled for security reasons in %s on line 12

Warning: strlen() has been disabled for security reasons in %s on line 13

Warning: DateTime() has been disabled for security reasons in %s on line 14

