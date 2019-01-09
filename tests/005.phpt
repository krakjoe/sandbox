--TEST--
Check sandbox configuration
--SKIPIF--
<?php
if (!extension_loaded('sandbox')) {
	echo 'skip';
}
?>
--FILE--
<?php
$sandbox = new sandbox\Runtime([
	"include_path" => "dummy",
	"cast_to_string" => 2.2
]);

$sandbox->enter(function() {
	var_dump(ini_get("include_path"));
});
?>
--EXPECTF--
string(%d) "dummy"
