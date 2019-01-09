--TEST--
ZEND_BIND_STATIC (OK)
--SKIPIF--
<?php
if (!extension_loaded('sandbox')) {
	echo 'skip';
}
?>
--FILE--
<?php
$sandbox = new sandbox\Runtime();

$var = 10;

var_dump($sandbox->enter(function(){
	static $var;

	$var++;

	return $var;
}));

var_dump($var);
?>
--EXPECT--
int(1)
int(10)
