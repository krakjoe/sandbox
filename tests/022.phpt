--TEST--
return destroyed
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
	return new stdClass;
});

echo "OK";
?>
--EXPECT--
OK

