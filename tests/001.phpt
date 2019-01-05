--TEST--
Check basic sandbox operation
--SKIPIF--
<?php
if (!extension_loaded('sandbox')) {
	echo 'skip';
}
?>
--FILE--
<?php
$sandbox = new sandbox\Runtime();

$sandbox->enter(function(){
	echo "OK";
});
?>
--EXPECT--
OK
