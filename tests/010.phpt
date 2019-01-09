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
--EXPECTF--
string(%d) "cannot declare class directly in the sandbox on line %d"


