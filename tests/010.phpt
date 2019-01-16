--TEST--
ZEND_DECLARE_INHERITED_CLASS
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
	$sandbox->enter(function(){
		class Foo extends Bar {}
	});
} catch (Throwable $t) {
	var_dump($t->getMessage());
}
?>
--EXPECT--
string(52) "illegal instruction (class) on line 1 of entry point"



